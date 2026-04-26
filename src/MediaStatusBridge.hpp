#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
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
    Q_PROPERTY(bool writeSpeedTestRunning READ writeSpeedTestRunning NOTIFY writeSpeedStateChanged)
    Q_PROPERTY(double lastWriteSpeedMBps READ lastWriteSpeedMBps NOTIFY writeSpeedStateChanged)
    Q_PROPERTY(QString writeSpeedResultText READ writeSpeedResultText NOTIFY writeSpeedStateChanged)
    Q_PROPERTY(QString writeSpeedDetailText READ writeSpeedDetailText NOTIFY writeSpeedStateChanged)
    Q_PROPERTY(QString writeSpeedError READ writeSpeedError NOTIFY writeSpeedStateChanged)

public:
    explicit MediaStatusBridge(QObject *parent = nullptr);

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
    bool writeSpeedTestRunning() const { return m_writeSpeedTestRunning; }
    double lastWriteSpeedMBps() const { return m_lastWriteSpeedMBps; }
    QString writeSpeedResultText() const;
    QString writeSpeedDetailText() const;
    QString writeSpeedError() const { return m_writeSpeedError; }

    Q_INVOKABLE bool ejectMedia();
    Q_INVOKABLE bool formatMedia();
    Q_INVOKABLE bool startWriteSpeedTest();

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
    void writeSpeedStateChanged();

private slots:
    void updateStatus();
    void handleWriteSpeedFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    bool isMounted(const QString &path) const;
    QString mountedDeviceForPath() const;
    QString parentDriveForDevice(const QString &devicePath) const;
    bool mountFormattedDevice(const QString &devicePath, const QString &filesystemType, QString *errorMessage);
    void flushMedia(const QString &devicePath);
    bool runProcess(const QString &program, const QStringList &arguments, QString *errorMessage);
    void setMediaMounted(bool mounted);
    void setLastActionError(const QString &errorText);
    void recalculateRemaining();

    QTimer m_timer;
    bool m_mediaMounted = false;
    QString m_mountPath = QStringLiteral("/media/RAW");
    QString m_statusText = QStringLiteral("NO MEDIA");
    QString m_lastActionError;

    double m_frameSizeMB = 5.3;   // editable later
    double m_fps = 24.0;          // updated from QML
    qulonglong m_freeBytes = 0;
    qulonglong m_totalBytes = 0;
    qint64 m_remainingSeconds = 0;
    QProcess m_writeSpeedProcess;
    QElapsedTimer m_writeSpeedTimer;
    QString m_writeSpeedTempPath;
    qint64 m_currentWriteTestBytes = 0;
    qint64 m_lastWriteTestBytes = 0;
    bool m_writeSpeedTestRunning = false;
    bool m_currentWriteTestUsedFio = false;
    bool m_lastWriteTestUsedFio = false;
    double m_lastWriteSpeedMBps = 0.0;
    QString m_writeSpeedError;
};
