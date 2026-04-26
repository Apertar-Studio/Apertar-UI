#pragma once

#include <QObject>
#include <QStringList>
#include <QString>

class SystemActionBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString currentTimeZone READ currentTimeZone NOTIFY currentTimeZoneChanged)
    Q_PROPERTY(QStringList availableTimeZones READ availableTimeZones CONSTANT)
    Q_PROPERTY(QString fanMode READ fanMode NOTIFY fanModeChanged)

public:
    explicit SystemActionBridge(QObject *parent = nullptr);

    QString lastError() const;
    QString currentTimeZone() const;
    QStringList availableTimeZones() const;
    QString fanMode() const;

    Q_INVOKABLE bool restartCamera();
    Q_INVOKABLE bool shutdownCamera();
    Q_INVOKABLE bool applyDateTime(int year, int month, int day, int hour, int minute, const QString &timeZone);
    Q_INVOKABLE bool applyFanMode(const QString &mode);

signals:
    void lastErrorChanged();
    void currentTimeZoneChanged();
    void fanModeChanged();

private:
    bool invokePowerAction(const QString &dbusMethod, const QStringList &systemctlArgs);
    bool invokeSystemctl(const QStringList &arguments, QString *errorMessage);
    bool invokeTimedatedCall(const QString &method, const QList<QVariant> &arguments, QString *errorMessage);
    bool invokePrivilegedCommand(const QString &program, const QStringList &arguments, QString *errorMessage);
    void refreshFanMode();
    void setLastError(const QString &lastError);
    void setCurrentTimeZone(const QString &timeZone);
    void setFanMode(const QString &mode);

    QString m_lastError;
    QString m_currentTimeZone;
    QStringList m_availableTimeZones;
    QString m_fanMode;
};
