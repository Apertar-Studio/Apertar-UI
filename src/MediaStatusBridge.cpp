#include "MediaStatusBridge.hpp"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QStorageInfo>

MediaStatusBridge::MediaStatusBridge(QObject *parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &MediaStatusBridge::updateStatus);
    connect(&m_writeSpeedProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &MediaStatusBridge::handleWriteSpeedFinished);
    m_timer.start(1000);
    updateStatus();
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

    if (m_mediaMounted && m_frameSizeMB > 0.0 && m_fps > 0.0) {
        const double freeMB = static_cast<double>(m_freeBytes) / (1024.0 * 1024.0);
        const double mbPerSecond = m_frameSizeMB * m_fps;

        if (mbPerSecond > 0.0)
            newRemainingSeconds = static_cast<qint64>(freeMB / mbPerSecond);
    }

    if (m_remainingSeconds != newRemainingSeconds) {
        m_remainingSeconds = newRemainingSeconds;
        emit remainingSecondsChanged();
        emit remainingMinutesTextChanged();
    }
}

QString MediaStatusBridge::remainingMinutesText() const
{
    if (!m_mediaMounted)
        return QStringLiteral("N/A");

    const qint64 minutes = m_remainingSeconds / 60;
    return QStringLiteral("%1 MIN").arg(minutes);
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

QString MediaStatusBridge::mountedDeviceForPath() const
{
    QProcess process;
    process.start(QStringLiteral("findmnt"),
                  {QStringLiteral("-n"),
                   QStringLiteral("-o"),
                   QStringLiteral("SOURCE"),
                   QStringLiteral("--target"),
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

bool MediaStatusBridge::mountFormattedDevice(const QString &devicePath,
                                            const QString &filesystemType,
                                            QString *errorMessage)
{
    if (!runProcess(QStringLiteral("sudo"),
                    {QStringLiteral("-n"),
                     QStringLiteral("mkdir"),
                     QStringLiteral("-p"),
                     m_mountPath},
                    errorMessage)) {
        return false;
    }

    return runProcess(QStringLiteral("sudo"),
                      {QStringLiteral("-n"),
                       QStringLiteral("mount"),
                       QStringLiteral("-t"),
                       QStringLiteral("exfat"),
                       QStringLiteral("-o"),
                       QStringLiteral("uid=1000,gid=1000"),
                       devicePath,
                       m_mountPath},
                      errorMessage);
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

bool MediaStatusBridge::runProcess(const QString &program, const QStringList &arguments, QString *errorMessage)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted(1500)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Could not start %1.").arg(program);
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

bool MediaStatusBridge::ejectMedia()
{
    setLastActionError(QString());

    if (!m_mediaMounted) {
        setLastActionError(QStringLiteral("No media is currently mounted."));
        return false;
    }

    QString errorMessage;
    const QString devicePath = mountedDeviceForPath();
    flushMedia(devicePath);
    bool unmounted = false;
    QString udisksError;

    if (!devicePath.isEmpty() && devicePath.startsWith(QStringLiteral("/dev/"))) {
        unmounted = runProcess(QStringLiteral("udisksctl"),
                               {QStringLiteral("unmount"),
                                QStringLiteral("-b"),
                                devicePath,
                                QStringLiteral("--no-user-interaction")},
                               &udisksError);

        if (unmounted && !devicePath.startsWith(QStringLiteral("/dev/nvme"))) {
            const QString drivePath = parentDriveForDevice(devicePath);
            if (!drivePath.isEmpty()) {
                QString powerOffError;
                runProcess(QStringLiteral("udisksctl"),
                           {QStringLiteral("power-off"),
                            QStringLiteral("-b"),
                            drivePath,
                            QStringLiteral("--no-user-interaction")},
                           &powerOffError);
            }
        }
    }

    if (!unmounted) {
        QString umountError;
        unmounted = runProcess(QStringLiteral("umount"), {m_mountPath}, &umountError);

        if (!unmounted)
            errorMessage = !udisksError.isEmpty() ? udisksError : umountError;
    }

    updateStatus();

    if (!unmounted) {
        if (errorMessage.contains(QStringLiteral("Authentication is required"), Qt::CaseInsensitive)
            || errorMessage.contains(QStringLiteral("not authorized"), Qt::CaseInsensitive)
            || errorMessage.contains(QStringLiteral("another user"), Qt::CaseInsensitive)) {
            errorMessage = QStringLiteral("Media eject needs permission. Add a UDisks polkit rule for this user.");
        }

        if (errorMessage.isEmpty())
            errorMessage = QStringLiteral("Media could not be ejected.");
        setLastActionError(errorMessage);
        return false;
    }

    return true;
}

bool MediaStatusBridge::formatMedia()
{
    setLastActionError(QString());

    if (!m_mediaMounted) {
        setLastActionError(QStringLiteral("No media is currently mounted."));
        return false;
    }

    runProcess(QStringLiteral("sync"), {}, nullptr);

    const QString devicePath = mountedDeviceForPath();
    if (devicePath.isEmpty() || !devicePath.startsWith(QStringLiteral("/dev/"))) {
        setLastActionError(QStringLiteral("Could not determine which card device to format."));
        return false;
    }

    const QString filesystemType = QStringLiteral("exfat");

    QString errorMessage;
    QString udisksError;
    bool unmounted = runProcess(QStringLiteral("udisksctl"),
                                {QStringLiteral("unmount"),
                                 QStringLiteral("-b"),
                                 devicePath,
                                 QStringLiteral("--no-user-interaction")},
                                &udisksError);

    if (!unmounted) {
        QString umountError;
        unmounted = runProcess(QStringLiteral("umount"), {m_mountPath}, &umountError);
        if (!unmounted)
            errorMessage = !udisksError.isEmpty() ? udisksError : umountError;
    }

    if (!unmounted) {
        if (errorMessage.contains(QStringLiteral("Authentication is required"), Qt::CaseInsensitive)
            || errorMessage.contains(QStringLiteral("not authorized"), Qt::CaseInsensitive)
            || errorMessage.contains(QStringLiteral("another user"), Qt::CaseInsensitive)) {
            errorMessage = QStringLiteral("Formatting needs permission to unmount the card first.");
        }

        if (errorMessage.isEmpty())
            errorMessage = QStringLiteral("The card could not be unmounted for formatting.");
        setLastActionError(errorMessage);
        return false;
    }

    const bool formatted = runProcess(QStringLiteral("sudo"),
                                      {QStringLiteral("-n"),
                                       QStringLiteral("mkfs.exfat"),
                                       QStringLiteral("-n"),
                                       QStringLiteral("RAW"),
                                       devicePath},
                                      &errorMessage);

    if (!formatted) {
        if (errorMessage.contains(QStringLiteral("password"), Qt::CaseInsensitive)
            || errorMessage.contains(QStringLiteral("permission denied"), Qt::CaseInsensitive)
            || errorMessage.contains(QStringLiteral("not allowed"), Qt::CaseInsensitive)) {
            errorMessage = QStringLiteral("Formatting the card needs sudo permission.");
        } else if (errorMessage.contains(QStringLiteral("Could not start mkfs.exfat"), Qt::CaseInsensitive)) {
            errorMessage = QStringLiteral("mkfs.exfat is not installed on the Pi.");
        }

        if (errorMessage.isEmpty())
            errorMessage = QStringLiteral("The card could not be formatted.");
        setLastActionError(errorMessage);
        updateStatus();
        return false;
    }

    if (!mountFormattedDevice(devicePath, filesystemType, &errorMessage)) {
        if (errorMessage.contains(QStringLiteral("password"), Qt::CaseInsensitive)
            || errorMessage.contains(QStringLiteral("permission denied"), Qt::CaseInsensitive)
            || errorMessage.contains(QStringLiteral("not allowed"), Qt::CaseInsensitive)) {
            errorMessage = QStringLiteral("The card was formatted, but remounting it needs sudo permission.");
        }

        if (errorMessage.isEmpty())
            errorMessage = QStringLiteral("The card was formatted, but it could not be mounted again.");
        setLastActionError(errorMessage);
        updateStatus();
        return false;
    }

    updateStatus();
    return true;
}

bool MediaStatusBridge::startWriteSpeedTest()
{
    if (m_writeSpeedTestRunning) {
        m_writeSpeedError = QStringLiteral("A write speed test is already running.");
        emit writeSpeedStateChanged();
        return false;
    }

    if (!m_mediaMounted) {
        m_writeSpeedError = QStringLiteral("No media is currently mounted.");
        emit writeSpeedStateChanged();
        return false;
    }

    const qint64 freeMiB = static_cast<qint64>(m_freeBytes / (1024ull * 1024ull));
    if (freeMiB < 96) {
        m_writeSpeedError = QStringLiteral("Need at least 96 MB free to run the write speed test.");
        emit writeSpeedStateChanged();
        return false;
    }

    qint64 testMiB = 64;
    if (freeMiB >= 1152)
        testMiB = 1024;
    else if (freeMiB >= 640)
        testMiB = 512;
    else if (freeMiB >= 320)
        testMiB = 256;
    else if (freeMiB >= 160)
        testMiB = 128;

    m_writeSpeedError.clear();
    m_currentWriteTestBytes = testMiB * 1024 * 1024;
    m_writeSpeedTempPath = QDir(m_mountPath).filePath(QStringLiteral(".apertar_write_speed_test.bin"));
    QFile::remove(m_writeSpeedTempPath);

    m_currentWriteTestUsedFio = true;
    QString shellSafePath = m_writeSpeedTempPath;
    shellSafePath.replace('\'', QStringLiteral("'\"'\"'"));

    QStringList arguments = {
        QStringLiteral("-lc"),
        QStringLiteral("fio --name=write_test "
                       "--filename='%1' "
                       "--size=%2M "
                       "--bs=1M "
                       "--rw=write "
                       "--direct=1 "
                       "--numjobs=1 "
                       "--iodepth=32")
            .arg(shellSafePath)
            .arg(testMiB)
    };

    m_writeSpeedProcess.start(QStringLiteral("/bin/bash"), arguments);
    if (!m_writeSpeedProcess.waitForStarted(1500)) {
        m_writeSpeedProcess.close();
        m_currentWriteTestUsedFio = false;
        arguments = {
            QStringLiteral("if=/dev/zero"),
            QStringLiteral("of=%1").arg(m_writeSpeedTempPath),
            QStringLiteral("bs=8M"),
            QStringLiteral("count=%1").arg(qMax<qint64>(1, testMiB / 8)),
            QStringLiteral("conv=fdatasync"),
            QStringLiteral("status=none")
        };

        m_writeSpeedProcess.start(QStringLiteral("dd"), arguments);
        if (!m_writeSpeedProcess.waitForStarted(1500)) {
            m_writeSpeedError = QStringLiteral("Could not start the write speed test.");
            emit writeSpeedStateChanged();
            return false;
        }
    }

    m_writeSpeedTimer.start();
    m_writeSpeedTestRunning = true;
    emit writeSpeedStateChanged();
    return true;
}

void MediaStatusBridge::handleWriteSpeedFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const qint64 elapsedMs = qMax<qint64>(1, m_writeSpeedTimer.elapsed());
    const QString stdErr = QString::fromLocal8Bit(m_writeSpeedProcess.readAllStandardError()).trimmed();
    const QString stdOut = QString::fromLocal8Bit(m_writeSpeedProcess.readAllStandardOutput()).trimmed();

    m_writeSpeedTestRunning = false;

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        m_lastWriteTestBytes = m_currentWriteTestBytes;
        m_lastWriteTestUsedFio = m_currentWriteTestUsedFio;

        double measuredMBps = 0.0;
        bool parsedSpeed = false;

        const QString combinedOutput = stdOut + QStringLiteral("\n") + stdErr;

        if (m_currentWriteTestUsedFio) {
            QRegularExpression mbRegex(QStringLiteral("WRITE:\\s*bw=.*\\(([-+]?\\d+(?:\\.\\d+)?)MB/s\\)"));
            QRegularExpressionMatch mbMatch = mbRegex.match(combinedOutput);
            if (mbMatch.hasMatch()) {
                measuredMBps = mbMatch.captured(1).toDouble();
                parsedSpeed = measuredMBps > 0.0;
            }
        }

        if (!parsedSpeed && m_currentWriteTestUsedFio) {
            QRegularExpression mibRegex(QStringLiteral("WRITE:\\s*bw=([-+]?\\d+(?:\\.\\d+)?)MiB/s"));
            QRegularExpressionMatch mibMatch = mibRegex.match(combinedOutput);
            if (mibMatch.hasMatch()) {
                measuredMBps = mibMatch.captured(1).toDouble() * 1.048576;
                parsedSpeed = measuredMBps > 0.0;
            }
        }

        if (!parsedSpeed) {
            measuredMBps = (static_cast<double>(m_lastWriteTestBytes) / (1024.0 * 1024.0))
                           / (static_cast<double>(elapsedMs) / 1000.0);
        }

        m_lastWriteSpeedMBps = measuredMBps;
        m_writeSpeedError.clear();
    } else {
        m_writeSpeedError = !stdErr.isEmpty()
                            ? stdErr
                            : (!stdOut.isEmpty() ? stdOut
                                                 : QStringLiteral("The write speed test failed."));
    }

    QFile::remove(m_writeSpeedTempPath);
    runProcess(QStringLiteral("sync"), {}, nullptr);
    m_currentWriteTestUsedFio = false;
    updateStatus();
    emit writeSpeedStateChanged();
}

void MediaStatusBridge::updateStatus()
{
    bool mounted = false;
    qulonglong newFreeBytes = 0;
    qulonglong newTotalBytes = 0;

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
            break;
        }
    }

    setMediaMounted(mounted);

    if (m_freeBytes != newFreeBytes) {
        m_freeBytes = newFreeBytes;
        emit freeBytesChanged();
    }

    if (m_totalBytes != newTotalBytes) {
        m_totalBytes = newTotalBytes;
        emit totalBytesChanged();
    }

    recalculateRemaining();
}
