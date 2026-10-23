#include "ApertarPreviewSocketClient.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <QDebug>
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
    if (!requestPreviewSubscription()) {
        QTimer::singleShot(100, this, [this]() {
            if (m_socketFd >= 0)
                requestPreviewSubscription();
        });
    }
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
    closeRegisteredPreviewBuffers();
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
    if (m_latestFrame.fdIsp < 0 || m_latestFrame.width == 0 || m_latestFrame.height == 0)
        return std::nullopt;
    return m_latestFrame;
}

int ApertarPreviewSocketClient::duplicateProducerFd(int procid, int fd) const
{
    if (procid != kOwnedFdProcId || fd < 0)
        return -1;
    return ::dup(fd);
}

bool ApertarPreviewSocketClient::requestPreviewSubscription()
{
    if (m_socketFd < 0)
        return false;

    QByteArray payload = QByteArrayLiteral("{\"cmd\":\"preview_subscribe\",\"id\":0}\n");
    const char *data = payload.constData();
    size_t remaining = static_cast<size_t>(payload.size());
    while (remaining > 0) {
        const ssize_t written = ::send(m_socketFd, data, remaining, MSG_NOSIGNAL);
        if (written < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return false;
            return false;
        }
        if (written == 0)
            return false;

        data += written;
        remaining -= static_cast<size_t>(written);
    }

    return true;
}

void ApertarPreviewSocketClient::readAvailableMessages()
{
    while (m_socketFd >= 0) {
        static constexpr int kMaxAttachedFds = 3;
        std::array<char, 8192> buffer{};
        std::array<char, CMSG_SPACE(sizeof(int) * kMaxAttachedFds)> control{};

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
    if (m_pendingFds.isEmpty())
        return {};

    QByteArray event;
    if (!jsonStringValue(message, "event", &event))
        return {};

    qlonglong requestedCount = 0;
    if (event == "preview_buffer") {
        requestedCount = 1;
        jsonIntegerValue(message, "plane_count", &requestedCount);
        requestedCount = qBound(qlonglong(1), requestedCount, qlonglong(3));
    } else if (event == "preview_frame") {
        if (message.contains("\"fd_attached\":true")) {
            requestedCount = 1;
            jsonIntegerValue(message, "plane_count", &requestedCount);
            requestedCount = qBound(qlonglong(1), requestedCount, qlonglong(3));
        }
    } else {
        return {};
    }

    if (requestedCount <= 0)
        return {};

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

void ApertarPreviewSocketClient::releaseOwnedPreviewFds()
{
    if (m_latestFrame.ownsPlaneFds && m_latestFrame.planeCount > 0) {
        for (int i = 0; i < m_latestFrame.planeCount && i < 3; ++i) {
            if (m_latestFrame.planeFds[i] >= 0)
                ::close(m_latestFrame.planeFds[i]);
        }
    } else if (m_previewFd >= 0) {
        ::close(m_previewFd);
    }

    m_previewFd = -1;
}

void ApertarPreviewSocketClient::closePreviewFd()
{
    releaseOwnedPreviewFds();
    m_latestFrame = PreviewFrameInfo{};
}

void ApertarPreviewSocketClient::closeRegisteredPreviewBuffers()
{
    for (auto &[slotIndex, buffer] : m_registeredPreviewBuffers) {
        (void)slotIndex;
        if (buffer.fd >= 0)
            ::close(buffer.fd);
        for (int fd : buffer.planeFds) {
            if (fd >= 0 && fd != buffer.fd)
                ::close(fd);
        }
    }
    m_registeredPreviewBuffers.clear();
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

    if (event == "preview_buffer") {
        handlePreviewBuffer(message, std::move(attachedFds));
        return;
    }

    if (event == "preview_frame") {
        handlePreviewFrame(message, std::move(attachedFds));
        return;
    }

    closeAttachedFds();
}

void ApertarPreviewSocketClient::handlePreviewBuffer(const QByteArray &message, QVector<int> attachedFds)
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

    qlonglong slotIndex = 0;
    qlonglong width = 0;
    qlonglong height = 0;
    qlonglong stride = 0;
    qlonglong captureWidth = 0;
    qlonglong captureHeight = 0;
    qlonglong planeCount = 1;
    if (!jsonIntegerValue(message, "slot", &slotIndex) ||
        !jsonIntegerValue(message, "width", &width) ||
        !jsonIntegerValue(message, "height", &height) ||
        !jsonIntegerValue(message, "stride", &stride)) {
        closeAttachedFds();
        return;
    }

    jsonIntegerValue(message, "capture_width", &captureWidth);
    jsonIntegerValue(message, "capture_height", &captureHeight);
    jsonIntegerValue(message, "layout_plane_count", &planeCount);
    planeCount = qBound(qlonglong(1), planeCount, qlonglong(3));

    RegisteredPreviewBuffer buffer;
    buffer.fd = attachedFds.front();
    buffer.width = static_cast<unsigned int>(qMax<qlonglong>(0, width));
    buffer.height = static_cast<unsigned int>(qMax<qlonglong>(0, height));
    buffer.stride = static_cast<unsigned int>(qMax<qlonglong>(0, stride));
    buffer.captureWidth = static_cast<unsigned int>(qMax<qlonglong>(0, captureWidth));
    buffer.captureHeight = static_cast<unsigned int>(qMax<qlonglong>(0, captureHeight));
    buffer.planeCount = static_cast<int>(planeCount);
    for (int i = 0; i < buffer.planeCount && i < attachedFds.size(); ++i)
        buffer.planeFds[i] = attachedFds.at(i);
    for (int i = 0; i < buffer.planeCount; ++i) {
        qlonglong offset = 0;
        qlonglong pitch = 0;
        jsonIntegerValue(message, QByteArray("plane") + QByteArray::number(i) + "_offset", &offset);
        jsonIntegerValue(message, QByteArray("plane") + QByteArray::number(i) + "_pitch", &pitch);
        buffer.planeOffsets[i] = static_cast<unsigned int>(qMax<qlonglong>(0, offset));
        buffer.planePitches[i] = static_cast<unsigned int>(qMax<qlonglong>(0, pitch));
    }

    const int attachedFdCount = attachedFds.size();
    for (int i = buffer.planeCount; i < attachedFdCount; ++i) {
        if (attachedFds.at(i) >= 0)
            ::close(attachedFds.at(i));
    }
    attachedFds.clear();

    const unsigned int slot = static_cast<unsigned int>(qMax<qlonglong>(0, slotIndex));
    auto existing = m_registeredPreviewBuffers.find(slot);
    if (existing != m_registeredPreviewBuffers.end()) {
        if (existing->second.fd >= 0)
            ::close(existing->second.fd);
        for (int fd : existing->second.planeFds) {
            if (fd >= 0 && fd != existing->second.fd)
                ::close(fd);
        }
    }
    m_registeredPreviewBuffers[slot] = buffer;

    if (!m_loggedPreviewBuffer) {
        qInfo() << "Apertar preview buffer registered:"
                << buffer.width << "x" << buffer.height
                << "stride" << buffer.stride
                << "slot" << slot
                << "layout planes" << buffer.planeCount
                << "attached fds" << qMax(1, attachedFdCount);
        m_loggedPreviewBuffer = true;
    }

    if (m_latestFrame.fdIsp >= 0 && m_latestFrame.slotIndex == slot) {
        m_latestFrame.fdIsp = buffer.fd;
        m_latestFrame.width = buffer.width;
        m_latestFrame.height = buffer.height;
        m_latestFrame.stride = buffer.stride;
        m_latestFrame.captureWidth = buffer.captureWidth;
        m_latestFrame.captureHeight = buffer.captureHeight;
        m_latestFrame.planeCount = buffer.planeCount;
        m_latestFrame.ownsPlaneFds = false;
        for (int i = 0; i < buffer.planeCount; ++i) {
            m_latestFrame.planeFds[i] = buffer.planeFds[i];
            m_latestFrame.planeOffsets[i] = buffer.planeOffsets[i];
            m_latestFrame.planePitches[i] = buffer.planePitches[i];
        }
        emit frameArrived();
        emit previewFrameReady();
    }
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

    qlonglong frameId = 0;
    qlonglong slotIndex = 0;
    if (!jsonIntegerValue(message, "frame_id", &frameId)) {
        closeAttachedFds();
        return;
    }

    if (attachedFds.isEmpty()) {
        if (!jsonIntegerValue(message, "slot", &slotIndex))
            return;

        const unsigned int slot = static_cast<unsigned int>(qMax<qlonglong>(0, slotIndex));
        auto it = m_registeredPreviewBuffers.find(slot);
        if (it == m_registeredPreviewBuffers.end() || it->second.fd < 0)
            return;

        PreviewFrameInfo frame;
        frame.procid = kOwnedFdProcId;
        frame.slotIndex = slot;
        frame.fdIsp = it->second.fd;
        frame.frame = static_cast<uint64_t>(frameId);
        frame.sequence = static_cast<unsigned int>(frameId);
        frame.width = it->second.width;
        frame.height = it->second.height;
        frame.stride = it->second.stride;
        frame.captureWidth = it->second.captureWidth;
        frame.captureHeight = it->second.captureHeight;
        frame.planeCount = it->second.planeCount;
        frame.ownsPlaneFds = false;
        for (int i = 0; i < frame.planeCount; ++i) {
            frame.planeFds[i] = it->second.planeFds[i];
            frame.planeOffsets[i] = it->second.planeOffsets[i];
            frame.planePitches[i] = it->second.planePitches[i];
        }
        releaseOwnedPreviewFds();
        m_latestFrame = frame;
        m_lastFrame = frame.frame;
        if (!m_loggedPreviewFrame) {
            qInfo() << "Apertar preview frame received:"
                    << frame.width << "x" << frame.height
                    << "stride" << frame.stride
                    << "slot" << frame.slotIndex
                    << "planes" << frame.planeCount;
            m_loggedPreviewFrame = true;
        }
        emit frameArrived();
        emit previewFrameReady();
        return;
    }

    qlonglong width = 0;
    qlonglong height = 0;
    qlonglong stride = 0;
    qlonglong captureWidth = 0;
    qlonglong captureHeight = 0;
    qlonglong planeCount = 1;
    if (!jsonIntegerValue(message, "width", &width) ||
        !jsonIntegerValue(message, "height", &height) ||
        !jsonIntegerValue(message, "stride", &stride)) {
        closeAttachedFds();
        return;
    }
    jsonIntegerValue(message, "slot", &slotIndex);
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

    PreviewFrameInfo frame;
    frame.procid = kOwnedFdProcId;
    frame.fdIsp = attachedFds.front();
    frame.slotIndex = static_cast<unsigned int>(qMax<qlonglong>(0, slotIndex));
    frame.planeCount = static_cast<int>(planeCount);
    frame.ownsPlaneFds = true;
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

    releaseOwnedPreviewFds();
    m_previewFd = frame.fdIsp;
    m_latestFrame = frame;
    m_lastFrame = frame.frame;
    if (!m_loggedPreviewFrame) {
        qInfo() << "Apertar preview frame received:"
                << frame.width << "x" << frame.height
                << "stride" << frame.stride
                << "slot" << frame.slotIndex
                << "planes" << frame.planeCount
                << "attached";
        m_loggedPreviewFrame = true;
    }
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
