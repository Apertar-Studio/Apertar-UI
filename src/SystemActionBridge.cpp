#include "SystemActionBridge.hpp"

#include <QDate>
#include <QDateTime>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDir>
#include <QElapsedTimer>
#include <cstring>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QThread>
#include <QTemporaryFile>
#include <QTime>
#include <QTimeZone>

namespace {
constexpr auto kFanBlockStart = "# BEGIN APERTAR FAN MODE";
constexpr auto kFanBlockEnd = "# END APERTAR FAN MODE";
constexpr auto kBootConfigPath = "/boot/firmware/config.txt";

QString resolveExecutablePath(const QStringList &candidates, const QString &fallbackName)
{
    for (const QString &candidate : candidates) {
        const QFileInfo info(candidate);
        if (info.exists() && info.isExecutable())
            return info.absoluteFilePath();
    }

    const QString discovered = QStandardPaths::findExecutable(fallbackName);
    if (!discovered.isEmpty())
        return discovered;

    return fallbackName;
}

QString timedatectlProgram()
{
    static const QString path = resolveExecutablePath(
        {QStringLiteral("/usr/bin/timedatectl"),
         QStringLiteral("/bin/timedatectl"),
         QStringLiteral("/usr/sbin/timedatectl"),
         QStringLiteral("/sbin/timedatectl")},
        QStringLiteral("timedatectl"));
    return path;
}

QString hwclockProgram()
{
    static const QString path = resolveExecutablePath(
        {QStringLiteral("/usr/sbin/hwclock"),
         QStringLiteral("/sbin/hwclock"),
         QStringLiteral("/usr/bin/hwclock"),
         QStringLiteral("/bin/hwclock")},
        QStringLiteral("hwclock"));
    return path;
}

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

QVariant timedatedPropertyValue(const QString &propertyName)
{
    QDBusInterface properties(QStringLiteral("org.freedesktop.timedate1"),
                              QStringLiteral("/org/freedesktop/timedate1"),
                              QStringLiteral("org.freedesktop.DBus.Properties"),
                              QDBusConnection::systemBus());
    if (!properties.isValid())
        return {};

    const QDBusMessage reply = properties.call(QStringLiteral("Get"),
                                               QStringLiteral("org.freedesktop.timedate1"),
                                               propertyName);
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty())
        return {};

    const QDBusVariant boxed = qvariant_cast<QDBusVariant>(reply.arguments().constFirst());
    return boxed.variant();
}

bool timedatedBoolProperty(const QString &propertyName, bool *ok = nullptr)
{
    if (ok)
        *ok = false;

    const QVariant value = timedatedPropertyValue(propertyName);
    if (!value.isValid())
        return false;

    if (value.metaType().id() == QMetaType::Bool) {
        if (ok)
            *ok = true;
        return value.toBool();
    }

    const QString normalized = value.toString().trimmed().toLower();
    if (normalized == QStringLiteral("true")
        || normalized == QStringLiteral("yes")
        || normalized == QStringLiteral("1")) {
        if (ok)
            *ok = true;
        return true;
    }

    if (normalized == QStringLiteral("false")
        || normalized == QStringLiteral("no")
        || normalized == QStringLiteral("0")) {
        if (ok)
            *ok = true;
        return false;
    }

    return false;
}

bool waitForTimedatedNetworkSync(int timeoutMs)
{
    bool canNtpKnown = false;
    const bool canNtp = timedatedBoolProperty(QStringLiteral("CanNTP"), &canNtpKnown);
    if (canNtpKnown && !canNtp)
        return false;

    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < timeoutMs) {
        bool syncKnown = false;
        const bool synced = timedatedBoolProperty(QStringLiteral("NTPSynchronized"), &syncKnown);
        if (syncKnown && synced)
            return true;
        QThread::msleep(250);
    }

    bool syncKnown = false;
    const bool synced = timedatedBoolProperty(QStringLiteral("NTPSynchronized"), &syncKnown);
    return syncKnown && synced;
}

bool hardwareClockDeviceAvailable()
{
    if (QFileInfo::exists(QStringLiteral("/dev/rtc"))
        || QFileInfo::exists(QStringLiteral("/dev/rtc0")))
        return true;

    QDir rtcClassDir(QStringLiteral("/sys/class/rtc"));
    return !rtcClassDir.entryList({QStringLiteral("rtc*")},
                                  QDir::Dirs | QDir::NoDotAndDotDot).isEmpty();
}

bool isMissingHardwareClockError(const QString &errorMessage)
{
    const QString lower = errorMessage.toLower();
    return lower.contains(QStringLiteral("cannot access the hardware clock via any known method"))
        || lower.contains(QStringLiteral("no usable clock interface found"))
        || lower.contains(QStringLiteral("no such file or directory"));
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

bool SystemActionBridge::applyTimeZone(const QString &timeZone)
{
    setLastError(QString());

    QTimeZone zone(timeZone.toUtf8());
    if (!zone.isValid())
        zone = QTimeZone::systemTimeZone();
    if (!zone.isValid())
        zone = QTimeZone::utc();

    const QString zoneName = QString::fromUtf8(zone.id());
    QString errorMessage;

    if (!invokeTimedatedCall(QStringLiteral("SetTimezone"),
                             {QVariant::fromValue(zoneName), QVariant::fromValue(false)},
                             &errorMessage)) {
        if (!invokePrivilegedCommand(timedatectlProgram(),
                                     {QStringLiteral("set-timezone"), zoneName},
                                     &errorMessage)) {
            if (errorMessage.isEmpty())
                errorMessage = QStringLiteral("Could not update the system timezone.");
            setLastError(errorMessage);
            return false;
        }
    }

    errorMessage.clear();
    if (!invokeTimedatedCall(QStringLiteral("SetNTP"),
                             {QVariant::fromValue(true), QVariant::fromValue(false)},
                             &errorMessage)) {
        errorMessage.clear();
        invokePrivilegedCommand(timedatectlProgram(),
                                {QStringLiteral("set-ntp"), QStringLiteral("true")},
                                &errorMessage);
    }

    const bool networkSynced = waitForTimedatedNetworkSync(2500);
    if (networkSynced) {
        errorMessage.clear();
        if (!syncHardwareClockIfAvailable(&errorMessage)) {
            if (errorMessage.isEmpty())
                errorMessage = QStringLiteral("The system clock synced, but writing it to the RTC failed.");
            setLastError(errorMessage);
            return false;
        }
    }

    setCurrentTimeZone(zoneName);
    return true;
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
        if (!invokePrivilegedCommand(timedatectlProgram(),
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
        if (!invokePrivilegedCommand(timedatectlProgram(),
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
        if (!invokePrivilegedCommand(timedatectlProgram(),
                                     {QStringLiteral("set-time"), localDateTimeString},
                                     &errorMessage)) {
            if (errorMessage.isEmpty())
                errorMessage = QStringLiteral("Could not update the system clock.");
            setLastError(errorMessage);
            return false;
        }
    }

    errorMessage.clear();
    if (!syncHardwareClockIfAvailable(&errorMessage)) {
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

    QString loginManagerError;
    QDBusInterface loginManager(QStringLiteral("org.freedesktop.login1"),
                                QStringLiteral("/org/freedesktop/login1"),
                                QStringLiteral("org.freedesktop.login1.Manager"),
                                QDBusConnection::systemBus());

    if (loginManager.isValid()) {
        QDBusReply<void> reply = loginManager.call(dbusMethod, false);
        if (reply.isValid()) {
            return true;
        }
        loginManagerError = reply.error().message();
    }

    QStringList nonInteractiveSystemctlArgs;
    nonInteractiveSystemctlArgs << QStringLiteral("--no-ask-password");
    nonInteractiveSystemctlArgs << systemctlArgs;

    QString errorMessage;
    if (invokePrivilegedCommand(QStringLiteral("systemctl"),
                                nonInteractiveSystemctlArgs,
                                &errorMessage)) {
        return true;
    }

    if (invokeSystemctl(nonInteractiveSystemctlArgs, &errorMessage)) {
        return true;
    }

    if (errorMessage.isEmpty()) {
        if (!loginManagerError.isEmpty())
            errorMessage = loginManagerError;
        else if (loginManager.isValid())
            errorMessage = loginManager.lastError().message();
    }

    if (errorMessage.isEmpty())
        errorMessage = QStringLiteral("Permission denied or system power control is unavailable.");

    setLastError(normalizedPowerError(errorMessage));
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

    if (!process.waitForFinished(4000)) {
        process.terminate();
        if (!process.waitForFinished(1000)) {
            process.kill();
            process.waitForFinished(1000);
        }

        if (errorMessage)
            *errorMessage = QStringLiteral("systemctl did not finish in time.");
        return false;
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

QString SystemActionBridge::normalizedPowerError(const QString &errorMessage) const
{
    const QString normalized = errorMessage.trimmed();
    const QString lower = normalized.toLower();

    if (lower.contains(QStringLiteral("multiple-sessions"))
        || lower.contains(QStringLiteral("authentication is required"))
        || lower.contains(QStringLiteral("interactive authentication required"))
        || lower.contains(QStringLiteral("access denied"))
        || lower.contains(QStringLiteral("a password is required"))
        || lower.contains(QStringLiteral("sudo")))
        return QStringLiteral("Shutdown needs a passwordless sudo rule for systemctl or a polkit rule that allows power off with multiple sessions.");

    if (normalized.isEmpty())
        return QStringLiteral("Permission denied or system power control is unavailable.");

    return normalized;
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

bool SystemActionBridge::syncHardwareClockIfAvailable(QString *errorMessage)
{
    if (!hardwareClockDeviceAvailable())
        return true;

    QString hwclockError;
    if (invokePrivilegedCommand(hwclockProgram(),
                                {QStringLiteral("--systohc"), QStringLiteral("--utc")},
                                &hwclockError)) {
        return true;
    }

    if (isMissingHardwareClockError(hwclockError))
        return true;

    if (errorMessage)
        *errorMessage = hwclockError;
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
