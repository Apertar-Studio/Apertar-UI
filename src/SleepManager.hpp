#pragma once

#include <QEvent>
#include <QElapsedTimer>
#include <QMap>
#include <QObject>
#include <QTimer>

class SleepManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString sleepMode READ sleepMode WRITE setSleepMode NOTIFY sleepModeChanged)
    Q_PROPERTY(bool standby READ standby WRITE setStandby NOTIFY standbyChanged)

public:
    explicit SleepManager(QObject *parent = nullptr);
    ~SleepManager() override;
    bool standby() const { return m_standby; }
    void setStandby(bool standby);

    QString sleepMode() const;
    void setSleepMode(const QString &mode);
    Q_INVOKABLE void restartIdleTimerNow();

signals:
    void sleepModeChanged();
    void sleepTriggered();
    void activityDetected();
    void standbyChanged();
    void userActivity();
    void wakeRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool isActivityEvent(QEvent::Type type) const;
    int timeoutMsForMode(const QString &mode) const;
    void resetIdleTimer();
    void restoreBacklights();

    QTimer m_idleTimer;
    QString m_sleepMode = QStringLiteral("Off");
    bool m_standby = false;
    bool m_consumeGesture = false;
    QElapsedTimer m_activityThrottle;
    QMap<QString, QByteArray> m_backlightBrightness;
};
