#include "DeviceInfoBridge.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QStorageInfo>
#include <QSysInfo>
#include <QTextStream>

#include <sys/sysinfo.h>

DeviceInfoBridge::DeviceInfoBridge(QObject *parent)
    : QObject(parent)
{
    detectStaticInfo();
    refreshDynamicInfo();

    m_refreshTimer.setInterval(5000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &DeviceInfoBridge::refreshDynamicInfo);
    m_refreshTimer.start();
}

void DeviceInfoBridge::detectStaticInfo()
{
    const QString model = readTextFile(QStringLiteral("/proc/device-tree/model"));
    if (!model.isEmpty())
        m_piModel = model;

    QString serial = readTextFile(QStringLiteral("/proc/device-tree/serial-number"));
    if (serial.isEmpty()) {
        QFile cpuInfo(QStringLiteral("/proc/cpuinfo"));
        if (cpuInfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&cpuInfo);
            while (!stream.atEnd()) {
                const QString line = stream.readLine();
                if (line.startsWith(QStringLiteral("Serial"), Qt::CaseInsensitive)) {
                    const QStringList parts = line.split(QLatin1Char(':'));
                    if (parts.size() > 1) {
                        serial = parts.at(1).trimmed();
                        break;
                    }
                }
            }
        }
    }
    if (!serial.isEmpty())
        m_serialNumber = serial;

    const QString prettyOs = QSysInfo::prettyProductName();
    if (!prettyOs.isEmpty())
        m_osVersion = prettyOs;

    const QString kernel = QSysInfo::kernelVersion();
    if (!kernel.isEmpty())
        m_kernelVersion = kernel;

    const QString host = QSysInfo::machineHostName();
    if (!host.isEmpty())
        m_hostname = host;

    const QString sensor = detectSensorName();
    if (!sensor.isEmpty())
        m_sensorName = sensor;
}

void DeviceInfoBridge::refreshDynamicInfo()
{
    bool changed = false;

    const QString nextSensorName = detectSensorName();
    if (nextSensorName != m_sensorName) {
        m_sensorName = nextSensorName;
        changed = true;
    }

    struct sysinfo info;
    const bool haveSysInfo = ::sysinfo(&info) == 0;

    const qulonglong memUnit = haveSysInfo ? static_cast<qulonglong>(info.mem_unit) : 1024ull;
    const qulonglong memTotalBytes = haveSysInfo
            ? static_cast<qulonglong>(info.totalram) * memUnit
            : readMemInfoValueKb(QStringLiteral("MemTotal")) * 1024ull;
    const qulonglong memAvailableBytes = haveSysInfo
            ? static_cast<qulonglong>(info.freeram + info.bufferram) * memUnit
            : readMemInfoValueKb(QStringLiteral("MemAvailable")) * 1024ull;

    const QString nextRamTotal = memTotalBytes > 0 ? formatBytes(memTotalBytes) : QStringLiteral("Unavailable");
    const QString nextRamAvailable = memAvailableBytes > 0 ? formatBytes(memAvailableBytes) : QStringLiteral("Unavailable");

    if (nextRamTotal != m_ramTotalText) {
        m_ramTotalText = nextRamTotal;
        changed = true;
    }

    if (nextRamAvailable != m_ramAvailableText) {
        m_ramAvailableText = nextRamAvailable;
        changed = true;
    }

    const QString nextUptime = haveSysInfo
            ? formatUptime(static_cast<qulonglong>(info.uptime))
            : QStringLiteral("Unavailable");
    if (nextUptime != m_uptimeText) {
        m_uptimeText = nextUptime;
        changed = true;
    }

    const QString nextIpAddress = primaryIpv4Address();
    if (nextIpAddress != m_ipAddress) {
        m_ipAddress = nextIpAddress;
        changed = true;
    }

    const QStorageInfo rootStorage = QStorageInfo::root();
    const QString nextStorageTotal = rootStorage.isValid()
            ? formatBytes(static_cast<qulonglong>(rootStorage.bytesTotal()))
            : QStringLiteral("Unavailable");
    const QString nextStorageFree = rootStorage.isValid()
            ? formatBytes(static_cast<qulonglong>(rootStorage.bytesAvailable()))
            : QStringLiteral("Unavailable");
    const QString nextMountPoint = rootStorage.isValid() ? rootStorage.rootPath() : QStringLiteral("/");

    if (nextStorageTotal != m_systemStorageTotalText) {
        m_systemStorageTotalText = nextStorageTotal;
        changed = true;
    }

    if (nextStorageFree != m_systemStorageFreeText) {
        m_systemStorageFreeText = nextStorageFree;
        changed = true;
    }

    if (nextMountPoint != m_systemStorageMountPoint) {
        m_systemStorageMountPoint = nextMountPoint;
        changed = true;
    }

    if (changed)
        emit infoChanged();
}

QString DeviceInfoBridge::readTextFile(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    QByteArray data = file.readAll();
    data.replace('\0', ' ');
    return QString::fromUtf8(data).trimmed();
}

QString DeviceInfoBridge::detectSensorName() const
{
    const QString fromI2cDevices = scanForSensorTokenInDirectory(QStringLiteral("/sys/bus/i2c/devices"), 200);
    if (!fromI2cDevices.isEmpty())
        return prettySensorName(fromI2cDevices);

    const QString fromFirmwareDeviceTree = scanForSensorTokenInDirectory(QStringLiteral("/sys/firmware/devicetree/base"), 400);
    if (!fromFirmwareDeviceTree.isEmpty())
        return prettySensorName(fromFirmwareDeviceTree);

    const QString fromDeviceTree = scanForSensorTokenInDirectory(QStringLiteral("/proc/device-tree"), 400);
    if (!fromDeviceTree.isEmpty())
        return prettySensorName(fromDeviceTree);

    const QString fromManagedSelection = scanBinaryFileForSensorToken(QStringLiteral("/boot/firmware/camera-selection.txt"), 4096);
    if (!fromManagedSelection.isEmpty())
        return prettySensorName(fromManagedSelection);

    const QString fromConfig = scanBinaryFileForSensorToken(QStringLiteral("/boot/firmware/config.txt"), 4096);
    if (!fromConfig.isEmpty())
        return prettySensorName(fromConfig);

    return QStringLiteral("Unknown Sensor");
}

QString DeviceInfoBridge::scanForSensorTokenInDirectory(const QString &rootPath, int maxFiles) const
{
    QDir root(rootPath);
    if (!root.exists())
        return QString();

    const QRegularExpression filenamePattern(
        QStringLiteral("(imx\\d{3,4}|ov\\d{4,5}|ar\\d{4})"),
        QRegularExpression::CaseInsensitiveOption);

    QDirIterator it(rootPath, QDir::Files, QDirIterator::Subdirectories);
    int visited = 0;
    while (it.hasNext() && visited < maxFiles) {
        const QString filePath = it.next();
        const QFileInfo info(filePath);

        const QRegularExpressionMatch pathMatch = filenamePattern.match(info.fileName());
        if (pathMatch.hasMatch())
            return pathMatch.captured(1);

        const qint64 size = info.size();
        const qint64 maxBytes = info.suffix().compare(QStringLiteral("dng"), Qt::CaseInsensitive) == 0
                ? 256 * 1024
                : qMin<qint64>(size, 4096);

        const QString token = scanBinaryFileForSensorToken(filePath, maxBytes);
        if (!token.isEmpty())
            return token;

        ++visited;
    }

    return QString();
}

QString DeviceInfoBridge::scanBinaryFileForSensorToken(const QString &filePath, qint64 maxBytes) const
{
    if (maxBytes <= 0)
        return QString();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return QString();

    QByteArray data = file.read(maxBytes);
    data.replace('\0', ' ');

    const QString text = QString::fromLatin1(data);
    const QRegularExpression pattern(
        QStringLiteral("(imx\\d{3,4}|ov\\d{4,5}|ar\\d{4})"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = pattern.match(text);
    return match.hasMatch() ? match.captured(1) : QString();
}

QString DeviceInfoBridge::prettySensorName(const QString &token) const
{
    const QString upper = token.toUpper();

    if (upper == QStringLiteral("IMX477"))
        return QStringLiteral("Sony IMX477");
    if (upper == QStringLiteral("IMX296"))
        return QStringLiteral("Sony IMX296");
    if (upper == QStringLiteral("IMX519"))
        return QStringLiteral("Sony IMX519");
    if (upper == QStringLiteral("OV5647"))
        return QStringLiteral("OmniVision OV5647");

    if (upper.startsWith(QStringLiteral("IMX")))
        return QStringLiteral("Sony ") + upper;
    if (upper.startsWith(QStringLiteral("OV")))
        return QStringLiteral("OmniVision ") + upper;
    if (upper.startsWith(QStringLiteral("AR")))
        return QStringLiteral("onsemi ") + upper;

    return upper;
}

QString DeviceInfoBridge::formatBytes(qulonglong bytes) const
{
    static const char *units[] = { "B", "KB", "MB", "GB", "TB" };
    double value = static_cast<double>(bytes);
    int unitIndex = 0;

    while (value >= 1024.0 && unitIndex < 4) {
        value /= 1024.0;
        ++unitIndex;
    }

    const int precision = (value >= 100.0 || unitIndex == 0) ? 0 : 1;
    return QString::number(value, 'f', precision) + QLatin1Char(' ') + QString::fromLatin1(units[unitIndex]);
}

QString DeviceInfoBridge::formatUptime(qulonglong uptimeSeconds) const
{
    const qulonglong days = uptimeSeconds / 86400ull;
    const qulonglong hours = (uptimeSeconds % 86400ull) / 3600ull;
    const qulonglong minutes = (uptimeSeconds % 3600ull) / 60ull;

    if (days > 0)
        return QStringLiteral("%1d %2h %3m").arg(days).arg(hours).arg(minutes);
    if (hours > 0)
        return QStringLiteral("%1h %2m").arg(hours).arg(minutes);
    return QStringLiteral("%1m").arg(minutes);
}

QString DeviceInfoBridge::primaryIpv4Address() const
{
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        if (!(iface.flags() & QNetworkInterface::IsUp)
            || !(iface.flags() & QNetworkInterface::IsRunning)
            || (iface.flags() & QNetworkInterface::IsLoopBack)) {
            continue;
        }

        const QString ifaceName = iface.name();
        if (ifaceName.startsWith(QStringLiteral("docker"))
            || ifaceName.startsWith(QStringLiteral("veth"))
            || ifaceName.startsWith(QStringLiteral("br-"))) {
            continue;
        }

        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            const QHostAddress ip = entry.ip();
            if (ip.protocol() != QAbstractSocket::IPv4Protocol)
                continue;
            if (ip.isLoopback())
                continue;

            const QString ipText = ip.toString();
            if (ipText.startsWith(QStringLiteral("169.254.")))
                continue;

            return ipText;
        }
    }

    return QStringLiteral("Unavailable");
}

qulonglong DeviceInfoBridge::readMemInfoValueKb(const QString &key) const
{
    QFile meminfo(QStringLiteral("/proc/meminfo"));
    if (!meminfo.open(QIODevice::ReadOnly | QIODevice::Text))
        return 0;

    QTextStream stream(&meminfo);
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        if (!line.startsWith(key))
            continue;

        const QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")),
                                             Qt::SkipEmptyParts);
        if (parts.size() >= 2)
            return parts.at(1).toULongLong();
    }

    return 0;
}
