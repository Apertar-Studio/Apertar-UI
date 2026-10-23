#include "GyroConfigBridge.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextStream>
#include <QtGlobal>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {

constexpr quint8 kMpu6050RegAccelXoutH = 0x3b;
constexpr quint8 kMpu6050RegSampleRateDivider = 0x19;
constexpr quint8 kMpu6050RegConfig = 0x1a;
constexpr quint8 kMpu6050RegGyroConfig = 0x1b;
constexpr quint8 kMpu6050RegAccelConfig = 0x1c;
constexpr quint8 kMpu6050RegPwrMgmt1 = 0x6b;
constexpr quint8 kMpu6050RegWhoAmI = 0x75;
constexpr quint8 kBmiChipIdRegister = 0x00;
constexpr quint8 kBmi160RegGyroXoutL = 0x0c;
constexpr quint8 kBmi160RegAccelXoutL = 0x12;
constexpr quint8 kBmi160RegAccelConfig = 0x40;
constexpr quint8 kBmi160RegAccelRange = 0x41;
constexpr quint8 kBmi160RegGyroConfig = 0x42;
constexpr quint8 kBmi160RegGyroRange = 0x43;
constexpr quint8 kBmi160RegCommand = 0x7e;
constexpr quint8 kBmi160ChipId = 0xd1;
constexpr quint8 kBmi270RegAccelXoutL = 0x0c;
constexpr quint8 kBmi270RegInternalStatus = 0x21;
constexpr quint8 kBmi270RegAccelConfig = 0x40;
constexpr quint8 kBmi270RegAccelRange = 0x41;
constexpr quint8 kBmi270RegGyroConfig = 0x42;
constexpr quint8 kBmi270RegGyroRange = 0x43;
constexpr quint8 kBmi270RegPowerConf = 0x7c;
constexpr quint8 kBmi270RegPowerCtrl = 0x7d;
constexpr quint8 kBmi270ChipId = 0x24;
constexpr quint8 kIcm42688RegDeviceConfig = 0x11;
constexpr quint8 kIcm42688RegAccelXoutH = 0x1f;
constexpr quint8 kIcm42688RegIntStatus = 0x2d;
constexpr quint8 kIcm42688RegPowerMgmt0 = 0x4e;
constexpr quint8 kIcm42688RegGyroConfig0 = 0x4f;
constexpr quint8 kIcm42688RegAccelConfig0 = 0x50;
constexpr quint8 kIcm42688RegWhoAmI = 0x75;
constexpr quint8 kIcm42688RegBankSelect = 0x76;
constexpr quint8 kIcm42688ChipId = 0x47;

QString trimConfig(QString value)
{
    return value.trimmed();
}

QString stripInlineComment(QString value)
{
    const int hash = value.indexOf(QLatin1Char('#'));
    const int semi = value.indexOf(QLatin1Char(';'));
    int pos = -1;
    if (hash >= 0)
        pos = hash;
    if (semi >= 0)
        pos = pos < 0 ? semi : std::min(pos, semi);
    if (pos >= 0)
        value.truncate(pos);
    return value.trimmed();
}

QString normalizeToken(QString value)
{
    value = stripInlineComment(value).toLower();
    value.replace(QLatin1Char('-'), QLatin1Char('_'));
    value.replace(QLatin1Char(' '), QLatin1Char('_'));
    value.replace(QLatin1Char('.'), QLatin1Char('_'));
    return value;
}

QString canonicalDriverName(QString value)
{
    const QString normalized = normalizeToken(value);
    if (normalized == QStringLiteral("bmi160"))
        return QStringLiteral("bmi160");
    if (normalized == QStringLiteral("bmi270"))
        return QStringLiteral("bmi270");
    if (normalized == QStringLiteral("icm42688p") ||
        normalized == QStringLiteral("icm42688_p") ||
        normalized == QStringLiteral("icm_42688_p") ||
        normalized == QStringLiteral("icm_42688p"))
        return QStringLiteral("icm42688p");
    return QStringLiteral("mpu6050");
}

QString driverConfigSection(const QString &driver)
{
    return canonicalDriverName(driver);
}

QString driverDisplayName(const QString &driver)
{
    const QString canonical = canonicalDriverName(driver);
    if (canonical == QStringLiteral("bmi160"))
        return QStringLiteral("BMI160");
    if (canonical == QStringLiteral("bmi270"))
        return QStringLiteral("BMI270");
    if (canonical == QStringLiteral("icm42688p"))
        return QStringLiteral("ICM42688-P");
    return QStringLiteral("MPU6050");
}

bool isActiveGyroConfigSection(const QString &section, const QString &driver)
{
    const QString normalized = normalizeToken(section);
    if (normalized == QStringLiteral("gyro"))
        return true;

    // Unknown sections must not inherit the default MPU6050 driver.
    const QString activeDriver = canonicalDriverName(driver);
    if (activeDriver == QStringLiteral("icm42688p")) {
        return normalized == QStringLiteral("icm42688p") ||
               normalized == QStringLiteral("icm42688_p") ||
               normalized == QStringLiteral("icm_42688_p") ||
               normalized == QStringLiteral("icm_42688p");
    }
    return normalized == activeDriver;
}

quint8 chipIdRegisterForDriver(const QString &driver)
{
    const QString canonical = canonicalDriverName(driver);
    if (canonical == QStringLiteral("bmi160") || canonical == QStringLiteral("bmi270"))
        return kBmiChipIdRegister;
    if (canonical == QStringLiteral("icm42688p"))
        return kIcm42688RegWhoAmI;
    return kMpu6050RegWhoAmI;
}

bool chipIdMatchesDriver(const QString &driver, quint8 chipId)
{
    const QString canonical = canonicalDriverName(driver);
    if (canonical == QStringLiteral("bmi160"))
        return chipId == kBmi160ChipId;
    if (canonical == QStringLiteral("bmi270"))
        return chipId == kBmi270ChipId;
    if (canonical == QStringLiteral("icm42688p"))
        return chipId == kIcm42688ChipId;
    return (chipId & 0x7e) == 0x68 || chipId == 0x70;
}

bool readRegisterRangeFromFd(int fd, int address, quint8 reg, quint8 *data, qsizetype length)
{
    if (fd < 0 || !data || length <= 0 || length > std::numeric_limits<quint16>::max()) {
        errno = EINVAL;
        return false;
    }

    // Recording and HDMI clients share the sensor's register pointer.
    // One combined transfer prevents another reader changing it mid-sample.
    i2c_msg messages[2]{};
    messages[0].addr = static_cast<quint16>(address);
    messages[0].len = 1;
    messages[0].buf = &reg;
    messages[1].addr = messages[0].addr;
    messages[1].flags = I2C_M_RD;
    messages[1].len = static_cast<quint16>(length);
    messages[1].buf = data;
    i2c_rdwr_ioctl_data transaction{messages, 2};
    const int result = ::ioctl(fd, I2C_RDWR, &transaction);
    if (result == 2)
        return true;
    if (result >= 0)
        errno = EIO;
    return false;
}

bool readRegisterFromFd(int fd, int address, quint8 reg, quint8 *value)
{
    return readRegisterRangeFromFd(fd, address, reg, value, 1);
}

QString defaultHardwareConfigPath()
{
    return QStringLiteral("/home/pi/apertar-hardware.conf");
}

QStringList hardwareConfigCandidatePaths()
{
    QStringList candidates;
    const QString envPath = qEnvironmentVariable("APERTAR_HARDWARE_CONFIG").trimmed();
    if (!envPath.isEmpty())
        candidates.push_back(envPath);

    candidates.push_back(defaultHardwareConfigPath());
    candidates.push_back(QStringLiteral("/boot/firmware/apertar-hardware.conf"));
    candidates.push_back(QStringLiteral("/boot/apertar-hardware.conf"));
    candidates.removeDuplicates();
    return candidates;
}

bool parseConfigDouble(const QString &value, double *out)
{
    if (!out)
        return false;

    bool ok = false;
    const double parsed = stripInlineComment(value).toDouble(&ok);
    if (!ok)
        return false;

    *out = parsed;
    return true;
}

bool parseConfigInt(const QString &value, int *out)
{
    if (!out)
        return false;

    bool ok = false;
    const int parsed = stripInlineComment(value).toInt(&ok, 0);
    if (!ok)
        return false;

    *out = parsed;
    return true;
}

bool parseConfigBool(const QString &value, bool fallback)
{
    const QString normalized = normalizeToken(value);
    if (normalized == QStringLiteral("1") ||
        normalized == QStringLiteral("true") ||
        normalized == QStringLiteral("yes") ||
        normalized == QStringLiteral("on"))
        return true;

    if (normalized == QStringLiteral("0") ||
        normalized == QStringLiteral("false") ||
        normalized == QStringLiteral("no") ||
        normalized == QStringLiteral("off"))
        return false;

    return fallback;
}

QString normalizeOrientation(QString value)
{
    value = stripInlineComment(value);
    value.remove(QLatin1Char(' '));
    value.remove(QLatin1Char('\t'));
    if (value.size() != 3)
        return QString();

    bool hasX = false;
    bool hasY = false;
    bool hasZ = false;
    for (const QChar ch : value) {
        const QChar lower = ch.toLower();
        if (lower == QLatin1Char('x'))
            hasX = true;
        else if (lower == QLatin1Char('y'))
            hasY = true;
        else if (lower == QLatin1Char('z'))
            hasZ = true;
        else
            return QString();
    }

    return hasX && hasY && hasZ ? value : QString();
}

double normalizeAngleDelta(double value)
{
    while (value > 180.0)
        value -= 360.0;
    while (value < -180.0)
        value += 360.0;
    return value;
}

double accelForAxis(QChar axis, double accelXG, double accelYG, double accelZG)
{
    axis = axis.toUpper();
    if (axis == QLatin1Char('X'))
        return accelXG;
    if (axis == QLatin1Char('Y'))
        return accelYG;
    return accelZG;
}

double mappedAccelForOrientation(const QString &orientation,
                                 int index,
                                 double accelXG,
                                 double accelYG,
                                 double accelZG)
{
    const QString normalized = normalizeOrientation(orientation).isEmpty()
                                   ? QStringLiteral("XYZ")
                                   : normalizeOrientation(orientation);
    const QChar token = normalized.at(index);
    const double sign = token == token.toUpper() ? 1.0 : -1.0;
    return sign * accelForAxis(token, accelXG, accelYG, accelZG);
}

double rollDegreesForOrientation(const QString &orientation,
                                 double accelXG,
                                 double accelYG,
                                 double accelZG)
{
    const double x = mappedAccelForOrientation(orientation, 0, accelXG, accelYG, accelZG);
    const double y = mappedAccelForOrientation(orientation, 1, accelXG, accelYG, accelZG);
    return std::atan2(x, y) * 180.0 / M_PI;
}

double pitchDegreesForOrientation(const QString &orientation,
                                  double accelXG,
                                  double accelYG,
                                  double accelZG)
{
    const double x = mappedAccelForOrientation(orientation, 0, accelXG, accelYG, accelZG);
    const double y = mappedAccelForOrientation(orientation, 1, accelXG, accelYG, accelZG);
    const double z = mappedAccelForOrientation(orientation, 2, accelXG, accelYG, accelZG);
    return std::atan2(z, std::hypot(x, y)) * 180.0 / M_PI;
}

QString sectionNameFromLine(const QString &line)
{
    const QString trimmed = line.trimmed();
    if (trimmed.size() < 3 || !trimmed.startsWith(QLatin1Char('[')) || !trimmed.endsWith(QLatin1Char(']')))
        return QString();
    return normalizeToken(trimmed.mid(1, trimmed.size() - 2));
}

QStringList readLines(const QString &path)
{
    QStringList lines;
    QFile input(path);
    if (!input.open(QIODevice::ReadOnly | QIODevice::Text))
        return lines;

    QTextStream stream(&input);
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
        lines.push_back(line);
    }
    return lines;
}

bool writeLines(const QString &path, const QStringList &lines, QString *error)
{
    const QFileInfo info(path);
    QDir dir = info.dir();
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (error)
            *error = QStringLiteral("Could not create %1.").arg(dir.absolutePath());
        return false;
    }

    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = QStringLiteral("Could not write %1.").arg(path);
        return false;
    }

    QTextStream stream(&output);
    for (const QString &line : lines)
        stream << line << '\n';

    if (!output.commit()) {
        if (error)
            *error = QStringLiteral("Could not save %1.").arg(path);
        return false;
    }
    return true;
}

QString formatConfigDouble(double value)
{
    QString text = QString::number(value, 'f', 2);
    while (text.contains(QLatin1Char('.')) && text.endsWith(QLatin1Char('0')))
        text.chop(1);
    if (text.endsWith(QLatin1Char('.')))
        text.chop(1);
    return text;
}

QString formatConfigDoublePrecise(double value, int decimals)
{
    QString text = QString::number(value, 'f', decimals);
    while (text.contains(QLatin1Char('.')) && text.endsWith(QLatin1Char('0')))
        text.chop(1);
    if (text.endsWith(QLatin1Char('.')))
        text.chop(1);
    return text;
}

int normalizeAccelRangeG(int value)
{
    if (value <= 2)
        return 2;
    if (value <= 4)
        return 4;
    if (value <= 8)
        return 8;
    return 16;
}

int normalizeGyroRangeDps(int value)
{
    if (value <= 250)
        return 250;
    if (value <= 500)
        return 500;
    if (value <= 1000)
        return 1000;
    return 2000;
}

double accelLsbPerG(int rangeG)
{
    switch (normalizeAccelRangeG(rangeG)) {
    case 2:
        return 16384.0;
    case 4:
        return 8192.0;
    case 16:
        return 2048.0;
    case 8:
    default:
        return 4096.0;
    }
}

double gyroLsbPerDps(int rangeDps)
{
    switch (normalizeGyroRangeDps(rangeDps)) {
    case 250:
        return 131.0;
    case 500:
        return 65.5;
    case 2000:
        return 16.4;
    case 1000:
    default:
        return 32.8;
    }
}

quint8 mpu6050DlpfConfigForHz(int hz)
{
    if (hz <= 5)
        return 6;
    if (hz <= 10)
        return 5;
    if (hz <= 20)
        return 4;
    if (hz <= 42)
        return 3;
    if (hz <= 98)
        return 2;
    if (hz <= 188)
        return 1;
    return 0;
}

quint8 mpu6050GyroRangeBits(int rangeDps)
{
    switch (normalizeGyroRangeDps(rangeDps)) {
    case 250:
        return 0;
    case 500:
        return 1;
    case 2000:
        return 3;
    case 1000:
    default:
        return 2;
    }
}

quint8 mpu6050AccelRangeBits(int rangeG)
{
    switch (normalizeAccelRangeG(rangeG)) {
    case 2:
        return 0;
    case 4:
        return 1;
    case 16:
        return 3;
    case 8:
    default:
        return 2;
    }
}

quint8 bmiOdrCode(int hz)
{
    if (hz <= 25)
        return 0x06;
    if (hz <= 50)
        return 0x07;
    if (hz <= 100)
        return 0x08;
    if (hz <= 200)
        return 0x09;
    if (hz <= 400)
        return 0x0a;
    if (hz <= 800)
        return 0x0b;
    return 0x0c;
}

quint8 bmiGyroRangeValue(int rangeDps)
{
    switch (normalizeGyroRangeDps(rangeDps)) {
    case 250:
        return 0x03;
    case 500:
        return 0x02;
    case 2000:
        return 0x00;
    case 1000:
    default:
        return 0x01;
    }
}

quint8 bmi160AccelRangeValue(int rangeG)
{
    switch (normalizeAccelRangeG(rangeG)) {
    case 2:
        return 0x03;
    case 4:
        return 0x05;
    case 16:
        return 0x0c;
    case 8:
    default:
        return 0x08;
    }
}

quint8 bmi270AccelRangeValue(int rangeG)
{
    switch (normalizeAccelRangeG(rangeG)) {
    case 2:
        return 0x00;
    case 4:
        return 0x01;
    case 16:
        return 0x03;
    case 8:
    default:
        return 0x02;
    }
}

quint8 icm42688OdrCode(int hz)
{
    if (hz <= 25)
        return 0x0a;
    if (hz <= 50)
        return 0x09;
    if (hz <= 100)
        return 0x08;
    if (hz <= 200)
        return 0x07;
    if (hz <= 500)
        return 0x0f;
    return 0x06;
}

quint8 icm42688GyroRangeBits(int rangeDps)
{
    switch (normalizeGyroRangeDps(rangeDps)) {
    case 250:
        return 0x60;
    case 500:
        return 0x40;
    case 2000:
        return 0x00;
    case 1000:
    default:
        return 0x20;
    }
}

quint8 icm42688AccelRangeBits(int rangeG)
{
    switch (normalizeAccelRangeG(rangeG)) {
    case 2:
        return 0x60;
    case 4:
        return 0x40;
    case 16:
        return 0x00;
    case 8:
    default:
        return 0x20;
    }
}

int16_t readBigEndianInt16(const quint8 *data)
{
    const quint16 value = (static_cast<quint16>(data[0]) << 8) |
                          static_cast<quint16>(data[1]);
    return static_cast<int16_t>(value);
}

int16_t readLittleEndianInt16(const quint8 *data)
{
    const quint16 value = (static_cast<quint16>(data[1]) << 8) |
                          static_cast<quint16>(data[0]);
    return static_cast<int16_t>(value);
}

} // namespace

GyroConfigBridge::GyroConfigBridge(QObject *parent)
    : QObject(parent)
{
    m_liveTimer.setInterval(m_livePollingIntervalMs);
    m_liveTimer.setTimerType(Qt::CoarseTimer);
    connect(&m_liveTimer, &QTimer::timeout, this, &GyroConfigBridge::pollLiveSample);

    loadConfiguration();
}

GyroConfigBridge::~GyroConfigBridge()
{
    closeLiveDevice();
}

QString GyroConfigBridge::syncOffsetText() const
{
    const double rounded = std::round(m_syncOffsetMs);
    const int decimals = std::abs(m_syncOffsetMs - rounded) < 0.05 ? 0 : 1;
    const QString sign = m_syncOffsetMs > 0.05 ? QStringLiteral("+") : QString();
    return QStringLiteral("%1%2 ms").arg(sign, QString::number(m_syncOffsetMs, 'f', decimals));
}

QString GyroConfigBridge::levelReferenceText() const
{
    if (!m_levelReferenceSet)
        return QStringLiteral("Not set");

    return QStringLiteral("X %1 / Y %2 / Z %3 g")
        .arg(formatConfigDoublePrecise(m_levelAccelXG, 2),
             formatConfigDoublePrecise(m_levelAccelYG, 2),
             formatConfigDoublePrecise(m_levelAccelZG, 2));
}

void GyroConfigBridge::refresh()
{
    m_liveRetryTimer.invalidate();
    m_autoProbeTimer.invalidate();
    loadConfiguration();
    if (m_livePollingEnabled && !m_configEnabled) {
        m_liveTimer.stop();
        closeLiveDevice();
        setLiveAvailable(false, QStringLiteral("IMU disabled in apertar-hardware.conf"));
    }
}

bool GyroConfigBridge::setDriverName(const QString &driver)
{
    const QString canonical = canonicalDriverName(driver);
    QString error;
    if (!writeConfigValue(QStringLiteral("imu"), QStringLiteral("driver"), canonical, &error)) {
        emit configSaveFinished(false, error);
        return false;
    }

    closeLiveDevice();
    m_liveRetryTimer.invalidate();
    m_autoProbeTimer.invalidate();
    loadConfiguration();
    if (m_livePollingEnabled)
        pollLiveSample();
    emit configSaveFinished(true, QStringLiteral("IMU driver saved."));
    return true;
}

void GyroConfigBridge::setLivePollingEnabled(bool enabled)
{
    if (m_livePollingEnabled == enabled)
        return;

    m_livePollingEnabled = enabled;
    emit livePollingEnabledChanged();

    if (enabled) {
        loadConfiguration();
        if (!m_configEnabled) {
            m_liveTimer.stop();
            closeLiveDevice();
            setLiveAvailable(false, QStringLiteral("IMU disabled in apertar-hardware.conf"));
            return;
        }
        m_liveTimer.start();
        pollLiveSample();
    } else {
        m_liveTimer.stop();
        closeLiveDevice();
        resetLiveAccelFilter();
        setLiveAvailable(false, QStringLiteral("Live gyro idle"));
    }
}

void GyroConfigBridge::setLivePollingIntervalMs(int intervalMs)
{
    const int clamped = std::clamp(intervalMs, 40, 1000);
    if (m_livePollingIntervalMs == clamped)
        return;

    m_livePollingIntervalMs = clamped;
    m_liveTimer.setInterval(m_livePollingIntervalMs);
    emit livePollingIntervalMsChanged();
}

void GyroConfigBridge::setLiveAccelOnly(bool enabled)
{
    if (m_liveAccelOnly == enabled)
        return;

    m_liveAccelOnly = enabled;
    resetLiveAccelFilter();
    emit liveAccelOnlyChanged();
}

void GyroConfigBridge::setLivePollingRequest(const QString &requestId,
                                             bool enabled,
                                             bool accelOnly,
                                             int intervalMs)
{
    const QString id = requestId.trimmed().isEmpty()
                           ? QStringLiteral("default")
                           : requestId.trimmed();
    if (!enabled) {
        if (!m_livePollingRequests.remove(id))
            return;
        applyLivePollingRequests();
        return;
    }

    LivePollingRequest request;
    request.accelOnly = accelOnly;
    request.intervalMs = std::clamp(intervalMs, 40, 1000);

    const auto existing = m_livePollingRequests.constFind(id);
    if (existing != m_livePollingRequests.constEnd()
        && existing->accelOnly == request.accelOnly
        && existing->intervalMs == request.intervalMs) {
        return;
    }

    m_livePollingRequests.insert(id, request);
    applyLivePollingRequests();
}

void GyroConfigBridge::applyLivePollingRequests()
{
    if (m_standby || m_livePollingRequests.isEmpty()) {
        setLivePollingEnabled(false);
        return;
    }

    bool accelOnly = true;
    int intervalMs = 1000;
    for (auto it = m_livePollingRequests.constBegin(); it != m_livePollingRequests.constEnd(); ++it) {
        accelOnly = accelOnly && it->accelOnly;
        intervalMs = std::min(intervalMs, it->intervalMs);
    }

    setLiveAccelOnly(accelOnly);
    setLivePollingIntervalMs(intervalMs);
    setLivePollingEnabled(true);
}

void GyroConfigBridge::setStandby(bool standby)
{
    if (m_standby == standby)
        return;
    m_standby = standby;
    applyLivePollingRequests();
}

bool GyroConfigBridge::setLevelReference()
{
    loadConfiguration();
    if (!m_configEnabled) {
        emit configSaveFinished(false, QStringLiteral("IMU disabled in apertar-hardware.conf."));
        return false;
    }

    if (!m_liveAvailable)
        pollLiveSample();

    if (!m_liveAvailable) {
        emit configSaveFinished(false, QStringLiteral("Gyro live sample is not available."));
        return false;
    }

    const double levelX = mappedLiveAccel(0);
    const double levelY = mappedLiveAccel(1);
    const double levelZ = mappedLiveAccel(2);

    QString error;
    const auto writeValue = [this, &error](const QString &key, const QString &value) {
        return writeConfigValue(driverConfigSection(m_driver), key, value, &error);
    };

    if (!writeValue(QStringLiteral("level_reference_set"), QStringLiteral("true")) ||
        !writeValue(QStringLiteral("level_reference_orientation"), m_orientation) ||
        !writeValue(QStringLiteral("level_accel_x"), formatConfigDoublePrecise(levelX, 4)) ||
        !writeValue(QStringLiteral("level_accel_y"), formatConfigDoublePrecise(levelY, 4)) ||
        !writeValue(QStringLiteral("level_accel_z"), formatConfigDoublePrecise(levelZ, 4))) {
        emit configSaveFinished(false, error);
        return false;
    }

    setLevelReferenceValue(true, levelX, levelY, levelZ, m_orientation);
    emit configSaveFinished(true, QStringLiteral("Gyro level reference set."));
    return true;
}

bool GyroConfigBridge::clearLevelReference()
{
    QString error;
    const auto writeValue = [this, &error](const QString &key, const QString &value) {
        return writeConfigValue(driverConfigSection(m_driver), key, value, &error);
    };

    if (!writeValue(QStringLiteral("level_reference_set"), QStringLiteral("false")) ||
        !writeValue(QStringLiteral("level_accel_x"), QStringLiteral("0")) ||
        !writeValue(QStringLiteral("level_accel_y"), QStringLiteral("0")) ||
        !writeValue(QStringLiteral("level_accel_z"), QStringLiteral("0"))) {
        emit configSaveFinished(false, error);
        return false;
    }

    setLevelReferenceValue(false, 0.0, 0.0, 0.0, m_orientation);
    emit configSaveFinished(true, QStringLiteral("Gyro level reference cleared."));
    return true;
}

bool GyroConfigBridge::setOrientation(const QString &orientation)
{
    const QString normalized = normalizeOrientation(orientation);
    if (normalized.isEmpty()) {
        emit configSaveFinished(false, QStringLiteral("Orientation must contain X, Y, and Z once."));
        return false;
    }

    QString error;
    if (!writeConfigValue(driverConfigSection(m_driver), QStringLiteral("orientation"), normalized, &error)) {
        emit configSaveFinished(false, error);
        return false;
    }

    setOrientationValue(normalized);
    emit configSaveFinished(true, QStringLiteral("Gyro orientation saved."));
    return true;
}

bool GyroConfigBridge::setSyncOffsetMs(double value)
{
    const double clamped = std::clamp(value, -2000.0, 2000.0);
    QString error;
    if (!writeConfigValue(QStringLiteral("imu"), QStringLiteral("sync_offset_ms"), formatConfigDouble(clamped), &error)) {
        emit configSaveFinished(false, error);
        return false;
    }

    setSyncOffsetValue(clamped);
    emit configSaveFinished(true, QStringLiteral("Gyro sync offset saved."));
    return true;
}

bool GyroConfigBridge::adjustSyncOffsetMs(double delta)
{
    return setSyncOffsetMs(m_syncOffsetMs + delta);
}

bool GyroConfigBridge::resetSyncOffset()
{
    return setSyncOffsetMs(0.0);
}

void GyroConfigBridge::loadConfiguration()
{
    setDriver(QStringLiteral("mpu6050"));
    setSyncMode(QStringLiteral("free_run"));
    setOrientationValue(QStringLiteral("XYZ"));
    setSyncOffsetValue(0.0);
    setSampleRateHz(500);
    setBusNumber(1);
    setAddress(0x68);
    setAccelRangeG(8);
    setGyroRangeDps(1000);
    m_configEnabled = true;
    setLevelReferenceValue(false, 0.0, 0.0, 0.0, QString());

    QString configPath = defaultHardwareConfigPath();
    for (const QString &candidate : hardwareConfigCandidatePaths()) {
        if (QFileInfo::exists(candidate)) {
            configPath = candidate;
            break;
        }
    }
    setConfigPath(configPath);

    const QStringList lines = readLines(configPath);
    QString preflightSection;
    for (const QString &line : lines) {
        const QString maybeSection = sectionNameFromLine(line);
        if (!maybeSection.isEmpty()) {
            preflightSection = maybeSection;
            continue;
        }

        if (preflightSection != QStringLiteral("imu"))
            continue;

        const int eq = line.indexOf(QLatin1Char('='));
        if (eq < 0)
            continue;

        const QString key = normalizeToken(line.left(eq));
        if (key == QStringLiteral("driver")) {
            const QString driver = canonicalDriverName(line.mid(eq + 1));
            if (!driver.isEmpty())
                setDriver(driver);
        }
    }

    QString section;
    bool syncOffsetFromImu = false;
    bool parsedLevelReferenceSet = false;
    bool parsedLevelX = false;
    bool parsedLevelY = false;
    bool parsedLevelZ = false;
    double parsedLevelAccelXG = 0.0;
    double parsedLevelAccelYG = 0.0;
    double parsedLevelAccelZG = 0.0;
    QString parsedLevelOrientation;
    for (const QString &line : lines) {
        const QString maybeSection = sectionNameFromLine(line);
        if (!maybeSection.isEmpty()) {
            section = maybeSection;
            continue;
        }

        const QString trimmed = trimConfig(line);
        if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#')) || trimmed.startsWith(QLatin1Char(';')))
            continue;

        const int eq = line.indexOf(QLatin1Char('='));
        if (eq < 0)
            continue;

        const QString key = normalizeToken(line.left(eq));
        const QString value = line.mid(eq + 1);
        if (section == QStringLiteral("imu")) {
            if (key == QStringLiteral("enabled")) {
                m_configEnabled = parseConfigBool(value, m_configEnabled);
            } else if (key == QStringLiteral("driver")) {
                const QString driver = normalizeToken(value);
                if (!driver.isEmpty())
                    setDriver(driver);
            } else if (key == QStringLiteral("sync_mode")) {
                const QString syncMode = normalizeToken(value);
                if (!syncMode.isEmpty())
                    setSyncMode(syncMode);
            } else if (key == QStringLiteral("sync_offset_ms") ||
                       key == QStringLiteral("offset_ms") ||
                       key == QStringLiteral("gyro_sync_offset_ms")) {
                double offset = 0.0;
                if (parseConfigDouble(value, &offset)) {
                    setSyncOffsetValue(std::clamp(offset, -2000.0, 2000.0));
                    syncOffsetFromImu = true;
                }
            }
        } else if (isActiveGyroConfigSection(section, m_driver)) {
            if (key == QStringLiteral("enabled")) {
                m_configEnabled = parseConfigBool(value, m_configEnabled);
            } else if (key == QStringLiteral("orientation") || key == QStringLiteral("imu_orientation")) {
                const QString orientation = normalizeOrientation(value);
                if (!orientation.isEmpty())
                    setOrientationValue(orientation);
            } else if (key == QStringLiteral("sample_rate_hz") || key == QStringLiteral("sample_rate")) {
                int sampleRate = 0;
                if (parseConfigInt(value, &sampleRate))
                    setSampleRateHz(std::clamp(sampleRate, 25, 1000));
            } else if (key == QStringLiteral("bus") || key == QStringLiteral("bus_number")) {
                int busNumber = 0;
                if (parseConfigInt(value, &busNumber))
                    setBusNumber(std::clamp(busNumber, 0, 32));
            } else if (key == QStringLiteral("address") || key == QStringLiteral("addr")) {
                int address = 0;
                if (parseConfigInt(value, &address))
                    setAddress(std::clamp(address, 0x03, 0x77));
            } else if (key == QStringLiteral("accel_range_g") || key == QStringLiteral("accel_range")) {
                int accelRange = 0;
                if (parseConfigInt(value, &accelRange))
                    setAccelRangeG(normalizeAccelRangeG(accelRange));
            } else if (key == QStringLiteral("gyro_range_dps") || key == QStringLiteral("gyro_range")) {
                int gyroRange = 0;
                if (parseConfigInt(value, &gyroRange))
                    setGyroRangeDps(normalizeGyroRangeDps(gyroRange));
            } else if (key == QStringLiteral("level_reference_set") ||
                       key == QStringLiteral("level_set") ||
                       key == QStringLiteral("accel_level_set")) {
                parsedLevelReferenceSet = parseConfigBool(value, parsedLevelReferenceSet);
            } else if (key == QStringLiteral("level_reference_orientation") ||
                       key == QStringLiteral("level_orientation")) {
                parsedLevelOrientation = normalizeOrientation(value);
            } else if (key == QStringLiteral("level_accel_x") ||
                       key == QStringLiteral("level_x_g") ||
                       key == QStringLiteral("accel_zero_x")) {
                double accel = 0.0;
                if (parseConfigDouble(value, &accel)) {
                    parsedLevelAccelXG = accel;
                    parsedLevelX = true;
                }
            } else if (key == QStringLiteral("level_accel_y") ||
                       key == QStringLiteral("level_y_g") ||
                       key == QStringLiteral("accel_zero_y")) {
                double accel = 0.0;
                if (parseConfigDouble(value, &accel)) {
                    parsedLevelAccelYG = accel;
                    parsedLevelY = true;
                }
            } else if (key == QStringLiteral("level_accel_z") ||
                       key == QStringLiteral("level_z_g") ||
                       key == QStringLiteral("accel_zero_z")) {
                double accel = 0.0;
                if (parseConfigDouble(value, &accel)) {
                    parsedLevelAccelZG = accel;
                    parsedLevelZ = true;
                }
            } else if (!syncOffsetFromImu &&
                       (key == QStringLiteral("sync_offset_ms") ||
                        key == QStringLiteral("offset_ms") ||
                        key == QStringLiteral("gyro_sync_offset_ms"))) {
                double offset = 0.0;
                if (parseConfigDouble(value, &offset))
                    setSyncOffsetValue(std::clamp(offset, -2000.0, 2000.0));
            }
        }
    }

    const bool levelReferenceComplete = parsedLevelReferenceSet && parsedLevelX && parsedLevelY && parsedLevelZ;
    const bool levelOrientationMatches = parsedLevelOrientation.isEmpty() || parsedLevelOrientation == m_orientation;
    setLevelReferenceValue(levelReferenceComplete && levelOrientationMatches,
                           parsedLevelAccelXG,
                           parsedLevelAccelYG,
                           parsedLevelAccelZG,
                           parsedLevelOrientation);
}

bool GyroConfigBridge::ensureLiveDevice()
{
    if (!m_configEnabled) {
        closeLiveDevice();
        setLiveAvailable(false, QStringLiteral("IMU disabled in apertar-hardware.conf"));
        return false;
    }

    if (m_liveFd >= 0 && m_liveConfigured)
        return true;

    // A missing IMU must not turn each level update into a full I2C scan.
    if (m_liveRetryTimer.isValid() && m_liveRetryTimer.elapsed() < 5000)
        return false;
    m_liveRetryTimer.start();

    if (m_liveFd >= 0) {
        if (configureLiveDevice())
            return true;
        closeLiveDevice();
        return false;
    }

    const QString devicePath = QStringLiteral("/dev/i2c-%1").arg(m_busNumber);
    m_liveFd = ::open(devicePath.toLocal8Bit().constData(), O_RDWR | O_CLOEXEC);
    if (m_liveFd < 0) {
        if (probeLiveDevice())
            return true;
        setLiveAvailable(false, QStringLiteral("Cannot open %1").arg(devicePath));
        return false;
    }

    if (::ioctl(m_liveFd, I2C_SLAVE, m_address) < 0) {
        const QString errorText = QString::fromLocal8Bit(std::strerror(errno));
        closeLiveDevice();
        if (probeLiveDevice())
            return true;
        setLiveAvailable(false,
                         QStringLiteral("Cannot select %1 0x%2: %3")
                             .arg(driverDisplayName(m_driver))
                             .arg(m_address, 2, 16, QLatin1Char('0'))
                             .arg(errorText));
        return false;
    }

    quint8 chipId = 0;
    if (!readRegisterFromFd(m_liveFd, m_address, chipIdRegisterForDriver(m_driver), &chipId) ||
        !chipIdMatchesDriver(m_driver, chipId)) {
        const int configuredBus = m_busNumber;
        const int configuredAddress = m_address;
        closeLiveDevice();
        if (probeLiveDevice())
            return true;

        setLiveAvailable(false,
                         QStringLiteral("%1 not found. i2c-%2 0x%3 returned id 0x%4")
                             .arg(driverDisplayName(m_driver))
                             .arg(configuredBus)
                             .arg(configuredAddress, 2, 16, QLatin1Char('0'))
                             .arg(chipId, 2, 16, QLatin1Char('0')));
        return false;
    }

    if (configureLiveDevice())
        return true;
    closeLiveDevice();
    return false;
}

bool GyroConfigBridge::probeLiveDevice()
{
    if (!m_configEnabled)
        return false;

    if (m_autoProbeTimer.isValid() && m_autoProbeTimer.elapsed() < 30000)
        return false;
    m_autoProbeTimer.start();

    const QString driver = canonicalDriverName(m_driver);
    const quint8 chipRegister = chipIdRegisterForDriver(driver);
    std::vector<int> buses;
    std::vector<QString> devicePaths;
    const auto addBus = [&buses, &devicePaths](int bus) {
        if (bus < 0 || bus > 32)
            return;
        const QString path = QFileInfo(QStringLiteral("/dev/i2c-%1").arg(bus)).canonicalFilePath();
        if (path.isEmpty() || std::find(devicePaths.begin(), devicePaths.end(), path) != devicePaths.end())
            return;
        devicePaths.push_back(path);
        buses.push_back(bus);
    };
    addBus(m_busNumber);
    for (int bus = 0; bus <= 32; ++bus) {
        if (QFileInfo::exists(QStringLiteral("/dev/i2c-%1").arg(bus)))
            addBus(bus);
    }

    std::vector<int> addresses;
    const auto addAddress = [&addresses](int address) {
        if (address < 0x03 || address > 0x77)
            return;
        if (std::find(addresses.begin(), addresses.end(), address) == addresses.end())
            addresses.push_back(address);
    };
    addAddress(m_address);
    addAddress(0x68);
    addAddress(0x69);

    for (const int bus : buses) {
        const QString devicePath = QStringLiteral("/dev/i2c-%1").arg(bus);
        for (const int address : addresses) {
            const int fd = ::open(devicePath.toLocal8Bit().constData(), O_RDWR | O_CLOEXEC);
            if (fd < 0)
                continue;
            if (::ioctl(fd, I2C_SLAVE, address) < 0) {
                ::close(fd);
                continue;
            }

            quint8 chipId = 0;
            if (readRegisterFromFd(fd, address, chipRegister, &chipId) && chipIdMatchesDriver(driver, chipId)) {
                const bool detectedConfigChanged = m_busNumber != bus || m_address != address;
                m_busNumber = bus;
                m_address = address;
                m_liveFd = fd;
                if (!configureLiveDevice()) {
                    closeLiveDevice();
                    continue;
                }
                if (detectedConfigChanged)
                    emit configChanged();
                return true;
            }

            ::close(fd);
        }
    }

    return false;
}

void GyroConfigBridge::closeLiveDevice()
{
    if (m_liveFd >= 0) {
        ::close(m_liveFd);
        m_liveFd = -1;
    }
    m_liveConfigured = false;
}

bool GyroConfigBridge::writeLiveRegister(quint8 reg, quint8 value)
{
    if (m_liveFd < 0)
        return false;

    const quint8 buffer[2] = { reg, value };
    return ::write(m_liveFd, buffer, sizeof(buffer)) == static_cast<ssize_t>(sizeof(buffer));
}

bool GyroConfigBridge::configureLiveDevice()
{
    if (m_liveFd < 0)
        return false;
    if (m_liveConfigured)
        return true;

    const QString driver = canonicalDriverName(m_driver);
    const quint8 odr = bmiOdrCode(m_sampleRateHz);

    if (driver == QStringLiteral("bmi160")) {
        if (!writeLiveRegister(kBmi160RegCommand, 0x11))
            return false;
        ::usleep(20000);
        if (!writeLiveRegister(kBmi160RegCommand, 0x15))
            return false;
        ::usleep(80000);

        if (!writeLiveRegister(kBmi160RegAccelConfig, static_cast<quint8>(0x20 | odr)) ||
            !writeLiveRegister(kBmi160RegGyroConfig, static_cast<quint8>(0x20 | odr)) ||
            !writeLiveRegister(kBmi160RegGyroRange, bmiGyroRangeValue(m_gyroRangeDps)) ||
            !writeLiveRegister(kBmi160RegAccelRange, bmi160AccelRangeValue(m_accelRangeG)))
            return false;

        m_liveConfigured = true;
        return true;
    }

    if (driver == QStringLiteral("bmi270")) {
        writeLiveRegister(kBmi270RegPowerConf, 0x00);
        ::usleep(2000);
        if (!writeLiveRegister(kBmi270RegPowerCtrl, 0x0e))
            return false;
        ::usleep(50000);

        if (!writeLiveRegister(kBmi270RegAccelConfig, static_cast<quint8>(0xa0 | odr)) ||
            !writeLiveRegister(kBmi270RegGyroConfig, static_cast<quint8>(0xe0 | odr)) ||
            !writeLiveRegister(kBmi270RegGyroRange, bmiGyroRangeValue(m_gyroRangeDps)) ||
            !writeLiveRegister(kBmi270RegAccelRange, bmi270AccelRangeValue(m_accelRangeG)))
            return false;

        quint8 status = 0;
        (void)readRegisterFromFd(m_liveFd, m_address, kBmi270RegInternalStatus, &status);
        m_liveConfigured = true;
        return true;
    }

    if (driver == QStringLiteral("icm42688p")) {
        writeLiveRegister(kIcm42688RegBankSelect, 0x00);
        if (!writeLiveRegister(kIcm42688RegDeviceConfig, 0x01))
            return false;
        ::usleep(20000);
        writeLiveRegister(kIcm42688RegBankSelect, 0x00);

        const quint8 icmOdr = icm42688OdrCode(m_sampleRateHz);
        if (!writeLiveRegister(kIcm42688RegGyroConfig0,
                               static_cast<quint8>(icm42688GyroRangeBits(m_gyroRangeDps) | icmOdr)) ||
            !writeLiveRegister(kIcm42688RegAccelConfig0,
                               static_cast<quint8>(icm42688AccelRangeBits(m_accelRangeG) | icmOdr)) ||
            !writeLiveRegister(kIcm42688RegPowerMgmt0, 0x0f))
            return false;
        ::usleep(50000);

        quint8 status = 0;
        (void)readRegisterFromFd(m_liveFd, m_address, kIcm42688RegIntStatus, &status);
        m_liveConfigured = true;
        return true;
    }

    if (!writeLiveRegister(kMpu6050RegPwrMgmt1, 0x80))
        return false;
    ::usleep(100000);

    if (!writeLiveRegister(kMpu6050RegPwrMgmt1, 0x03))
        return false;
    ::usleep(10000);

    const quint8 dlpfConfig = mpu6050DlpfConfigForHz(98);
    const int baseRateHz = dlpfConfig == 0 ? 8000 : 1000;
    const int requestedRateHz = std::max(1, m_sampleRateHz);
    const int divisor = std::clamp((baseRateHz + (requestedRateHz / 2)) / requestedRateHz - 1, 0, 255);

    if (!writeLiveRegister(kMpu6050RegConfig, dlpfConfig) ||
        !writeLiveRegister(kMpu6050RegSampleRateDivider, static_cast<quint8>(divisor)) ||
        !writeLiveRegister(kMpu6050RegGyroConfig,
                           static_cast<quint8>(mpu6050GyroRangeBits(m_gyroRangeDps) << 3)) ||
        !writeLiveRegister(kMpu6050RegAccelConfig,
                           static_cast<quint8>(mpu6050AccelRangeBits(m_accelRangeG) << 3)))
        return false;

    m_liveConfigured = true;
    return true;
}

bool GyroConfigBridge::readLiveRegisters(quint8 startRegister, quint8 *data, qsizetype length)
{
    if (!data || length <= 0 || !ensureLiveDevice())
        return false;

    return readRegisterRangeFromFd(m_liveFd, m_address, startRegister, data, length);
}

void GyroConfigBridge::pollLiveSample()
{
    if (!m_configEnabled) {
        closeLiveDevice();
        setLiveAvailable(false, QStringLiteral("IMU disabled in apertar-hardware.conf"));
        return;
    }

    const QString driver = canonicalDriverName(m_driver);
    const QString label = driverDisplayName(driver);
    const double accelScale = accelLsbPerG(m_accelRangeG);
    const double gyroScale = gyroLsbPerDps(m_gyroRangeDps);
    double accelX = 0.0;
    double accelY = 0.0;
    double accelZ = 0.0;
    double gyroX = 0.0;
    double gyroY = 0.0;
    double gyroZ = 0.0;
    bool ok = false;

    if (m_liveAccelOnly && driver == QStringLiteral("bmi160")) {
        std::array<quint8, 6> data{};
        ok = readLiveRegisters(kBmi160RegAccelXoutL, data.data(), static_cast<qsizetype>(data.size()));
        if (ok) {
            accelX = readLittleEndianInt16(data.data() + 0) / accelScale;
            accelY = readLittleEndianInt16(data.data() + 2) / accelScale;
            accelZ = readLittleEndianInt16(data.data() + 4) / accelScale;
        }
    } else if (m_liveAccelOnly && driver == QStringLiteral("bmi270")) {
        std::array<quint8, 6> data{};
        ok = readLiveRegisters(kBmi270RegAccelXoutL, data.data(), static_cast<qsizetype>(data.size()));
        if (ok) {
            accelX = readLittleEndianInt16(data.data() + 0) / accelScale;
            accelY = readLittleEndianInt16(data.data() + 2) / accelScale;
            accelZ = readLittleEndianInt16(data.data() + 4) / accelScale;
        }
    } else if (m_liveAccelOnly && driver == QStringLiteral("icm42688p")) {
        std::array<quint8, 6> data{};
        ok = readLiveRegisters(kIcm42688RegAccelXoutH, data.data(), static_cast<qsizetype>(data.size()));
        if (ok) {
            accelX = readBigEndianInt16(data.data() + 0) / accelScale;
            accelY = readBigEndianInt16(data.data() + 2) / accelScale;
            accelZ = readBigEndianInt16(data.data() + 4) / accelScale;
        }
    } else if (m_liveAccelOnly) {
        std::array<quint8, 6> data{};
        ok = readLiveRegisters(kMpu6050RegAccelXoutH, data.data(), static_cast<qsizetype>(data.size()));
        if (ok) {
            accelX = readBigEndianInt16(data.data() + 0) / accelScale;
            accelY = readBigEndianInt16(data.data() + 2) / accelScale;
            accelZ = readBigEndianInt16(data.data() + 4) / accelScale;
        }
    } else if (driver == QStringLiteral("bmi160")) {
        std::array<quint8, 12> data{};
        ok = readLiveRegisters(kBmi160RegGyroXoutL, data.data(), static_cast<qsizetype>(data.size()));
        if (ok) {
            gyroX = readLittleEndianInt16(data.data() + 0) / gyroScale;
            gyroY = readLittleEndianInt16(data.data() + 2) / gyroScale;
            gyroZ = readLittleEndianInt16(data.data() + 4) / gyroScale;
            accelX = readLittleEndianInt16(data.data() + 6) / accelScale;
            accelY = readLittleEndianInt16(data.data() + 8) / accelScale;
            accelZ = readLittleEndianInt16(data.data() + 10) / accelScale;
        }
    } else if (driver == QStringLiteral("bmi270")) {
        std::array<quint8, 12> data{};
        ok = readLiveRegisters(kBmi270RegAccelXoutL, data.data(), static_cast<qsizetype>(data.size()));
        if (ok) {
            accelX = readLittleEndianInt16(data.data() + 0) / accelScale;
            accelY = readLittleEndianInt16(data.data() + 2) / accelScale;
            accelZ = readLittleEndianInt16(data.data() + 4) / accelScale;
            gyroX = readLittleEndianInt16(data.data() + 6) / gyroScale;
            gyroY = readLittleEndianInt16(data.data() + 8) / gyroScale;
            gyroZ = readLittleEndianInt16(data.data() + 10) / gyroScale;
        }
    } else if (driver == QStringLiteral("icm42688p")) {
        std::array<quint8, 12> data{};
        ok = readLiveRegisters(kIcm42688RegAccelXoutH, data.data(), static_cast<qsizetype>(data.size()));
        if (ok) {
            accelX = readBigEndianInt16(data.data() + 0) / accelScale;
            accelY = readBigEndianInt16(data.data() + 2) / accelScale;
            accelZ = readBigEndianInt16(data.data() + 4) / accelScale;
            gyroX = readBigEndianInt16(data.data() + 6) / gyroScale;
            gyroY = readBigEndianInt16(data.data() + 8) / gyroScale;
            gyroZ = readBigEndianInt16(data.data() + 10) / gyroScale;
        }
    } else {
        std::array<quint8, 14> data{};
        ok = readLiveRegisters(kMpu6050RegAccelXoutH, data.data(), static_cast<qsizetype>(data.size()));
        if (ok) {
            accelX = readBigEndianInt16(data.data() + 0) / accelScale;
            accelY = readBigEndianInt16(data.data() + 2) / accelScale;
            accelZ = readBigEndianInt16(data.data() + 4) / accelScale;
            gyroX = readBigEndianInt16(data.data() + 8) / gyroScale;
            gyroY = readBigEndianInt16(data.data() + 10) / gyroScale;
            gyroZ = readBigEndianInt16(data.data() + 12) / gyroScale;
        }
    }

    if (!ok) {
        if (m_liveFd >= 0) {
            closeLiveDevice();
            m_liveRetryTimer.start();
        }
        setLiveAvailable(false, QStringLiteral("Waiting for %1").arg(label));
        return;
    }

    setLiveSample(accelX,
                  accelY,
                  accelZ,
                  gyroX,
                  gyroY,
                  gyroZ,
                  QStringLiteral("%1 live 0x%2 on i2c-%3")
                      .arg(label)
                      .arg(m_address, 2, 16, QLatin1Char('0'))
                      .arg(m_busNumber));
}

void GyroConfigBridge::setLiveAvailable(bool available, const QString &statusText)
{
    if (m_liveAvailable == available && m_liveStatusText == statusText)
        return;

    if (!available)
        resetLiveAccelFilter();

    m_liveAvailable = available;
    m_liveStatusText = statusText;
    emit liveSampleChanged();
}

void GyroConfigBridge::setLiveSample(double accelXG, double accelYG, double accelZG,
                                     double gyroXDps, double gyroYDps, double gyroZDps,
                                     const QString &statusText)
{
    if (m_liveAccelOnly) {
        if (!m_liveAccelFilterValid) {
            m_liveFilteredAccelXG = accelXG;
            m_liveFilteredAccelYG = accelYG;
            m_liveFilteredAccelZG = accelZG;
            m_liveAccelFilterValid = true;
        } else {
            const double previousFilteredRoll = rollDegreesForOrientation(m_orientation,
                                                                          m_liveFilteredAccelXG,
                                                                          m_liveFilteredAccelYG,
                                                                          m_liveFilteredAccelZG);
            const double rawRoll = rollDegreesForOrientation(m_orientation, accelXG, accelYG, accelZG);
            const double rawRollDelta = std::abs(normalizeAngleDelta(rawRoll - previousFilteredRoll));
            const double previousFilteredPitch = pitchDegreesForOrientation(m_orientation,
                                                                             m_liveFilteredAccelXG,
                                                                             m_liveFilteredAccelYG,
                                                                             m_liveFilteredAccelZG);
            const double rawPitch = pitchDegreesForOrientation(m_orientation, accelXG, accelYG, accelZG);
            const double rawPitchDelta = std::abs(rawPitch - previousFilteredPitch);
            const double alpha = std::max(rawRollDelta, rawPitchDelta) > 3.0 ? 0.42 : 0.18;
            m_liveFilteredAccelXG += (accelXG - m_liveFilteredAccelXG) * alpha;
            m_liveFilteredAccelYG += (accelYG - m_liveFilteredAccelYG) * alpha;
            m_liveFilteredAccelZG += (accelZG - m_liveFilteredAccelZG) * alpha;
        }

        accelXG = m_liveFilteredAccelXG;
        accelYG = m_liveFilteredAccelYG;
        accelZG = m_liveFilteredAccelZG;
    }

    if (m_liveAccelOnly && m_liveAvailable && m_liveStatusText == statusText) {
        const double previousRoll = rollDegreesForOrientation(m_orientation, m_accelXG, m_accelYG, m_accelZG);
        const double nextRoll = rollDegreesForOrientation(m_orientation, accelXG, accelYG, accelZG);
        const double rollDelta = std::abs(normalizeAngleDelta(nextRoll - previousRoll));
        const double previousPitch = pitchDegreesForOrientation(m_orientation, m_accelXG, m_accelYG, m_accelZG);
        const double nextPitch = pitchDegreesForOrientation(m_orientation, accelXG, accelYG, accelZG);
        const double pitchDelta = std::abs(nextPitch - previousPitch);
        constexpr double kAccelOnlyAngleEmitThresholdDegrees = 0.04;
        if (rollDelta < kAccelOnlyAngleEmitThresholdDegrees && pitchDelta < kAccelOnlyAngleEmitThresholdDegrees)
            return;
    }

    m_liveAvailable = true;
    m_liveStatusText = statusText;
    m_accelXG = accelXG;
    m_accelYG = accelYG;
    m_accelZG = accelZG;
    m_gyroXDps = gyroXDps;
    m_gyroYDps = gyroYDps;
    m_gyroZDps = gyroZDps;
    emit liveSampleChanged();
}

void GyroConfigBridge::resetLiveAccelFilter()
{
    m_liveAccelFilterValid = false;
    m_liveFilteredAccelXG = 0.0;
    m_liveFilteredAccelYG = 0.0;
    m_liveFilteredAccelZG = 0.0;
}

double GyroConfigBridge::mappedLiveAccel(int index) const
{
    const QString orientation = normalizeOrientation(m_orientation).isEmpty()
                                    ? QStringLiteral("XYZ")
                                    : normalizeOrientation(m_orientation);
    if (index < 0 || index >= orientation.size())
        return 0.0;

    const QChar token = orientation.at(index);
    const QChar axis = token.toUpper();
    double value = 0.0;
    if (axis == QLatin1Char('X'))
        value = m_accelXG;
    else if (axis == QLatin1Char('Y'))
        value = m_accelYG;
    else if (axis == QLatin1Char('Z'))
        value = m_accelZG;

    return token == axis ? value : -value;
}

void GyroConfigBridge::setLevelReferenceValue(bool enabled,
                                              double accelXG,
                                              double accelYG,
                                              double accelZG,
                                              const QString &orientation)
{
    if (m_levelReferenceSet == enabled &&
        std::abs(m_levelAccelXG - accelXG) < 0.0001 &&
        std::abs(m_levelAccelYG - accelYG) < 0.0001 &&
        std::abs(m_levelAccelZG - accelZG) < 0.0001 &&
        m_levelReferenceOrientation == orientation)
        return;

    m_levelReferenceSet = enabled;
    m_levelAccelXG = accelXG;
    m_levelAccelYG = accelYG;
    m_levelAccelZG = accelZG;
    m_levelReferenceOrientation = orientation;
    emit levelReferenceChanged();
}

bool GyroConfigBridge::writeConfigValue(const QString &sectionName,
                                        const QString &key,
                                        const QString &value,
                                        QString *error)
{
    QString path = m_configPath.isEmpty() ? defaultHardwareConfigPath() : m_configPath;
    if (path.isEmpty())
        path = defaultHardwareConfigPath();

    QStringList lines = readLines(path);
    if (lines.isEmpty()) {
        lines << QStringLiteral("# Apertar hardware configuration")
              << QString();
    }

    const QString normalizedSection = normalizeToken(sectionName);
    const QString normalizedKey = normalizeToken(key);
    bool foundSection = false;
    bool inTargetSection = false;
    bool wroteKey = false;

    for (int i = 0; i < lines.size(); ++i) {
        const QString maybeSection = sectionNameFromLine(lines[i]);
        if (!maybeSection.isEmpty()) {
            if (inTargetSection && !wroteKey) {
                lines.insert(i, QStringLiteral("%1=%2").arg(normalizedKey, value));
                wroteKey = true;
                ++i;
            }
            inTargetSection = maybeSection == normalizedSection;
            foundSection = foundSection || inTargetSection;
            continue;
        }

        if (!inTargetSection)
            continue;

        const int eq = lines[i].indexOf(QLatin1Char('='));
        if (eq < 0)
            continue;

        const QString existingKey = normalizeToken(lines[i].left(eq));
        if (existingKey != normalizedKey)
            continue;

        const int indentEnd = lines[i].indexOf(QRegularExpression(QStringLiteral("[^ \\t]")));
        const QString indent = indentEnd > 0 ? lines[i].left(indentEnd) : QString();
        lines[i] = QStringLiteral("%1%2=%3").arg(indent, normalizedKey, value);
        wroteKey = true;
        break;
    }

    if (inTargetSection && !wroteKey) {
        lines.push_back(QStringLiteral("%1=%2").arg(normalizedKey, value));
        wroteKey = true;
    }

    if (!foundSection) {
        if (!lines.isEmpty() && !lines.last().trimmed().isEmpty())
            lines.push_back(QString());
        lines.push_back(QStringLiteral("[%1]").arg(normalizedSection));
        lines.push_back(QStringLiteral("%1=%2").arg(normalizedKey, value));
        wroteKey = true;
    }

    if (!wroteKey) {
        if (error)
            *error = QStringLiteral("Could not update gyro config.");
        return false;
    }

    if (!writeLines(path, lines, error))
        return false;

    setConfigPath(path);
    return true;
}

void GyroConfigBridge::setConfigPath(const QString &value)
{
    if (m_configPath == value)
        return;
    m_configPath = value;
    emit configChanged();
}

void GyroConfigBridge::setDriver(const QString &value)
{
    const QString canonical = canonicalDriverName(value);
    if (m_driver == canonical)
        return;
    m_driver = canonical;
    closeLiveDevice();
    emit configChanged();
}

void GyroConfigBridge::setSyncMode(const QString &value)
{
    if (m_syncMode == value)
        return;
    m_syncMode = value;
    emit configChanged();
}

void GyroConfigBridge::setOrientationValue(const QString &value)
{
    if (m_orientation == value)
        return;
    m_orientation = value;
    if (m_levelReferenceSet &&
        !m_levelReferenceOrientation.isEmpty() &&
        m_levelReferenceOrientation != value)
        setLevelReferenceValue(false, m_levelAccelXG, m_levelAccelYG, m_levelAccelZG, m_levelReferenceOrientation);
    emit orientationChanged();
}

void GyroConfigBridge::setSyncOffsetValue(double value)
{
    if (std::abs(m_syncOffsetMs - value) < 0.001)
        return;
    m_syncOffsetMs = value;
    emit syncOffsetMsChanged();
}

void GyroConfigBridge::setSampleRateHz(int value)
{
    if (m_sampleRateHz == value)
        return;
    m_sampleRateHz = value;
    m_liveConfigured = false;
    emit configChanged();
}

void GyroConfigBridge::setBusNumber(int value)
{
    if (m_busNumber == value)
        return;
    m_busNumber = value;
    closeLiveDevice();
    emit configChanged();
}

void GyroConfigBridge::setAddress(int value)
{
    if (m_address == value)
        return;
    m_address = value;
    closeLiveDevice();
    emit configChanged();
}

void GyroConfigBridge::setAccelRangeG(int value)
{
    value = normalizeAccelRangeG(value);
    if (m_accelRangeG == value)
        return;
    m_accelRangeG = value;
    m_liveConfigured = false;
    emit configChanged();
}

void GyroConfigBridge::setGyroRangeDps(int value)
{
    value = normalizeGyroRangeDps(value);
    if (m_gyroRangeDps == value)
        return;
    m_gyroRangeDps = value;
    m_liveConfigured = false;
    emit configChanged();
}
