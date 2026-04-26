#include "SystemActionBridge.hpp"

#include <QDate>
#include <QDateTime>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusReply>
#include <cstring>
#include <QFile>
#include <QProcess>
#include <QTemporaryFile>
#include <QTime>
#include <QTimeZone>

namespace {
constexpr auto kFanBlockStart = "# BEGIN APERTAR FAN MODE";
constexpr auto kFanBlockEnd = "# END APERTAR FAN MODE";
constexpr auto kBootConfigPath = "/boot/firmware/config.txt";

QString fanModeBlock(const QString &mode)
{
    if (mode == QStringLiteral("Auto"))
        return QString();

    QStringList lines;
    lines << QString::fromLatin1(kFanBlockStart);
    lines << QStringLiteral("# mode: %1").arg(mode);
    lines << QStringLiteral("dtparam=cooling_fan=on");

    if (mode == QStringLiteral("Silent")) {
        lines << QStringLiteral("dtparam=fan_temp0=60000");
        lines << QStringLiteral("dtparam=fan_temp0_hyst=5000");
        lines << QStringLiteral("dtparam=fan_temp0_speed=90");
        lines << QStringLiteral("dtparam=fan_temp1=70000");
        lines << QStringLiteral("dtparam=fan_temp1_hyst=5000");
        lines << QStringLiteral("dtparam=fan_temp1_speed=140");
        lines << QStringLiteral("dtparam=fan_temp2=77500");
        lines << QStringLiteral("dtparam=fan_temp2_hyst=4000");
        lines << QStringLiteral("dtparam=fan_temp2_speed=190");
        lines << QStringLiteral("dtparam=fan_temp3=82000");
        lines << QStringLiteral("dtparam=fan_temp3_hyst=3000");
        lines << QStringLiteral("dtparam=fan_temp3_speed=255");
    } else if (mode == QStringLiteral("Full Blast")) {
        lines << QStringLiteral("dtparam=fan_temp0=0");
        lines << QStringLiteral("dtparam=fan_temp0_hyst=0");
        lines << QStringLiteral("dtparam=fan_temp0_speed=255");
        lines << QStringLiteral("dtparam=fan_temp1=1000");
        lines << QStringLiteral("dtparam=fan_temp1_hyst=0");
        lines << QStringLiteral("dtparam=fan_temp1_speed=255");
        lines << QStringLiteral("dtparam=fan_temp2=2000");
        lines << QStringLiteral("dtparam=fan_temp2_hyst=0");
        lines << QStringLiteral("dtparam=fan_temp2_speed=255");
        lines << QStringLiteral("dtparam=fan_temp3=3000");
        lines << QStringLiteral("dtparam=fan_temp3_hyst=0");
        lines << QStringLiteral("dtparam=fan_temp3_speed=255");
    }

    lines << QString::fromLatin1(kFanBlockEnd);
    return lines.join('\n') + QLatin1Char('\n');
}

QString stripManagedFanBlock(QString text)
{
    const int start = text.indexOf(QString::fromLatin1(kFanBlockStart));
    if (start < 0)
        return text;

    int end = text.indexOf(QString::fromLatin1(kFanBlockEnd), start);
    if (end < 0)
        return text;

    end += int(strlen(kFanBlockEnd));
    while (end < text.size() && (text.at(end) == QLatin1Char('\n') || text.at(end) == QLatin1Char('\r')))
        ++end;

    text.remove(start, end - start);
    while (text.contains(QStringLiteral("\n\n\n")))
        text.replace(QStringLiteral("\n\n\n"), QStringLiteral("\n\n"));
    return text;
}

QString detectManagedFanMode(const QString &text)
{
    const int start = text.indexOf(QString::fromLatin1(kFanBlockStart));
    if (start < 0)
        return QStringLiteral("Auto");

    const int end = text.indexOf(QString::fromLatin1(kFanBlockEnd), start);
    const QString block = end > start ? text.mid(start, end - start) : text.mid(start);

    if (block.contains(QStringLiteral("# mode: Silent")))
        return QStringLiteral("Silent");
    if (block.contains(QStringLiteral("# mode: Full Blast")))
        return QStringLiteral("Full Blast");
    return QStringLiteral("Auto");
}
}

SystemActionBridge::SystemActionBridge(QObject *parent)
    : QObject(parent)
{
    const QList<QByteArray> zoneIds = QTimeZone::availableTimeZoneIds();
    m_availableTimeZones.reserve(zoneIds.size());
    for (const QByteArray &zoneId : zoneIds)
        m_availableTimeZones.append(QString::fromUtf8(zoneId));
    m_availableTimeZones.sort(Qt::CaseInsensitive);

    const QByteArray systemZoneId = QTimeZone::systemTimeZoneId();
    setCurrentTimeZone(systemZoneId.isEmpty() ? QStringLiteral("UTC")
                                              : QString::fromUtf8(systemZoneId));
    refreshFanMode();
}

QString SystemActionBridge::lastError() const
{
    return m_lastError;
}

QString SystemActionBridge::currentTimeZone() const
{
    return m_currentTimeZone;
}

QStringList SystemActionBridge::availableTimeZones() const
{
    return m_availableTimeZones;
}

QString SystemActionBridge::fanMode() const
{
    return m_fanMode;
}

bool SystemActionBridge::restartCamera()
{
    return invokePowerAction(QStringLiteral("Reboot"), {QStringLiteral("reboot")});
}

bool SystemActionBridge::shutdownCamera()
{
    return invokePowerAction(QStringLiteral("PowerOff"), {QStringLiteral("poweroff")});
}

bool SystemActionBridge::applyDateTime(int year,
                                       int month,
                                       int day,
                                       int hour,
                                       int minute,
                                       const QString &timeZone)
{
    setLastError(QString());

    const QDate date(year, month, day);
    const QTime time(hour, minute, 0);
    if (!date.isValid() || !time.isValid()) {
        setLastError(QStringLiteral("The selected date or time is invalid."));
        return false;
    }

    QTimeZone zone(timeZone.toUtf8());
    if (!zone.isValid())
        zone = QTimeZone::systemTimeZone();
    if (!zone.isValid())
        zone = QTimeZone::utc();

    const QString zoneName = QString::fromUtf8(zone.id());
    const QDateTime zonedDateTime(date, time, zone);
    const QString localDateTimeString = zonedDateTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    const qlonglong usecUtc = static_cast<qlonglong>(zonedDateTime.toMSecsSinceEpoch()) * 1000;

    QString errorMessage;

    if (!invokeTimedatedCall(QStringLiteral("SetNTP"),
                             {QVariant::fromValue(false), QVariant::fromValue(false)},
                             &errorMessage)) {
        if (!invokePrivilegedCommand(QStringLiteral("timedatectl"),
                                     {QStringLiteral("set-ntp"), QStringLiteral("false")},
                                     &errorMessage)) {
            if (errorMessage.isEmpty())
                errorMessage = QStringLiteral("Could not disable automatic network time.");
            setLastError(errorMessage);
            return false;
        }
    }

    errorMessage.clear();
    if (!invokeTimedatedCall(QStringLiteral("SetTimezone"),
                             {QVariant::fromValue(zoneName), QVariant::fromValue(false)},
                             &errorMessage)) {
        if (!invokePrivilegedCommand(QStringLiteral("timedatectl"),
                                     {QStringLiteral("set-timezone"), zoneName},
                                     &errorMessage)) {
            if (errorMessage.isEmpty())
                errorMessage = QStringLiteral("Could not update the system timezone.");
            setLastError(errorMessage);
            return false;
        }
    }

    errorMessage.clear();
    if (!invokeTimedatedCall(QStringLiteral("SetTime"),
                             {QVariant::fromValue(usecUtc),
                              QVariant::fromValue(false),
                              QVariant::fromValue(false)},
                             &errorMessage)) {
        if (!invokePrivilegedCommand(QStringLiteral("timedatectl"),
                                     {QStringLiteral("set-time"), localDateTimeString},
                                     &errorMessage)) {
            if (errorMessage.isEmpty())
                errorMessage = QStringLiteral("Could not update the system clock.");
            setLastError(errorMessage);
            return false;
        }
    }

    errorMessage.clear();
    if (!invokePrivilegedCommand(QStringLiteral("hwclock"),
                                 {QStringLiteral("--systohc"), QStringLiteral("--utc")},
                                 &errorMessage)) {
        if (errorMessage.isEmpty())
            errorMessage = QStringLiteral("The system time changed, but writing it to the RTC failed.");
        setLastError(errorMessage);
        return false;
    }

    setCurrentTimeZone(zoneName);
    return true;
}

bool SystemActionBridge::applyFanMode(const QString &mode)
{
    setLastError(QString());

    const QString normalizedMode = mode.trimmed();
    if (normalizedMode != QStringLiteral("Silent")
        && normalizedMode != QStringLiteral("Auto")
        && normalizedMode != QStringLiteral("Full Blast")) {
        setLastError(QStringLiteral("Unknown fan mode selected."));
        return false;
    }

    QFile configFile(QString::fromLatin1(kBootConfigPath));
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setLastError(QStringLiteral("Could not read /boot/firmware/config.txt."));
        return false;
    }

    QString configText = QString::fromUtf8(configFile.readAll());
    configFile.close();

    configText = stripManagedFanBlock(configText).trimmed();
    if (!configText.isEmpty())
        configText.append(QLatin1String("\n\n"));
    configText.append(fanModeBlock(normalizedMode));

    QTemporaryFile tempFile;
    tempFile.setAutoRemove(false);
    if (!tempFile.open()) {
        setLastError(QStringLiteral("Could not prepare the fan mode update."));
        return false;
    }

    tempFile.write(configText.toUtf8());
    tempFile.flush();
    tempFile.close();

    QString errorMessage;
    const bool copied = invokePrivilegedCommand(QStringLiteral("install"),
                                                {QStringLiteral("-m"),
                                                 QStringLiteral("644"),
                                                 tempFile.fileName(),
                                                 QString::fromLatin1(kBootConfigPath)},
                                                &errorMessage);
    QFile::remove(tempFile.fileName());

    if (!copied) {
        if (errorMessage.contains(QStringLiteral("password"), Qt::CaseInsensitive)
            || errorMessage.contains(QStringLiteral("permission denied"), Qt::CaseInsensitive)
            || errorMessage.contains(QStringLiteral("not allowed"), Qt::CaseInsensitive)) {
            errorMessage = QStringLiteral("Changing the fan mode needs sudo permission.");
        }

        if (errorMessage.isEmpty())
            errorMessage = QStringLiteral("Could not update the fan mode configuration.");
        setLastError(errorMessage);
        return false;
    }

    setFanMode(normalizedMode);
    return true;
}

bool SystemActionBridge::invokePowerAction(const QString &dbusMethod, const QStringList &systemctlArgs)
{
    setLastError(QString());

    QDBusInterface loginManager(QStringLiteral("org.freedesktop.login1"),
                                QStringLiteral("/org/freedesktop/login1"),
                                QStringLiteral("org.freedesktop.login1.Manager"),
                                QDBusConnection::systemBus());

    if (loginManager.isValid()) {
        QDBusReply<void> reply = loginManager.call(dbusMethod, false);
        if (reply.isValid()) {
            return true;
        }
    }

    QString errorMessage;
    if (invokeSystemctl(systemctlArgs, &errorMessage)) {
        return true;
    }

    if (errorMessage.isEmpty() && loginManager.isValid()) {
        errorMessage = loginManager.lastError().message();
    }

    if (errorMessage.isEmpty()) {
        errorMessage = QStringLiteral("Permission denied or system power control is unavailable.");
    }

    setLastError(errorMessage);
    return false;
}

bool SystemActionBridge::invokeSystemctl(const QStringList &arguments, QString *errorMessage)
{
    QProcess process;
    process.start(QStringLiteral("systemctl"), arguments);
    if (!process.waitForStarted(1500)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Could not start systemctl.");
        }
        return false;
    }

    if (!process.waitForFinished(3000)) {
        return true;
    }

    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        return true;
    }

    if (errorMessage) {
        const QString stdErr = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
        const QString stdOut = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
        *errorMessage = !stdErr.isEmpty()
                        ? stdErr
                        : (!stdOut.isEmpty() ? stdOut
                                             : QStringLiteral("systemctl rejected the power action."));
    }
    return false;
}

bool SystemActionBridge::invokeTimedatedCall(const QString &method,
                                             const QList<QVariant> &arguments,
                                             QString *errorMessage)
{
    QDBusInterface timedated(QStringLiteral("org.freedesktop.timedate1"),
                             QStringLiteral("/org/freedesktop/timedate1"),
                             QStringLiteral("org.freedesktop.timedate1"),
                             QDBusConnection::systemBus());

    if (!timedated.isValid()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Could not connect to the system time service.");
        return false;
    }

    const QDBusMessage reply = timedated.callWithArgumentList(QDBus::Block, method, arguments);
    if (reply.type() != QDBusMessage::ErrorMessage)
        return true;

    if (errorMessage)
        *errorMessage = reply.errorMessage();
    return false;
}

bool SystemActionBridge::invokePrivilegedCommand(const QString &program,
                                                 const QStringList &arguments,
                                                 QString *errorMessage)
{
    QProcess process;
    QStringList sudoArguments;
    sudoArguments << QStringLiteral("-n") << program;
    sudoArguments << arguments;

    process.start(QStringLiteral("sudo"), sudoArguments);
    if (!process.waitForStarted(1500)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Could not start sudo for %1.").arg(program);
        return false;
    }

    if (!process.waitForFinished(5000)) {
        process.terminate();
        if (!process.waitForFinished(1000)) {
            process.kill();
            process.waitForFinished(1000);
        }

        if (errorMessage)
            *errorMessage = QStringLiteral("%1 did not finish in time.").arg(program);
        return false;
    }

    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0)
        return true;

    if (errorMessage) {
        const QString stdErr = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
        const QString stdOut = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
        *errorMessage = !stdErr.isEmpty()
                        ? stdErr
                        : (!stdOut.isEmpty() ? stdOut
                                             : QStringLiteral("%1 failed.").arg(program));
    }
    return false;
}

void SystemActionBridge::refreshFanMode()
{
    QFile configFile(QString::fromLatin1(kBootConfigPath));
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setFanMode(QStringLiteral("Auto"));
        return;
    }

    const QString configText = QString::fromUtf8(configFile.readAll());
    configFile.close();
    setFanMode(detectManagedFanMode(configText));
}

void SystemActionBridge::setLastError(const QString &lastError)
{
    if (m_lastError == lastError) {
        return;
    }

    m_lastError = lastError;
    emit lastErrorChanged();
}

void SystemActionBridge::setCurrentTimeZone(const QString &timeZone)
{
    if (m_currentTimeZone == timeZone)
        return;

    m_currentTimeZone = timeZone;
    emit currentTimeZoneChanged();
}

void SystemActionBridge::setFanMode(const QString &mode)
{
    if (m_fanMode == mode)
        return;

    m_fanMode = mode;
    emit fanModeChanged();
}
