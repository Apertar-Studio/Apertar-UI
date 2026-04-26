#include "ApertarPreviewSocketClient.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <utility>

ApertarPreviewSocketClient::ApertarPreviewSocketClient(QObject *parent)
    : QObject(parent)
{
    m_reconnectTimer.setInterval(1000);
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout,
            this, [this]() {
                if (!m_socketPath.isEmpty())
                    connectToCore(m_socketPath);
            });

    setStatusText(QStringLiteral("Apertar-Core preview socket disconnected"));
}

ApertarPreviewSocketClient::~ApertarPreviewSocketClient()
{
    disconnectFromCore();
}

bool ApertarPreviewSocketClient::connectToCore(const QString &socketPath)
{
    closeSocket();
    m_reconnectTimer.stop();
    m_socketPath = socketPath;

    const QByteArray pathBytes = socketPath.toUtf8();

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    if (pathBytes.size() >= static_cast<int>(sizeof(address.sun_path))) {
        setStatusText(QStringLiteral("ApertarCore socket path is too long"));
        return false;
    }

    m_socketFd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (m_socketFd < 0) {
        setStatusText(QStringLiteral("socket() failed: %1").arg(QString::fromLocal8Bit(std::strerror(errno))));
        scheduleReconnect();
        return false;
    }

    std::strncpy(address.sun_path, pathBytes.constData(), sizeof(address.sun_path) - 1);

    if (::connect(m_socketFd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        if (errno != EINPROGRESS) {
            if (errno == ENOENT || errno == ECONNREFUSED)
                setStatusText(QStringLiteral("Waiting for Apertar-Core preview socket..."));
            else
                setStatusText(QStringLiteral("connect() failed: %1").arg(QString::fromLocal8Bit(std::strerror(errno))));
            ::close(m_socketFd);
            m_socketFd = -1;
            scheduleReconnect();
            return false;
        }
    }

    m_notifier = new QSocketNotifier(m_socketFd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated,
            this, &ApertarPreviewSocketClient::readAvailableMessages);

    setConnected(true);
    setStatusText(QStringLiteral("Connected to Apertar-Core preview socket"));
    return true;
}

void ApertarPreviewSocketClient::disconnectFromCore()
{
    m_reconnectTimer.stop();
    m_socketPath.clear();
    closeSocket();
}

void ApertarPreviewSocketClient::closeSocket()
{
    if (m_notifier) {
        m_notifier->setEnabled(false);
        m_notifier->deleteLater();
        m_notifier = nullptr;
    }

    if (m_socketFd >= 0) {
        ::close(m_socketFd);
        m_socketFd = -1;
    }

    closePreviewFd();
    closePendingFds();
    m_readBuffer.clear();
    setConnected(false);
}

void ApertarPreviewSocketClient::closePendingFds()
{
    for (int fd : m_pendingFds) {
        if (fd >= 0)
            ::close(fd);
    }
    m_pendingFds.clear();
}

void ApertarPreviewSocketClient::scheduleReconnect()
{
    if (m_socketPath.isEmpty() || m_reconnectTimer.isActive())
        return;

    m_reconnectTimer.start();
}

std::optional<PreviewFrameInfo> ApertarPreviewSocketClient::currentPreviewFrame() const
{
    if (m_previewFd < 0 || m_latestFrame.width == 0 || m_latestFrame.height == 0)
        return std::nullopt;
    return m_latestFrame;
}

int ApertarPreviewSocketClient::duplicateProducerFd(int procid, int fd) const
{
    if (procid != kOwnedFdProcId || fd < 0)
        return -1;
    return ::dup(fd);
}

void ApertarPreviewSocketClient::readAvailableMessages()
{
    while (m_socketFd >= 0) {
        std::array<char, 8192> buffer{};
        std::array<char, CMSG_SPACE(sizeof(int))> control{};

        iovec iov{};
        iov.iov_base = buffer.data();
        iov.iov_len = buffer.size();

        msghdr msg{};
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = control.data();
        msg.msg_controllen = control.size();

        const ssize_t received = ::recvmsg(m_socketFd, &msg, MSG_DONTWAIT);
        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;

            setStatusText(QStringLiteral("recvmsg() failed: %1").arg(QString::fromLocal8Bit(std::strerror(errno))));
            closeSocket();
            scheduleReconnect();
            return;
        }

        if (received == 0) {
            setStatusText(QStringLiteral("Apertar-Core preview socket closed"));
            closeSocket();
            scheduleReconnect();
            return;
        }

        for (cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
                const size_t payloadBytes = cmsg->cmsg_len - CMSG_LEN(0);
                const int fdCount = static_cast<int>(payloadBytes / sizeof(int));
                const int *fds = reinterpret_cast<const int *>(CMSG_DATA(cmsg));
                for (int i = 0; i < fdCount; ++i)
                    m_pendingFds.push_back(fds[i]);
            }
        }

        m_readBuffer.append(buffer.data(), static_cast<int>(received));
        processReadBuffer();
    }
}

void ApertarPreviewSocketClient::processReadBuffer()
{
    while (true) {
        const int newline = m_readBuffer.indexOf('\n');
        if (newline < 0)
            break;

        QByteArray line = m_readBuffer.left(newline).trimmed();
        m_readBuffer.remove(0, newline + 1);
        if (line.isEmpty())
            continue;

        handleMessage(line, takePendingFdsForMessage(line));
    }

    if (m_readBuffer.size() > 1024 * 1024) {
        m_readBuffer.clear();
        closePendingFds();
        setStatusText(QStringLiteral("Dropped oversized ApertarCore preview message."));
    }
}

QVector<int> ApertarPreviewSocketClient::takePendingFdsForMessage(const QByteArray &message)
{
    if (!message.contains("\"preview_frame\"") || m_pendingFds.isEmpty())
        return {};

    qlonglong requestedCount = 1;
    jsonIntegerValue(message, "plane_count", &requestedCount);
    requestedCount = qBound(qlonglong(1), requestedCount, qlonglong(3));

    QVector<int> fds;
    fds.reserve(static_cast<int>(requestedCount));
    while (!m_pendingFds.isEmpty() && fds.size() < requestedCount)
        fds.push_back(m_pendingFds.takeFirst());

    return fds;
}

void ApertarPreviewSocketClient::setConnected(bool connected)
{
    if (m_connected == connected)
        return;
    m_connected = connected;
    emit connectedChanged();
}

void ApertarPreviewSocketClient::setStatusText(const QString &text)
{
    if (m_statusText == text)
        return;
    m_statusText = text;
    emit statusTextChanged();
}

void ApertarPreviewSocketClient::closePreviewFd()
{
    if (m_latestFrame.planeCount > 0) {
        for (int i = 0; i < m_latestFrame.planeCount && i < 3; ++i) {
            if (m_latestFrame.planeFds[i] >= 0)
                ::close(m_latestFrame.planeFds[i]);
        }
    } else if (m_previewFd >= 0) {
        ::close(m_previewFd);
    }

    m_previewFd = -1;
    m_latestFrame = PreviewFrameInfo{};
}

void ApertarPreviewSocketClient::handleMessage(const QByteArray &message, QVector<int> attachedFds)
{
    auto closeAttachedFds = [&attachedFds]() {
        for (int fd : attachedFds) {
            if (fd >= 0)
                ::close(fd);
        }
        attachedFds.clear();
    };

    QByteArray event;
    if (!jsonStringValue(message, "event", &event)) {
        closeAttachedFds();
        return;
    }

    if (event == "preview_frame") {
        handlePreviewFrame(message, std::move(attachedFds));
        return;
    }

    closeAttachedFds();
}

void ApertarPreviewSocketClient::handlePreviewFrame(const QByteArray &message, QVector<int> attachedFds)
{
    auto closeAttachedFds = [&attachedFds]() {
        for (int fd : attachedFds) {
            if (fd >= 0)
                ::close(fd);
        }
        attachedFds.clear();
    };

    if (attachedFds.isEmpty())
        return;

    qlonglong frameId = 0;
    qlonglong width = 0;
    qlonglong height = 0;
    qlonglong stride = 0;
    qlonglong captureWidth = 0;
    qlonglong captureHeight = 0;
    qlonglong planeCount = 1;
    if (!jsonIntegerValue(message, "frame_id", &frameId) ||
        !jsonIntegerValue(message, "width", &width) ||
        !jsonIntegerValue(message, "height", &height) ||
        !jsonIntegerValue(message, "stride", &stride)) {
        closeAttachedFds();
        return;
    }
    jsonIntegerValue(message, "plane_count", &planeCount);
    jsonIntegerValue(message, "capture_width", &captureWidth);
    jsonIntegerValue(message, "capture_height", &captureHeight);
    planeCount = qBound(qlonglong(1), planeCount, qlonglong(3));

    if (attachedFds.size() < planeCount)
        planeCount = attachedFds.size();

    if (planeCount <= 0) {
        closeAttachedFds();
        return;
    }

    closePreviewFd();
    m_previewFd = attachedFds.front();

    PreviewFrameInfo frame;
    frame.procid = kOwnedFdProcId;
    frame.fdIsp = m_previewFd;
    frame.planeCount = static_cast<int>(planeCount);
    frame.frame = static_cast<uint64_t>(frameId);
    frame.sequence = static_cast<unsigned int>(frameId);
    frame.width = static_cast<unsigned int>(width);
    frame.height = static_cast<unsigned int>(height);
    frame.stride = static_cast<unsigned int>(stride);
    frame.captureWidth = static_cast<unsigned int>(qMax<qlonglong>(0, captureWidth));
    frame.captureHeight = static_cast<unsigned int>(qMax<qlonglong>(0, captureHeight));
    for (int i = 0; i < frame.planeCount; ++i) {
        qlonglong offset = 0;
        qlonglong pitch = 0;
        jsonIntegerValue(message, QByteArray("plane") + QByteArray::number(i) + "_offset", &offset);
        jsonIntegerValue(message, QByteArray("plane") + QByteArray::number(i) + "_pitch", &pitch);
        frame.planeFds[i] = attachedFds.at(i);
        frame.planeOffsets[i] = static_cast<unsigned int>(qMax<qlonglong>(0, offset));
        frame.planePitches[i] = static_cast<unsigned int>(qMax<qlonglong>(0, pitch));
    }

    for (int i = frame.planeCount; i < attachedFds.size(); ++i) {
        if (attachedFds.at(i) >= 0)
            ::close(attachedFds.at(i));
    }
    attachedFds.clear();

    m_latestFrame = frame;
    m_lastFrame = frame.frame;
    emit frameArrived();
    emit previewFrameReady();
}

bool ApertarPreviewSocketClient::jsonStringValue(const QByteArray &message, const QByteArray &key, QByteArray *value) const
{
    const QByteArray pattern = "\"" + key + "\":\"";
    const int start = message.indexOf(pattern);
    if (start < 0)
        return false;

    const int valueStart = start + pattern.size();
    const int valueEnd = message.indexOf('"', valueStart);
    if (valueEnd < 0)
        return false;

    if (value)
        *value = message.mid(valueStart, valueEnd - valueStart);
    return true;
}

bool ApertarPreviewSocketClient::jsonIntegerValue(const QByteArray &message, const QByteArray &key, qlonglong *value) const
{
    const QByteArray pattern = "\"" + key + "\":";
    const int start = message.indexOf(pattern);
    if (start < 0)
        return false;

    int pos = start + pattern.size();
    while (pos < message.size() && message.at(pos) == ' ')
        ++pos;

    int end = pos;
    while (end < message.size() && ((message.at(end) >= '0' && message.at(end) <= '9') || message.at(end) == '-'))
        ++end;

    bool ok = false;
    const qlonglong parsed = message.mid(pos, end - pos).toLongLong(&ok);
    if (!ok)
        return false;

    if (value)
        *value = parsed;
    return true;
}
