#include "SleepManager.hpp"

#include <QCoreApplication>
#include <QEvent>

SleepManager::SleepManager(QObject *parent)
    : QObject(parent)
{
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

    if (isActivityEvent(type)) {
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
    const int timeoutMs = timeoutMsForMode(m_sleepMode);
    if (timeoutMs <= 0)
        return;

    m_idleTimer.start(timeoutMs);
}
