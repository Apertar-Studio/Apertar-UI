#pragma once

#include <QEvent>
#include <QObject>
#include <QTimer>

class SleepManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString sleepMode READ sleepMode WRITE setSleepMode NOTIFY sleepModeChanged)

public:
    explicit SleepManager(QObject *parent = nullptr);

    QString sleepMode() const;
    void setSleepMode(const QString &mode);
    Q_INVOKABLE void restartIdleTimerNow();

signals:
    void sleepModeChanged();
    void sleepTriggered();
    void activityDetected();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool isActivityEvent(QEvent::Type type) const;
    int timeoutMsForMode(const QString &mode) const;
    void resetIdleTimer();

    QTimer m_idleTimer;
    QString m_sleepMode = QStringLiteral("Off");
};
