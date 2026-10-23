#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVector>
#include <QElapsedTimer>
#include <QTimer>

class MediaStatusBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool mediaMounted READ mediaMounted NOTIFY mediaMountedChanged)
    Q_PROPERTY(QString mountPath READ mountPath CONSTANT)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString lastActionError READ lastActionError NOTIFY lastActionErrorChanged)

    Q_PROPERTY(double frameSizeMB READ frameSizeMB WRITE setFrameSizeMB NOTIFY frameSizeMBChanged)
    Q_PROPERTY(double fps READ fps WRITE setFps NOTIFY fpsChanged)
    Q_PROPERTY(qulonglong freeBytes READ freeBytes NOTIFY freeBytesChanged)
    Q_PROPERTY(qulonglong totalBytes READ totalBytes NOTIFY totalBytesChanged)
    Q_PROPERTY(qint64 remainingSeconds READ remainingSeconds NOTIFY remainingSecondsChanged)
    Q_PROPERTY(QString remainingMinutesText READ remainingMinutesText NOTIFY remainingMinutesTextChanged)
    Q_PROPERTY(qulonglong remainingStills READ remainingStills NOTIFY remainingStillsChanged)
    Q_PROPERTY(QString remainingStillsText READ remainingStillsText NOTIFY remainingStillsChanged)
    Q_PROPERTY(bool writeSpeedTestRunning READ writeSpeedTestRunning NOTIFY writeSpeedStateChanged)
    Q_PROPERTY(double lastWriteSpeedMBps READ lastWriteSpeedMBps NOTIFY writeSpeedStateChanged)
    Q_PROPERTY(QString writeSpeedResultText READ writeSpeedResultText NOTIFY writeSpeedStateChanged)
    Q_PROPERTY(QString writeSpeedDetailText READ writeSpeedDetailText NOTIFY writeSpeedStateChanged)
    Q_PROPERTY(QString writeSpeedError READ writeSpeedError NOTIFY writeSpeedStateChanged)
    Q_PROPERTY(QString mediaTypeLabel READ mediaTypeLabel NOTIFY mediaInfoChanged)
    Q_PROPERTY(QString mediaPromptLabel READ mediaPromptLabel NOTIFY mediaInfoChanged)
    Q_PROPERTY(QVariantList mountableDrives READ mountableDrives NOTIFY mountableDrivesChanged)
    Q_PROPERTY(QStringList mountableDriveOptions READ mountableDriveOptions NOTIFY mountableDrivesChanged)

public:
    explicit MediaStatusBridge(QObject *parent = nullptr);
    void setStandby(bool standby);

    bool mediaMounted() const { return m_mediaMounted; }
    QString mountPath() const { return m_mountPath; }
    QString statusText() const { return m_statusText; }
    QString lastActionError() const { return m_lastActionError; }

    double frameSizeMB() const { return m_frameSizeMB; }
    void setFrameSizeMB(double value);

    double fps() const { return m_fps; }
    void setFps(double value);

    qulonglong freeBytes() const { return m_freeBytes; }
    qulonglong totalBytes() const { return m_totalBytes; }
    qint64 remainingSeconds() const { return m_remainingSeconds; }
    QString remainingMinutesText() const;
    qulonglong remainingStills() const { return m_remainingStills; }
    QString remainingStillsText() const;
    bool writeSpeedTestRunning() const { return m_writeSpeedTestRunning; }
    double lastWriteSpeedMBps() const { return m_lastWriteSpeedMBps; }
    QString writeSpeedResultText() const;
    QString writeSpeedDetailText() const;
    QString writeSpeedError() const { return m_writeSpeedError; }
    QString mediaTypeLabel() const;
    QString mediaPromptLabel() const;
    QVariantList mountableDrives() const { return m_mountableDrives; }
    QStringList mountableDriveOptions() const { return m_mountableDriveOptions; }

    Q_INVOKABLE bool ejectMedia();
    Q_INVOKABLE bool formatMedia(const QString &filesystemType);
    Q_INVOKABLE bool startWriteSpeedTest();
    Q_INVOKABLE void refreshMountableDrives();
    Q_INVOKABLE void setDriveDiscoveryActive(bool active);
    Q_INVOKABLE bool mountDrive(const QString &devicePath);
    Q_INVOKABLE bool mountDriveByDisplayName(const QString &displayName);

signals:
    void mediaMountedChanged();
    void statusTextChanged();
    void lastActionErrorChanged();

    void frameSizeMBChanged();
    void fpsChanged();
    void freeBytesChanged();
    void totalBytesChanged();
    void remainingSecondsChanged();
    void remainingMinutesTextChanged();
    void remainingStillsChanged();
    void writeSpeedStateChanged();
    void mediaInfoChanged();
    void mountableDrivesChanged();

private slots:
    void updateStatus();
    void pollCoreWriteSpeedTest();

private:
    struct DriveCandidate
    {
        QString devicePath;
        QString displayName;
        QString label;
        QString model;
        QString filesystemType;
        QString sizeText;
        QString mountPoint;
    };

    bool isMounted(const QString &path) const;
    QString formatBytes(qint64 bytes) const;
    QString volumeLabelForDevice(const QString &devicePath) const;
    QString mountedDeviceForPath() const;
    QString parentDriveForDevice(const QString &devicePath) const;
    QString pciAddressForDevice(const QString &devicePath) const;
    bool detachNvmeController(const QString &devicePath, QString *errorMessage);
    bool startNvmeReinsertWatcher(QString *errorMessage);
    bool mountDeviceAtMediaPath(const QString &devicePath,
                                const QString &filesystemType,
                                QString *errorMessage);
    bool mountFormattedDevice(const QString &devicePath, const QString &filesystemType, QString *errorMessage);
    QVector<DriveCandidate> scanMountableDriveCandidates() const;
    void updateMountableDrives();
    void flushMedia(const QString &devicePath);
    bool runProcess(const QString &program,
                    const QStringList &arguments,
                    QString *errorMessage,
                    int timeoutMs = 5000);
    void setMediaMounted(bool mounted);
    void updateMediaInfo(const QString &devicePath, const QString &volumeLabel);
    void setLastActionError(const QString &errorText);
    void recalculateRemaining();

    QTimer m_timer;
    bool m_standby = false;
    bool m_mediaMounted = false;
    QString m_mountPath = QStringLiteral("/media/RAW");
    QString m_statusText = QStringLiteral("NO MEDIA");
    QString m_lastActionError;

    double m_frameSizeMB = 5.3;   // editable later
    double m_fps = 24.0;          // updated from QML
    qulonglong m_freeBytes = 0;
    qulonglong m_totalBytes = 0;
    qint64 m_remainingSeconds = 0;
    qulonglong m_remainingStills = 0;
    qint64 m_currentWriteTestBytes = 0;
    qint64 m_lastWriteTestBytes = 0;
    bool m_writeSpeedTestRunning = false;
    bool m_currentWriteTestUsedFio = false;
    bool m_lastWriteTestUsedFio = false;
    double m_lastWriteSpeedMBps = 0.0;
    QString m_writeSpeedError;
    QString m_currentDevicePath;
    QString m_currentVolumeLabel;
    QVariantList m_mountableDrives;
    QStringList m_mountableDriveOptions;
    int m_driveScanTick = 0;
    bool m_driveDiscoveryActive = false;
};
