#include "PowerButtonBridge.hpp"

#include <QDir>
#include <QFile>
#include <QSocketNotifier>
#include <QTimer>

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

namespace {

QString readTrimmedTextFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    return QString::fromUtf8(file.readAll()).trimmed();
}

} // namespace

PowerButtonBridge::PowerButtonBridge(QObject *parent)
    : QObject(parent)
    , m_scanTimer(new QTimer(this))
{
    m_scanTimer->setInterval(1500);
    connect(m_scanTimer, &QTimer::timeout, this, [this]() {
        scanForDevice();
    });
    m_scanTimer->start();

    QTimer::singleShot(0, this, [this]() {
        scanForDevice();
    });
}

PowerButtonBridge::~PowerButtonBridge()
{
    closeDevice();
}

void PowerButtonBridge::scanForDevice()
{
    if (m_deviceFd >= 0)
        return;

    const QString eventPath = detectPowerButtonEventPath();
    if (eventPath.isEmpty()) {
        setAvailable(false);
        setDevicePath(QString());
        return;
    }

    openDevice(eventPath);
}

void PowerButtonBridge::readEvents()
{
    if (m_deviceFd < 0)
        return;

    struct input_event events[16];
    while (true) {
        const ssize_t bytesRead = ::read(m_deviceFd, events, sizeof(events));
        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;

            closeDevice();
            return;
        }

        if (bytesRead == 0) {
            closeDevice();
            return;
        }

        const ssize_t eventCount = bytesRead / static_cast<ssize_t>(sizeof(input_event));
        for (ssize_t i = 0; i < eventCount; ++i) {
            const input_event &event = events[i];
            if (event.type != EV_KEY || event.code != KEY_POWER)
                continue;

            if (event.value == 1) {
                if (!m_pressed) {
                    setPressed(true);
                    emit buttonPressed();
                }
            } else if (event.value == 0) {
                if (m_pressed) {
                    setPressed(false);
                    emit buttonReleased();
                }
            }
        }
    }
}

bool PowerButtonBridge::openDevice(const QString &path)
{
    const QByteArray nativePath = QFile::encodeName(path);
    const int fd = ::open(nativePath.constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        setAvailable(false);
        return false;
    }

    closeDevice();

    m_deviceFd = fd;
    m_notifier = new QSocketNotifier(m_deviceFd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, [this]() {
        readEvents();
    });

    setDevicePath(path);
    setAvailable(true);
    setPressed(false);
    return true;
}

QString PowerButtonBridge::detectPowerButtonEventPath() const
{
    const QDir inputDir(QStringLiteral("/sys/class/input"));
    const QFileInfoList eventEntries = inputDir.entryInfoList(
        QStringList() << QStringLiteral("event*"),
        QDir::Dirs | QDir::NoDotAndDotDot,
        QDir::Name);

    for (const QFileInfo &entry : eventEntries) {
        const QString deviceName = readTrimmedTextFile(entry.absoluteFilePath() + QStringLiteral("/device/name"));
        if (deviceName == QStringLiteral("pwr_button"))
            return QStringLiteral("/dev/input/") + entry.fileName();
    }

    return {};
}

void PowerButtonBridge::closeDevice()
{
    if (m_notifier) {
        m_notifier->setEnabled(false);
        m_notifier->deleteLater();
        m_notifier = nullptr;
    }

    if (m_deviceFd >= 0) {
        ::close(m_deviceFd);
        m_deviceFd = -1;
    }

    setPressed(false);
    setAvailable(false);
    setDevicePath(QString());
}

void PowerButtonBridge::setAvailable(bool available)
{
    if (m_available == available)
        return;

    m_available = available;
    emit availableChanged();
}

void PowerButtonBridge::setPressed(bool pressed)
{
    if (m_pressed == pressed)
        return;

    m_pressed = pressed;
    emit pressedChanged();
}

void PowerButtonBridge::setDevicePath(const QString &path)
{
    if (m_devicePath == path)
        return;

    m_devicePath = path;
    emit devicePathChanged();
}
