#include "SleepManager.hpp"

#include <QCoreApplication>
#include <QEvent>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QDebug>
#include <QtEndian>

namespace {
QByteArray readValue(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll().trimmed() : QByteArray();
}

bool writeValue(const QString &path, const QByteArray &value)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning().noquote() << "Apertar standby: cannot write" << path << file.errorString();
        return false;
    }
    const QByteArray line = value.trimmed() + '\n';
    return file.write(line) == line.size() && file.flush();
}

QString panelLedPath()
{
    const QString overridePath = qEnvironmentVariable("APERTAR_PANEL_LED_PATH");
    if (!overridePath.isEmpty())
        return overridePath;

    // Waveshare's square DPI overlay exposes its active-low GPIO18 backlight
    // as an LED, not a backlight. Never switch arbitrary status/power LEDs.
    const QString led = QStringLiteral("/sys/class/leds/wittypi_led");
    const QByteArray gpios = readValue(led + "/device/of_node/wittypi_led/gpios");
    const QByteArray width = readValue("/proc/device-tree/panel/panel-timing/hactive");
    const QByteArray height = readValue("/proc/device-tree/panel/panel-timing/vactive");
    if (gpios.size() == 12 && width.size() == 4 && height.size() == 4 &&
        qFromBigEndian<quint32>(gpios.constData() + 4) == 18 &&
        qFromBigEndian<quint32>(gpios.constData() + 8) == 1 &&
        qFromBigEndian<quint32>(width.constData()) == 720 &&
        qFromBigEndian<quint32>(height.constData()) == 720 &&
        readValue("/proc/device-tree/panel/compatible").startsWith("panel-dpi"))
        return led;
    return {};
}
}

SleepManager::SleepManager(QObject *parent)
    : QObject(parent)
{
    // Recover backlight levels if the previous UI was terminated during standby.
    QSettings saved(QStringLiteral("Apertar"), QStringLiteral("StandbyBacklights"));
    for (const QString &key : saved.childKeys()) {
        const QString path = QString::fromUtf8(QByteArray::fromBase64(key.toLatin1(), QByteArray::Base64UrlEncoding));
        m_backlightBrightness.insert(path, saved.value(key).toByteArray());
    }
    restoreBacklights();
    m_idleTimer.setSingleShot(true);
    connect(&m_idleTimer, &QTimer::timeout, this, [this]() {
        if (timeoutMsForMode(m_sleepMode) > 0)
            emit sleepTriggered();
    });

    if (QCoreApplication::instance())
        QCoreApplication::instance()->installEventFilter(this);
}

QString SleepManager::sleepMode() const
{
    return m_sleepMode;
}

SleepManager::~SleepManager()
{
    restoreBacklights();
}

void SleepManager::restoreBacklights()
{
    QSettings saved(QStringLiteral("Apertar"), QStringLiteral("StandbyBacklights"));
    for (auto it = m_backlightBrightness.constBegin(); it != m_backlightBrightness.constEnd(); ++it) {
        if (writeValue(it.key(), it.value())) {
            saved.remove(QString::fromLatin1(it.key().toUtf8().toBase64(
                QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
        }
    }
    saved.sync();
    m_backlightBrightness.clear();
}

void SleepManager::setStandby(bool standby)
{
    if (m_standby == standby)
        return;
    m_standby = standby;
    if (standby) {
        m_idleTimer.stop();
        const QDir backlights(qEnvironmentVariable("APERTAR_BACKLIGHT_DIR",
                                                   QStringLiteral("/sys/class/backlight")));
        QStringList paths;
        for (const QString &name : backlights.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            paths.append(backlights.filePath(name));
        }
        const QString led = panelLedPath();
        if (!led.isEmpty())
            paths.append(led);
        for (const QString &device : paths) {
            const QString path = device + QStringLiteral("/brightness");
            QByteArray brightness = readValue(path);
            if (brightness.isEmpty())
                continue;
            // gpio-leds can report the trigger's 255 even with max_brightness=1.
            bool validBrightness = false, validMaximum = false;
            const int level = brightness.toInt(&validBrightness);
            const int maximum = readValue(device + "/max_brightness").toInt(&validMaximum);
            if (!validBrightness || !validMaximum || maximum <= 0)
                continue;
            brightness = QByteArray::number(qBound(0, level, maximum));
            QSettings saved(QStringLiteral("Apertar"), QStringLiteral("StandbyBacklights"));
            auto remember = [&](const QString &valuePath, const QByteArray &value) {
                saved.setValue(QString::fromLatin1(valuePath.toUtf8().toBase64(
                    QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)), value);
                m_backlightBrightness.insert(valuePath, value);
            };
            const QString triggerPath = device + QStringLiteral("/trigger");
            const QByteArray triggers = readValue(triggerPath);
            const int start = triggers.indexOf('['), end = triggers.indexOf(']');
            if (start >= 0 && end > start) {
                const QByteArray trigger = triggers.mid(start + 1, end - start - 1);
                remember(triggerPath, trigger);
                saved.sync();
                if (!writeValue(triggerPath, "none"))
                    continue;
            }
            remember(path, brightness);
            saved.sync();
            if (writeValue(path, "0"))
                qInfo().noquote() << "Apertar standby: backlight off:" << device;
            else
                qWarning().noquote() << "Apertar standby: display blanked, but backlight control failed:" << device;
        }
    } else {
        restoreBacklights();
        resetIdleTimer();
    }
    qInfo() << "Apertar standby: UI" << (standby ? "asleep" : "awake");
    emit standbyChanged();
}

void SleepManager::setSleepMode(const QString &mode)
{
    if (m_sleepMode == mode)
        return;

    m_sleepMode = mode;
    emit sleepModeChanged();

    const int timeoutMs = timeoutMsForMode(m_sleepMode);
    if (timeoutMs <= 0) {
        m_idleTimer.stop();
        return;
    }

    resetIdleTimer();
}

void SleepManager::restartIdleTimerNow()
{
    resetIdleTimer();
}

bool SleepManager::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);

    if (!event)
        return false;

    const QEvent::Type type = event->type();

    if (m_standby && isActivityEvent(type)) {
        // Only dedicated hardware inputs wake the camera. Remember a touch/key
        // still held during hardware wake so its release cannot activate the UI.
        if (type == QEvent::TouchBegin || type == QEvent::MouseButtonPress || type == QEvent::KeyPress)
            m_consumeGesture = true;
        else if (type == QEvent::TouchEnd || type == QEvent::TouchCancel ||
                 type == QEvent::MouseButtonRelease || type == QEvent::KeyRelease)
            m_consumeGesture = false;
        return true;
    }
    if (m_consumeGesture && isActivityEvent(type)) {
        if (type == QEvent::TouchEnd || type == QEvent::TouchCancel ||
            type == QEvent::MouseButtonRelease || type == QEvent::KeyRelease)
            m_consumeGesture = false;
        return true;
    }
    if (isActivityEvent(type)) {
        if (!m_activityThrottle.isValid() || m_activityThrottle.elapsed() >= 1000) {
            m_activityThrottle.restart();
            emit userActivity();
        }
        if (timeoutMsForMode(m_sleepMode) > 0)
            resetIdleTimer();
        emit activityDetected();
    }

    return false;
}
bool SleepManager::isActivityEvent(QEvent::Type type) const
{
    switch (type) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::Wheel:
        return true;
    default:
        return false;
    }
}

int SleepManager::timeoutMsForMode(const QString &mode) const
{
    if (mode == QStringLiteral("1 min"))
        return 1 * 60 * 1000;
    if (mode == QStringLiteral("2 min"))
        return 2 * 60 * 1000;
    if (mode == QStringLiteral("5 min"))
        return 5 * 60 * 1000;
    if (mode == QStringLiteral("10 min"))
        return 10 * 60 * 1000;
    return 0;
}

void SleepManager::resetIdleTimer()
{
    if (m_standby) {
        m_idleTimer.stop();
        return;
    }
    const int timeoutMs = timeoutMsForMode(m_sleepMode);
    if (timeoutMs <= 0)
        return;

    m_idleTimer.start(timeoutMs);
}
