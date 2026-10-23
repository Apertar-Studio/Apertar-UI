#include "MediaStatusBridge.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStorageInfo>
#include <QThread>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QElapsedTimer>

namespace {

QJsonObject coreMediaCommand(const QString &command, QJsonObject fields, QString *error, int timeout = 5000)
{
    QLocalSocket socket;
    socket.connectToServer(qEnvironmentVariable("APERTAR_CONTROL_SOCKET", "/tmp/apertar-core.sock"));
    if (!socket.waitForConnected(1000)) {
        *error = QStringLiteral("Connect to Apertar Core before changing media.");
        return {};
    }
    fields.insert("cmd", command);
    fields.insert("id", 42001);
    socket.write(QJsonDocument(fields).toJson(QJsonDocument::Compact) + '\n');
    QElapsedTimer clock; clock.start();
    while (clock.elapsed() < timeout) {
        while (socket.canReadLine()) {
            const auto reply = QJsonDocument::fromJson(socket.readLine()).object();
            if (reply.value("id").toInt(-1) != 42001) continue;
            if (!reply.value("ok").toBool()) *error = reply.value("error").toString();
            return reply;
        }
        if (!socket.waitForReadyRead(qMax(1, int(timeout - clock.elapsed())))) break;
    }
    *error = QStringLiteral("Core did not confirm the media operation. Check its status before retrying.");
    return {};
}

QString normalizedVolumeLabel(const QString &value)
{
    return value.trimmed().toUpper();
}

QString mediaTypeLabelForVolumeLabel(const QString &volumeLabel)
{
    const QString normalized = normalizedVolumeLabel(volumeLabel);
    if (normalized == QStringLiteral("CFE-RAW"))
        return QStringLiteral("CFExpress Type B");
    if (normalized == QStringLiteral("SSD-RAW") || normalized == QStringLiteral("NVME-RAW"))
        return QStringLiteral("NVMe M.2 SSD");
    if (normalized == QStringLiteral("USB-RAW"))
        return QStringLiteral("External USB");
    if (normalized == QStringLiteral("RAW"))
        return QStringLiteral("CFExpress Type B");
    return QStringLiteral("CFExpress Type B");
}

QString mediaTypeLabelForDeviceAndVolumeLabel(const QString &devicePath,
                                              const QString &volumeLabel)
{
    const QString normalized = normalizedVolumeLabel(volumeLabel);
    if (normalized == QStringLiteral("CFE-RAW") || normalized == QStringLiteral("RAW"))
        return QStringLiteral("CFExpress Type B");
    if (normalized == QStringLiteral("SSD-RAW") || normalized == QStringLiteral("NVME-RAW"))
        return QStringLiteral("NVMe M.2 SSD");
    if (normalized == QStringLiteral("USB-RAW"))
        return QStringLiteral("External USB");
    if (devicePath.startsWith(QStringLiteral("/dev/sd")))
        return QStringLiteral("External USB");
    if (devicePath.startsWith(QStringLiteral("/dev/nvme")))
        return QStringLiteral("NVMe M.2 SSD");
    return mediaTypeLabelForVolumeLabel(volumeLabel);
}

QString mediaPromptLabelForVolumeLabel(const QString &volumeLabel)
{
    const QString normalized = normalizedVolumeLabel(volumeLabel);
    if (normalized == QStringLiteral("CFE-RAW"))
        return QStringLiteral("CFExpress card");
    if (normalized == QStringLiteral("SSD-RAW") || normalized == QStringLiteral("NVME-RAW"))
        return QStringLiteral("NVMe M.2 SSD");
    if (normalized == QStringLiteral("USB-RAW"))
        return QStringLiteral("External USB");
    if (normalized == QStringLiteral("RAW"))
        return QStringLiteral("CFExpress Type B");
    return QStringLiteral("CFExpress Type B");
}

QString mediaPromptLabelForDeviceAndVolumeLabel(const QString &devicePath,
                                                const QString &volumeLabel)
{
    const QString normalized = normalizedVolumeLabel(volumeLabel);
    if (normalized == QStringLiteral("CFE-RAW") || normalized == QStringLiteral("RAW"))
        return QStringLiteral("CFExpress Type B");
    if (normalized == QStringLiteral("SSD-RAW") || normalized == QStringLiteral("NVME-RAW"))
        return QStringLiteral("NVMe M.2 SSD");
    if (normalized == QStringLiteral("USB-RAW"))
        return QStringLiteral("External USB drive");
    if (devicePath.startsWith(QStringLiteral("/dev/sd")))
        return QStringLiteral("External USB drive");
    if (devicePath.startsWith(QStringLiteral("/dev/nvme")))
        return QStringLiteral("NVMe M.2 SSD");
    return mediaPromptLabelForVolumeLabel(volumeLabel);
}

QString normalizedMountPath(QString path)
{
    path = path.trimmed();
    while (path.length() > 1 && path.endsWith('/'))
        path.chop(1);
    return path;
}

QString decodeLsblkValue(QString value)
{
    QRegularExpression hexPattern(QStringLiteral(R"(\\x([0-9a-fA-F]{2}))"));
    QRegularExpressionMatch match = hexPattern.match(value);
    while (match.hasMatch()) {
        bool ok = false;
        const ushort code = match.captured(1).toUShort(&ok, 16);
        value.replace(match.capturedStart(0),
                      match.capturedLength(0),
                      ok ? QString(QChar(code)) : QString());
        match = hexPattern.match(value);
    }

    value.replace(QStringLiteral("\\\""), QStringLiteral("\""));
    value.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));
    return value;
}

QMap<QString, QString> parseLsblkPairs(const QString &line)
{
    QMap<QString, QString> fields;
    QRegularExpression pairPattern(QStringLiteral(R"LSBLK(([A-Z0-9_]+)="((?:\\.|[^"])*)")LSBLK"));
    QRegularExpressionMatchIterator it = pairPattern.globalMatch(line);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        fields.insert(match.captured(1), decodeLsblkValue(match.captured(2)));
    }
    return fields;
}

QString normalizedFormatFilesystemType(const QString &value)
{
    QString normalized = value.trimmed().toLower();
    normalized.remove(QChar(' '));
    normalized.remove(QChar('-'));

    if (normalized == QStringLiteral("exfat"))
        return QStringLiteral("exfat");
    if (normalized == QStringLiteral("fat32") || normalized == QStringLiteral("vfat"))
        return QStringLiteral("vfat");
    if (normalized == QStringLiteral("ntfs"))
        return QStringLiteral("ntfs");
    if (normalized == QStringLiteral("ext4"))
        return QStringLiteral("ext4");

    return QString();
}

QString formatFilesystemDisplayName(const QString &filesystemType)
{
    const QString normalized = normalizedFormatFilesystemType(filesystemType);
    if (normalized == QStringLiteral("exfat"))
        return QStringLiteral("exFAT");
    if (normalized == QStringLiteral("vfat"))
        return QStringLiteral("FAT32");
    if (normalized == QStringLiteral("ntfs"))
        return QStringLiteral("NTFS");
    if (normalized == QStringLiteral("ext4"))
        return QStringLiteral("ext4");
    return QStringLiteral("selected filesystem");
}

QString formatFilesystemToolName(const QString &filesystemType)
{
    const QString normalized = normalizedFormatFilesystemType(filesystemType);
    if (normalized == QStringLiteral("exfat"))
        return QStringLiteral("mkfs.exfat");
    if (normalized == QStringLiteral("vfat"))
        return QStringLiteral("mkfs.vfat");
    if (normalized == QStringLiteral("ntfs"))
        return QStringLiteral("mkfs.ntfs");
    if (normalized == QStringLiteral("ext4"))
        return QStringLiteral("mkfs.ext4");
    return QStringLiteral("mkfs");
}

QString existingExecutablePath(const QStringList &candidates)
{
    for (const QString &candidate : candidates) {
        QFileInfo info(candidate);
        if (info.exists() && info.isFile() && info.isExecutable())
            return info.absoluteFilePath();
    }
    return QString();
}

QString formatFilesystemToolPath(const QString &filesystemType)
{
    const QString normalized = normalizedFormatFilesystemType(filesystemType);
    if (normalized == QStringLiteral("exfat")) {
        return existingExecutablePath({
            QStringLiteral("/usr/sbin/mkfs.exfat"),
            QStringLiteral("/sbin/mkfs.exfat"),
            QStringLiteral("/usr/bin/mkfs.exfat"),
            QStringLiteral("/bin/mkfs.exfat")
        });
    }
    if (normalized == QStringLiteral("vfat")) {
        return existingExecutablePath({
            QStringLiteral("/usr/sbin/mkfs.vfat"),
            QStringLiteral("/sbin/mkfs.vfat"),
            QStringLiteral("/usr/sbin/mkdosfs"),
            QStringLiteral("/sbin/mkdosfs")
        });
    }
    if (normalized == QStringLiteral("ntfs")) {
        return existingExecutablePath({
            QStringLiteral("/usr/sbin/mkfs.ntfs"),
            QStringLiteral("/sbin/mkfs.ntfs"),
            QStringLiteral("/usr/sbin/mkntfs"),
            QStringLiteral("/sbin/mkntfs")
        });
    }
    return QString();
}

QString volumeLabelForFormat(QString label, const QString &filesystemType)
{
    label = label.trimmed();
    if (label.isEmpty())
        label = QStringLiteral("RAW");

    const QString normalized = normalizedFormatFilesystemType(filesystemType);
    if (normalized == QStringLiteral("vfat")) {
        QString sanitized;
        const QString upper = label.toUpper();
        for (const QChar ch : upper) {
            if (ch.isLetterOrNumber() || ch == QChar(' ') || ch == QChar('-') || ch == QChar('_'))
                sanitized.append(ch);
        }
        sanitized = sanitized.trimmed().left(11);
        return sanitized.isEmpty() ? QStringLiteral("RAW") : sanitized;
    }

    if (normalized == QStringLiteral("exfat"))
        return label.left(15);
    if (normalized == QStringLiteral("ext4"))
        return label.left(16);
    if (normalized == QStringLiteral("ntfs"))
        return label.left(128);

    return label;
}

}

MediaStatusBridge::MediaStatusBridge(QObject *parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, [this] {
        updateStatus();
        pollCoreWriteSpeedTest();
    });
    m_timer.start(1000);
}

void MediaStatusBridge::setStandby(bool standby)
{
    if (m_standby == standby) return;
    m_standby = standby;
    if (standby) m_timer.stop();
    else {
        m_driveScanTick = 0;
        updateStatus();
        m_timer.start(1000);
    }
}

void MediaStatusBridge::setDriveDiscoveryActive(bool active)
{
    if (m_driveDiscoveryActive == active)
        return;

    m_driveDiscoveryActive = active;
    m_driveScanTick = 0;
    if (m_driveDiscoveryActive && !m_standby)
        updateMountableDrives();
}

bool MediaStatusBridge::isMounted(const QString &path) const
{
    const QString wanted = path.endsWith('/')
        ? path.left(path.length() - 1)
        : path;

    const auto volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &storage : volumes) {
        QString rootPath = storage.rootPath();
        if (rootPath.endsWith('/'))
            rootPath.chop(1);

        if (rootPath == wanted && storage.isValid() && storage.isReady())
            return true;
    }

    return false;
}

void MediaStatusBridge::setMediaMounted(bool mounted)
{
    if (m_mediaMounted == mounted)
        return;

    m_mediaMounted = mounted;
    emit mediaMountedChanged();
    emit remainingMinutesTextChanged();
    emit remainingStillsChanged();

    const QString newStatus = mounted ? QStringLiteral("READY")
                                      : QStringLiteral("NO MEDIA");

    if (m_statusText != newStatus) {
        m_statusText = newStatus;
        emit statusTextChanged();
    }
}

void MediaStatusBridge::setLastActionError(const QString &errorText)
{
    if (m_lastActionError == errorText)
        return;

    m_lastActionError = errorText;
    emit lastActionErrorChanged();
}

void MediaStatusBridge::setFrameSizeMB(double value)
{
    if (qFuzzyCompare(m_frameSizeMB, value))
        return;

    m_frameSizeMB = value;
    emit frameSizeMBChanged();
    recalculateRemaining();
}

void MediaStatusBridge::setFps(double value)
{
    if (qFuzzyCompare(m_fps, value))
        return;

    m_fps = value;
    emit fpsChanged();
    recalculateRemaining();
}

void MediaStatusBridge::recalculateRemaining()
{
    qint64 newRemainingSeconds = 0;
    qulonglong newRemainingStills = 0;

    if (m_mediaMounted && m_frameSizeMB > 0.0 && m_fps > 0.0) {
        const double freeMB = static_cast<double>(m_freeBytes) / (1024.0 * 1024.0);
        const double mbPerSecond = m_frameSizeMB * m_fps;

        if (mbPerSecond > 0.0)
            newRemainingSeconds = static_cast<qint64>(freeMB / mbPerSecond);
    }

    if (m_mediaMounted && m_frameSizeMB > 0.0) {
        const double freeMB = static_cast<double>(m_freeBytes) / (1024.0 * 1024.0);
        newRemainingStills = static_cast<qulonglong>(freeMB / m_frameSizeMB);
    }

    if (m_remainingSeconds != newRemainingSeconds) {
        m_remainingSeconds = newRemainingSeconds;
        emit remainingSecondsChanged();
        emit remainingMinutesTextChanged();
    }

    if (m_remainingStills != newRemainingStills) {
        m_remainingStills = newRemainingStills;
        emit remainingStillsChanged();
    }
}

QString MediaStatusBridge::remainingMinutesText() const
{
    if (!m_mediaMounted)
        return QStringLiteral("N/A");

    const qint64 minutes = m_remainingSeconds / 60;
    return QStringLiteral("%1 MIN").arg(minutes);
}

QString MediaStatusBridge::remainingStillsText() const
{
    if (!m_mediaMounted)
        return QStringLiteral("N/A");

    return m_remainingStills == 1
        ? QStringLiteral("1 STILL")
        : QStringLiteral("%1 STILLS").arg(m_remainingStills);
}

QString MediaStatusBridge::writeSpeedResultText() const
{
    if (!m_mediaMounted)
        return QStringLiteral("No Media");

    if (m_writeSpeedTestRunning)
        return QStringLiteral("Testing...");

    if (!m_writeSpeedError.isEmpty())
        return QStringLiteral("Test Failed");

    if (m_lastWriteSpeedMBps > 0.0) {
        const int precision = m_lastWriteSpeedMBps >= 100.0 ? 0 : 1;
        return QStringLiteral("%1 MB/s").arg(QString::number(m_lastWriteSpeedMBps, 'f', precision));
    }

    return QStringLiteral("Not Tested");
}

QString MediaStatusBridge::writeSpeedDetailText() const
{
    if (!m_mediaMounted)
        return QStringLiteral("Mount /media/RAW to run a media speed test.");

    const qint64 bytes = m_writeSpeedTestRunning ? m_currentWriteTestBytes : m_lastWriteTestBytes;
    const qint64 testMiB = bytes > 0 ? bytes / (1024 * 1024) : 0;

    if (m_writeSpeedTestRunning)
        return m_currentWriteTestUsedFio
               ? QStringLiteral("Running a %1 MB direct fio write test (QD32)...").arg(testMiB)
               : QStringLiteral("Running a %1 MB sequential write test...").arg(testMiB);

    if (!m_writeSpeedError.isEmpty())
        return m_writeSpeedError;

    if (m_lastWriteSpeedMBps > 0.0)
        return m_lastWriteTestUsedFio
               ? QStringLiteral("Measured with a %1 MB direct fio write test (QD32).").arg(testMiB)
               : QStringLiteral("Measured with a %1 MB sequential write test.").arg(testMiB);

    return QStringLiteral("Runs a temporary direct write test and removes the file afterwards.");
}

QString MediaStatusBridge::mediaTypeLabel() const
{
    return m_mediaMounted
        ? mediaTypeLabelForDeviceAndVolumeLabel(m_currentDevicePath, m_currentVolumeLabel)
        : QStringLiteral("Recording Media");
}

QString MediaStatusBridge::mediaPromptLabel() const
{
    return m_mediaMounted
        ? mediaPromptLabelForDeviceAndVolumeLabel(m_currentDevicePath, m_currentVolumeLabel)
        : QStringLiteral("recording media");
}

QString MediaStatusBridge::formatBytes(qint64 bytes) const
{
    static const QStringList units = {
        QStringLiteral("B"),
        QStringLiteral("KB"),
        QStringLiteral("MB"),
        QStringLiteral("GB"),
        QStringLiteral("TB")
    };

    double value = static_cast<double>(qMax<qint64>(0, bytes));
    int unitIndex = 0;
    while (value >= 1024.0 && unitIndex < units.size() - 1) {
        value /= 1024.0;
        ++unitIndex;
    }

    const int precision = value >= 100.0 || unitIndex == 0 ? 0 : 1;
    return QStringLiteral("%1 %2").arg(QString::number(value, 'f', precision), units.at(unitIndex));
}

QString MediaStatusBridge::volumeLabelForDevice(const QString &devicePath) const
{
    if (!devicePath.startsWith(QStringLiteral("/dev/")))
        return QString();

    QProcess process;
    process.start(QStringLiteral("lsblk"),
                  {QStringLiteral("-no"), QStringLiteral("LABEL"), devicePath});
    if (!process.waitForFinished(1500))
        return QString();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return QString();

    return QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
}

QString MediaStatusBridge::mountedDeviceForPath() const
{
    QProcess process;
    process.start(QStringLiteral("findmnt"),
                  {QStringLiteral("-n"),
                   QStringLiteral("-o"),
                   QStringLiteral("SOURCE"),
                   QStringLiteral("--mountpoint"),
                   m_mountPath});
    if (!process.waitForFinished(1500))
        return QString();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return QString();

    return QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
}

QString MediaStatusBridge::parentDriveForDevice(const QString &devicePath) const
{
    if (!devicePath.startsWith(QStringLiteral("/dev/")))
        return QString();

    QProcess process;
    process.start(QStringLiteral("lsblk"),
                  {QStringLiteral("-no"), QStringLiteral("PKNAME"), devicePath});
    if (!process.waitForFinished(1500))
        return QString();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return QString();

    const QString parentName = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    if (parentName.isEmpty())
        return QString();

    return QStringLiteral("/dev/%1").arg(parentName);
}

QVector<MediaStatusBridge::DriveCandidate> MediaStatusBridge::scanMountableDriveCandidates() const
{
    QProcess process;
    process.start(QStringLiteral("lsblk"),
                  {QStringLiteral("-P"),
                   QStringLiteral("-b"),
                   QStringLiteral("-o"),
                   QStringLiteral("NAME,PATH,TYPE,SIZE,FSTYPE,LABEL,MODEL,TRAN,RM,RO,MOUNTPOINT,PKNAME")});
    if (!process.waitForFinished(2000))
        return {};

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return {};

    QList<QMap<QString, QString>> rows;
    const QStringList lines = QString::fromLocal8Bit(process.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    rows.reserve(lines.size());
    for (const QString &line : lines) {
        const QMap<QString, QString> fields = parseLsblkPairs(line);
        if (!fields.isEmpty())
            rows.append(fields);
    }

    QSet<QString> systemDevices;
    const QString wantedMountPath = normalizedMountPath(m_mountPath);
    for (const QMap<QString, QString> &fields : rows) {
        const QString mountPoint = normalizedMountPath(fields.value(QStringLiteral("MOUNTPOINT")));
        if (mountPoint != QStringLiteral("/") && !mountPoint.startsWith(QStringLiteral("/boot")))
            continue;

        const QString path = fields.value(QStringLiteral("PATH"));
        if (path.isEmpty())
            continue;

        systemDevices.insert(path);
        const QString parentName = fields.value(QStringLiteral("PKNAME"));
        if (!parentName.isEmpty())
            systemDevices.insert(QStringLiteral("/dev/%1").arg(parentName));
    }

    QVector<DriveCandidate> out;
    QSet<QString> seenDevices;
    QSet<QString> seenDisplayNames;

    for (const QMap<QString, QString> &fields : rows) {
        const QString path = fields.value(QStringLiteral("PATH"));
        const QString type = fields.value(QStringLiteral("TYPE"));
        const QString fsType = fields.value(QStringLiteral("FSTYPE")).toLower();
        const QString mountPoint = normalizedMountPath(fields.value(QStringLiteral("MOUNTPOINT")));
        const QString parentName = fields.value(QStringLiteral("PKNAME"));
        const QString parentPath = parentName.isEmpty() ? path : QStringLiteral("/dev/%1").arg(parentName);

        if (path.isEmpty() || seenDevices.contains(path))
            continue;
        if (fields.value(QStringLiteral("RO")) == QStringLiteral("1"))
            continue;
        if (type != QStringLiteral("part") && !(type == QStringLiteral("disk") && !fsType.isEmpty()))
            continue;
        if (fsType.isEmpty())
            continue;
        if (systemDevices.contains(path) || systemDevices.contains(parentPath))
            continue;
        if (!mountPoint.isEmpty() && mountPoint != wantedMountPath)
            continue;

        QString baseName = fields.value(QStringLiteral("LABEL")).trimmed();
        if (baseName.isEmpty())
            baseName = fields.value(QStringLiteral("MODEL")).trimmed();
        if (baseName.isEmpty())
            baseName = path;

        const QString sizeText = formatBytes(fields.value(QStringLiteral("SIZE")).toLongLong());
        QString displayName = QStringLiteral("%1 • %2 • %3 • %4")
                                  .arg(baseName,
                                       path,
                                       fsType.toUpper(),
                                       sizeText);
        if (seenDisplayNames.contains(displayName))
            displayName += QStringLiteral(" • %1").arg(path);

        DriveCandidate candidate;
        candidate.devicePath = path;
        candidate.displayName = displayName;
        candidate.label = fields.value(QStringLiteral("LABEL")).trimmed();
        candidate.model = fields.value(QStringLiteral("MODEL")).trimmed();
        candidate.filesystemType = fsType;
        candidate.sizeText = sizeText;
        candidate.mountPoint = mountPoint;

        out.append(candidate);
        seenDevices.insert(path);
        seenDisplayNames.insert(displayName);
    }

    return out;
}

void MediaStatusBridge::updateMountableDrives()
{
    const QVector<DriveCandidate> candidates = scanMountableDriveCandidates();
    QVariantList newDrives;
    QStringList newOptions;
    newDrives.reserve(candidates.size());
    newOptions.reserve(candidates.size());

    for (const DriveCandidate &candidate : candidates) {
        QVariantMap entry;
        entry.insert(QStringLiteral("devicePath"), candidate.devicePath);
        entry.insert(QStringLiteral("displayName"), candidate.displayName);
        entry.insert(QStringLiteral("label"), candidate.label);
        entry.insert(QStringLiteral("model"), candidate.model);
        entry.insert(QStringLiteral("filesystemType"), candidate.filesystemType);
        entry.insert(QStringLiteral("sizeText"), candidate.sizeText);
        entry.insert(QStringLiteral("mountPoint"), candidate.mountPoint);
        newDrives.append(entry);
        newOptions.append(candidate.displayName);
    }

    if (m_mountableDrives == newDrives && m_mountableDriveOptions == newOptions)
        return;

    m_mountableDrives = newDrives;
    m_mountableDriveOptions = newOptions;
    emit mountableDrivesChanged();
}

void MediaStatusBridge::refreshMountableDrives()
{
    updateMountableDrives();
}

QString MediaStatusBridge::pciAddressForDevice(const QString &devicePath) const
{
    if (!devicePath.startsWith(QStringLiteral("/dev/")))
        return QString();

    QProcess process;
    process.start(QStringLiteral("udevadm"),
                  {QStringLiteral("info"),
                   QStringLiteral("-q"),
                   QStringLiteral("path"),
                   QStringLiteral("-n"),
                   devicePath});
    if (!process.waitForFinished(1500))
        return QString();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return QString();

    const QString sysPath = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    if (sysPath.isEmpty())
        return QString();

    const QRegularExpression pciPattern(QStringLiteral(R"((\d{4}:\d{2}:\d{2}\.\d))"));
    QRegularExpressionMatchIterator matchIt = pciPattern.globalMatch(sysPath);
    QString lastMatch;
    while (matchIt.hasNext())
        lastMatch = matchIt.next().captured(1);

    return lastMatch;
}

bool MediaStatusBridge::detachNvmeController(const QString &devicePath, QString *errorMessage)
{
    const QString pciAddress = pciAddressForDevice(devicePath);
    if (pciAddress.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Media was unmounted, but the NVMe controller path could not be resolved.");
        return false;
    }

    const QString removePath = QStringLiteral("/sys/bus/pci/devices/%1/remove").arg(pciAddress);
    if (!QFile::exists(removePath)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Media was unmounted, but the NVMe controller remove path was not found.");
        return false;
    }

    if (!runProcess(QStringLiteral("sudo"),
                    {QStringLiteral("-n"),
                     QStringLiteral("sh"),
                     QStringLiteral("-c"),
                     QStringLiteral("echo 1 > %1").arg(removePath)},
                    errorMessage)) {
        return false;
    }

    runProcess(QStringLiteral("udevadm"),
               {QStringLiteral("settle")},
               nullptr);
    return true;
}

bool MediaStatusBridge::startNvmeReinsertWatcher(QString *errorMessage)
{
    // The native service continuously tracks external unmounts and physical
    // insert edges. Keep the embedded Python watcher only as a compatibility
    // fallback for systems that have not installed the native helper yet.
    if (QFileInfo::exists(QStringLiteral("/run/apertar-cfe/ready")))
        return true;

    static const QString helperScript = QStringLiteral(R"PY(
import os
import re
import configparser
import pwd
import smbus
import subprocess
import time

I2C_BUS = 10
I2C_ADDR = 0x34
CFE_HAT_ENABLED = True
MOUNT_PATH = "/media/RAW"
PCIE_CONTROLLER = "1000110000.pcie"

def hardware_config_candidate_paths():
    candidates = []
    seen = set()

    def add_candidate(path):
        if not path or path in seen:
            return
        seen.add(path)
        candidates.append(path)

    env_path = os.environ.get("APERTAR_HARDWARE_CONFIG")
    add_candidate(env_path)
    add_candidate("/home/pi/apertar-hardware.conf")
    add_candidate("/boot/firmware/apertar-hardware.conf")
    add_candidate("/boot/apertar-hardware.conf")
    return candidates

def parse_config_int(raw_value, default_value):
    if raw_value is None:
        return default_value
    value = str(raw_value).strip()
    if not value:
        return default_value
    try:
        return int(value, 0)
    except ValueError:
        return default_value

def parse_config_bool(raw_value, default_value):
    if raw_value is None:
        return default_value
    value = str(raw_value).split("#", 1)[0].split(";", 1)[0].strip().lower()
    if value in ("1", "true", "yes", "on", "enabled"):
        return True
    if value in ("0", "false", "no", "off", "disabled"):
        return False
    return default_value

def load_hardware_config():
    global I2C_BUS, I2C_ADDR, CFE_HAT_ENABLED, MOUNT_PATH, PCIE_CONTROLLER

    parser = configparser.ConfigParser()
    parser.optionxform = str.lower

    config_path = next((path for path in hardware_config_candidate_paths() if path and os.path.isfile(path)), None)
    if not config_path:
        return

    try:
        parser.read(config_path)
    except OSError:
        return

    if parser.has_section("cfexpress_hat"):
        section = parser["cfexpress_hat"]
        CFE_HAT_ENABLED = parse_config_bool(section.get("enabled"), CFE_HAT_ENABLED)
        I2C_BUS = parse_config_int(section.get("bus", section.get("channel", section.get("i2c_bus"))), I2C_BUS)
        I2C_ADDR = parse_config_int(section.get("address", section.get("addr")), I2C_ADDR)

    if parser.has_section("storage"):
        section = parser["storage"]
        mount_path = section.get("mount_path", "").strip()
        if mount_path:
            MOUNT_PATH = mount_path
        pcie_controller = section.get("pcie_controller", "").strip()
        if pcie_controller:
            PCIE_CONTROLLER = pcie_controller

def run_command(arguments):
    return subprocess.run(arguments, text=True, capture_output=True, check=False)

def run_privileged_command(arguments):
    if os.geteuid() == 0:
        return run_command(arguments)
    return run_command(["sudo", "-n", *arguments])

def write_privileged_file(path, value):
    if os.geteuid() == 0:
        try:
            with open(path, "w", encoding="utf-8") as handle:
                handle.write(str(value))
            return True
        except OSError:
            return False

    return run_command(["sudo", "-n", "sh", "-c", f"echo {value} > {path}"]).returncode == 0

def read_insert_state(bus, timeout_seconds=0.5, default_state=True):
    deadline = time.monotonic() + timeout_seconds
    while time.monotonic() < deadline:
        try:
            data = bus.read_byte(I2C_ADDR)
        except OSError:
            time.sleep(0.1)
            continue
        if data != 0x69:
            return (data & 0x01) == 0x01
        time.sleep(0.1)
    return default_state

def get_filesystem_type(device_path):
    try:
        result = run_privileged_command(["blkid", "-s", "TYPE", "-o", "value", device_path])
        if result.returncode == 0:
            return result.stdout.strip()
    except Exception:
        pass
    return None

def find_nvme_partition():
    try:
        output = subprocess.check_output(
            "lsblk -rno NAME,TYPE | awk '$2==\"part\" && $1 ~ /^nvme[0-9]+n[0-9]+p[0-9]+$/ {print $1}'",
            shell=True,
            text=True
        ).strip().splitlines()
        if not output:
            return None

        def sort_key(name):
            match = re.search(r"nvme(\d+)n(\d+)p(\d+)$", name)
            if match:
                return tuple(map(int, match.groups()))
            return (9999, 9999, 9999)

        output.sort(key=sort_key)
        return "/dev/" + output[-1]
    except Exception:
        return None

def wait_for_nvme_partition(timeout_seconds=3.0, interval_seconds=0.15):
    deadline = time.monotonic() + timeout_seconds
    while time.monotonic() < deadline:
        device_path = find_nvme_partition()
        if device_path:
            return device_path
        time.sleep(interval_seconds)
    return None

def mount_partition(device_path):
    fs_type = get_filesystem_type(device_path)
    if not fs_type:
        return False

    if run_privileged_command(["mkdir", "-p", MOUNT_PATH]).returncode != 0:
        return False

    if fs_type == "ntfs":
        cmd = ["mount", "-t", "ntfs3", "-o", "uid=1000,gid=1000", device_path, MOUNT_PATH]
    elif fs_type == "ext4":
        cmd = ["mount", "-t", "ext4", device_path, MOUNT_PATH]
    elif fs_type == "exfat":
        cmd = ["mount", "-t", "exfat", "-o", "uid=1000,gid=1000", device_path, MOUNT_PATH]
    else:
        return False

    return run_privileged_command(cmd).returncode == 0

def reset_pcie_controller():
    controller_path = f"/sys/bus/platform/drivers/brcm-pcie/{PCIE_CONTROLLER}"
    if os.path.exists(controller_path):
        write_privileged_file("/sys/bus/platform/drivers/brcm-pcie/unbind", PCIE_CONTROLLER)

    write_privileged_file("/sys/bus/platform/drivers/brcm-pcie/bind", PCIE_CONTROLLER)
    if wait_for_nvme_partition(timeout_seconds=2.0):
        return

    write_privileged_file("/sys/bus/pci/rescan", 1)
    wait_for_nvme_partition(timeout_seconds=3.0)

    run_command(["udevadm", "settle"])

load_hardware_config()
if not CFE_HAT_ENABLED:
    raise SystemExit(0)
bus = smbus.SMBus(I2C_BUS)

for _ in range(600):
    if read_insert_state(bus):
        break
    time.sleep(0.1)
else:
    raise SystemExit(0)

for _ in range(600):
    if not read_insert_state(bus):
        break
    time.sleep(0.1)
else:
    raise SystemExit(0)

reset_pcie_controller()

device_path = find_nvme_partition() or wait_for_nvme_partition(timeout_seconds=3.0)
if device_path and mount_partition(device_path):
    raise SystemExit(0)
)PY");

    if (QProcess::startDetached(QStringLiteral("python3"),
                                {QStringLiteral("-c"), helperScript})) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral("Media was ejected, but the automatic remount watcher could not be started.");
    }
    return false;
}

bool MediaStatusBridge::mountDeviceAtMediaPath(const QString &devicePath,
                                               const QString &filesystemType,
                                               QString *errorMessage)
{
    if (!runProcess(QStringLiteral("sudo"),
                    {QStringLiteral("-n"),
                     QStringLiteral("mkdir"),
                     QStringLiteral("-p"),
                     m_mountPath},
                    errorMessage,
                    30000)) {
        return false;
    }

    const QString fsType = filesystemType.trimmed().toLower();
    QString mountType = fsType;
    QString mountOptions;

    if (fsType == QStringLiteral("ntfs")) {
        mountType = QStringLiteral("ntfs3");
        mountOptions = QStringLiteral("uid=1000,gid=1000,umask=0002");
    } else if (fsType == QStringLiteral("exfat")
               || fsType == QStringLiteral("vfat")
               || fsType == QStringLiteral("fat32")
               || fsType == QStringLiteral("fat")
               || fsType == QStringLiteral("msdos")) {
        mountOptions = QStringLiteral("uid=1000,gid=1000,umask=0002");
    }

    QStringList arguments = {
        QStringLiteral("-n"),
        QStringLiteral("mount")
    };
    if (!mountType.isEmpty())
        arguments << QStringLiteral("-t") << mountType;
    if (!mountOptions.isEmpty())
        arguments << QStringLiteral("-o") << mountOptions;
    arguments << devicePath << m_mountPath;

    if (fsType == QStringLiteral("ntfs")) {
        QString ntfs3Error;
        if (runProcess(QStringLiteral("sudo"), arguments, &ntfs3Error, 30000))
            return true;

        QStringList fallbackArguments = {
            QStringLiteral("-n"),
            QStringLiteral("mount"),
            QStringLiteral("-t"),
            QStringLiteral("ntfs-3g"),
            QStringLiteral("-o"),
            mountOptions,
            devicePath,
            m_mountPath
        };
        if (runProcess(QStringLiteral("sudo"), fallbackArguments, errorMessage, 30000))
            return true;

        if (errorMessage && errorMessage->isEmpty())
            *errorMessage = ntfs3Error;
        return false;
    }

    return runProcess(QStringLiteral("sudo"), arguments, errorMessage, 30000);
}

bool MediaStatusBridge::mountFormattedDevice(const QString &devicePath,
                                            const QString &filesystemType,
                                            QString *errorMessage)
{
    return mountDeviceAtMediaPath(devicePath, filesystemType, errorMessage);
}

void MediaStatusBridge::flushMedia(const QString &devicePath)
{
    // Ask the kernel to flush this filesystem specifically before unmounting,
    // then fall back to a global sync as a safety net.
    if (!runProcess(QStringLiteral("sync"),
                    {QStringLiteral("-f"), m_mountPath},
                    nullptr)) {
        runProcess(QStringLiteral("sync"), {}, nullptr);
    }

    if (!devicePath.isEmpty() && devicePath.startsWith(QStringLiteral("/dev/"))) {
        runProcess(QStringLiteral("sudo"),
                   {QStringLiteral("-n"),
                    QStringLiteral("blockdev"),
                    QStringLiteral("--flushbufs"),
                    devicePath},
                   nullptr);
    }
}

bool MediaStatusBridge::runProcess(const QString &program,
                                   const QStringList &arguments,
                                   QString *errorMessage,
                                   int timeoutMs)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted(1500)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Could not start %1.").arg(program);
        return false;
    }

    if (!process.waitForFinished(timeoutMs)) {
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

bool MediaStatusBridge::mountDrive(const QString &devicePath)
{
    setLastActionError(QString());
    const QString normalizedDevicePath = devicePath.trimmed();

    if (m_mediaMounted) {
        setLastActionError(QStringLiteral("Eject the currently mounted media before mounting another drive."));
        return false;
    }

    if (!normalizedDevicePath.startsWith(QStringLiteral("/dev/"))) {
        setLastActionError(QStringLiteral("Select a valid drive to mount."));
        return false;
    }

    QString filesystemType;
    const QVector<DriveCandidate> candidates = scanMountableDriveCandidates();
    for (const DriveCandidate &candidate : candidates) {
        if (candidate.devicePath == normalizedDevicePath) {
            filesystemType = candidate.filesystemType;
            break;
        }
    }

    if (filesystemType.isEmpty()) {
        setLastActionError(QStringLiteral("That drive is not available for camera media mounting."));
        updateMountableDrives();
        return false;
    }

    QString errorMessage;
    if (!mountDeviceAtMediaPath(normalizedDevicePath, filesystemType, &errorMessage)) {
        if (errorMessage.contains(QStringLiteral("password"), Qt::CaseInsensitive)
            || errorMessage.contains(QStringLiteral("permission denied"), Qt::CaseInsensitive)
            || errorMessage.contains(QStringLiteral("not allowed"), Qt::CaseInsensitive)) {
            errorMessage = QStringLiteral("Mounting this drive needs sudo permission for mkdir and mount.");
        } else if (errorMessage.contains(QStringLiteral("unknown filesystem"), Qt::CaseInsensitive)
                   || errorMessage.contains(QStringLiteral("wrong fs type"), Qt::CaseInsensitive)
                   || errorMessage.contains(QStringLiteral("bad option"), Qt::CaseInsensitive)) {
            errorMessage = QStringLiteral("The selected drive format is not supported by this Pi image.");
        }

        if (errorMessage.isEmpty())
            errorMessage = QStringLiteral("The selected drive could not be mounted.");
        setLastActionError(errorMessage);
        updateStatus();
        updateMountableDrives();
        return false;
    }

    updateStatus();
    updateMountableDrives();
    return true;
}

bool MediaStatusBridge::mountDriveByDisplayName(const QString &displayName)
{
    const QString wanted = displayName.trimmed();
    if (wanted.isEmpty()) {
        setLastActionError(QStringLiteral("Select a drive to mount."));
        return false;
    }

    updateMountableDrives();
    for (const QVariant &entryVariant : m_mountableDrives) {
        const QVariantMap entry = entryVariant.toMap();
        if (entry.value(QStringLiteral("displayName")).toString() == wanted)
            return mountDrive(entry.value(QStringLiteral("devicePath")).toString());
    }

    setLastActionError(QStringLiteral("That drive is no longer available."));
    updateMountableDrives();
    return false;
}

bool MediaStatusBridge::ejectMedia()
{
    QString error;
    const auto result = coreMediaCommand("eject_media", {}, &error, 60000);
    setLastActionError(error);
    updateStatus();
    return result.value("ok").toBool();
}

bool MediaStatusBridge::formatMedia(const QString &filesystemType)
{
    QString error;
    const auto result = coreMediaCommand("format_media", {{"filesystem", filesystemType}}, &error, 360000);
    setLastActionError(error);
    updateStatus();
    return result.value("ok").toBool();
}

bool MediaStatusBridge::startWriteSpeedTest()
{
    QString error;
    const auto result = coreMediaCommand("run_media_speed_test", {}, &error);
    m_writeSpeedError = error;
    m_writeSpeedTestRunning = result.value("ok").toBool();
    emit writeSpeedStateChanged();
    const bool started = m_writeSpeedTestRunning;
    if (started) pollCoreWriteSpeedTest();
    return started;
}

void MediaStatusBridge::pollCoreWriteSpeedTest()
{
    if (!m_writeSpeedTestRunning) return;
    QString error;
    const auto result = coreMediaCommand("get_media_test_state", {}, &error);
    if (!result.value("ok").toBool()) {
        m_writeSpeedTestRunning = false;
        m_writeSpeedError = error;
    } else {
        m_writeSpeedTestRunning = result.value("running").toBool();
        m_currentWriteTestBytes = qint64(result.value("bytes").toDouble());
        m_currentWriteTestUsedFio = result.value("used_fio").toBool();
        if (!m_writeSpeedTestRunning) {
            m_writeSpeedError = result.value("error").toString();
            m_lastWriteSpeedMBps = result.value("mbps").toDouble();
            m_lastWriteTestBytes = qint64(result.value("bytes").toDouble());
            m_lastWriteTestUsedFio = result.value("used_fio").toBool();
        }
    }
    emit writeSpeedStateChanged();
}

void MediaStatusBridge::updateStatus()
{
    if (m_standby) return;
    bool mounted = false;
    qulonglong newFreeBytes = 0;
    qulonglong newTotalBytes = 0;
    QString currentDevicePath;
    QString currentVolumeLabel;

    const QString wanted = m_mountPath.endsWith('/')
        ? m_mountPath.left(m_mountPath.length() - 1)
        : m_mountPath;

    const auto volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &storage : volumes) {
        QString rootPath = storage.rootPath();
        if (rootPath.endsWith('/'))
            rootPath.chop(1);

        if (rootPath == wanted && storage.isValid() && storage.isReady()) {
            mounted = true;
            newFreeBytes = storage.bytesAvailable();
            newTotalBytes = storage.bytesTotal();
            currentDevicePath = QString::fromLocal8Bit(storage.device()).trimmed();
            currentVolumeLabel = currentDevicePath == m_currentDevicePath
                ? m_currentVolumeLabel
                : volumeLabelForDevice(currentDevicePath);
            break;
        }
    }

    setMediaMounted(mounted);
    updateMediaInfo(currentDevicePath, currentVolumeLabel);

    if (m_freeBytes != newFreeBytes) {
        m_freeBytes = newFreeBytes;
        emit freeBytesChanged();
    }

    if (m_totalBytes != newTotalBytes) {
        m_totalBytes = newTotalBytes;
        emit totalBytesChanged();
    }

    recalculateRemaining();

    if (m_driveDiscoveryActive) {
        ++m_driveScanTick;
        if (m_driveScanTick >= 3) {
            m_driveScanTick = 0;
            updateMountableDrives();
        }
    }
}

void MediaStatusBridge::updateMediaInfo(const QString &devicePath, const QString &volumeLabel)
{
    if (m_currentDevicePath == devicePath && m_currentVolumeLabel == volumeLabel)
        return;

    m_currentDevicePath = devicePath;
    m_currentVolumeLabel = volumeLabel;
    emit mediaInfoChanged();
}
