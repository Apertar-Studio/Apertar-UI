#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QHash>
#include <QString>
#include <QTimer>

class GyroConfigBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString driver READ driver NOTIFY configChanged)
    Q_PROPERTY(QString syncMode READ syncMode NOTIFY configChanged)
    Q_PROPERTY(QString orientation READ orientation NOTIFY orientationChanged)
    Q_PROPERTY(double syncOffsetMs READ syncOffsetMs NOTIFY syncOffsetMsChanged)
    Q_PROPERTY(QString syncOffsetText READ syncOffsetText NOTIFY syncOffsetMsChanged)
    Q_PROPERTY(int sampleRateHz READ sampleRateHz NOTIFY configChanged)
    Q_PROPERTY(QString configPath READ configPath NOTIFY configChanged)
    Q_PROPERTY(bool livePollingEnabled READ livePollingEnabled WRITE setLivePollingEnabled NOTIFY livePollingEnabledChanged)
    Q_PROPERTY(int livePollingIntervalMs READ livePollingIntervalMs WRITE setLivePollingIntervalMs NOTIFY livePollingIntervalMsChanged)
    Q_PROPERTY(bool liveAccelOnly READ liveAccelOnly WRITE setLiveAccelOnly NOTIFY liveAccelOnlyChanged)
    Q_PROPERTY(bool liveAvailable READ liveAvailable NOTIFY liveSampleChanged)
    Q_PROPERTY(QString liveStatusText READ liveStatusText NOTIFY liveSampleChanged)
    Q_PROPERTY(double accelXG READ accelXG NOTIFY liveSampleChanged)
    Q_PROPERTY(double accelYG READ accelYG NOTIFY liveSampleChanged)
    Q_PROPERTY(double accelZG READ accelZG NOTIFY liveSampleChanged)
    Q_PROPERTY(double gyroXDps READ gyroXDps NOTIFY liveSampleChanged)
    Q_PROPERTY(double gyroYDps READ gyroYDps NOTIFY liveSampleChanged)
    Q_PROPERTY(double gyroZDps READ gyroZDps NOTIFY liveSampleChanged)
    Q_PROPERTY(int accelRangeG READ accelRangeG NOTIFY configChanged)
    Q_PROPERTY(int gyroRangeDps READ gyroRangeDps NOTIFY configChanged)
    Q_PROPERTY(bool levelReferenceSet READ levelReferenceSet NOTIFY levelReferenceChanged)
    Q_PROPERTY(QString levelReferenceText READ levelReferenceText NOTIFY levelReferenceChanged)
    Q_PROPERTY(double levelAccelXG READ levelAccelXG NOTIFY levelReferenceChanged)
    Q_PROPERTY(double levelAccelYG READ levelAccelYG NOTIFY levelReferenceChanged)
    Q_PROPERTY(double levelAccelZG READ levelAccelZG NOTIFY levelReferenceChanged)

public:
    explicit GyroConfigBridge(QObject *parent = nullptr);
    ~GyroConfigBridge() override;

    QString driver() const { return m_driver; }
    QString syncMode() const { return m_syncMode; }
    QString orientation() const { return m_orientation; }
    double syncOffsetMs() const { return m_syncOffsetMs; }
    QString syncOffsetText() const;
    int sampleRateHz() const { return m_sampleRateHz; }
    QString configPath() const { return m_configPath; }
    bool livePollingEnabled() const { return m_livePollingEnabled; }
    int livePollingIntervalMs() const { return m_livePollingIntervalMs; }
    bool liveAccelOnly() const { return m_liveAccelOnly; }
    bool liveAvailable() const { return m_liveAvailable; }
    QString liveStatusText() const { return m_liveStatusText; }
    double accelXG() const { return m_accelXG; }
    double accelYG() const { return m_accelYG; }
    double accelZG() const { return m_accelZG; }
    double gyroXDps() const { return m_gyroXDps; }
    double gyroYDps() const { return m_gyroYDps; }
    double gyroZDps() const { return m_gyroZDps; }
    int accelRangeG() const { return m_accelRangeG; }
    int gyroRangeDps() const { return m_gyroRangeDps; }
    bool levelReferenceSet() const { return m_levelReferenceSet; }
    QString levelReferenceText() const;
    double levelAccelXG() const { return m_levelAccelXG; }
    double levelAccelYG() const { return m_levelAccelYG; }
    double levelAccelZG() const { return m_levelAccelZG; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool setDriverName(const QString &driver);
    Q_INVOKABLE void setLivePollingEnabled(bool enabled);
    void setStandby(bool standby);
    Q_INVOKABLE void setLivePollingIntervalMs(int intervalMs);
    Q_INVOKABLE void setLiveAccelOnly(bool enabled);
    Q_INVOKABLE void setLivePollingRequest(const QString &requestId,
                                           bool enabled,
                                           bool accelOnly,
                                           int intervalMs);
    Q_INVOKABLE bool setLevelReference();
    Q_INVOKABLE bool clearLevelReference();
    Q_INVOKABLE bool setOrientation(const QString &orientation);
    Q_INVOKABLE bool setSyncOffsetMs(double value);
    Q_INVOKABLE bool adjustSyncOffsetMs(double delta);
    Q_INVOKABLE bool resetSyncOffset();

signals:
    void configChanged();
    void orientationChanged();
    void syncOffsetMsChanged();
    void livePollingEnabledChanged();
    void livePollingIntervalMsChanged();
    void liveAccelOnlyChanged();
    void liveSampleChanged();
    void levelReferenceChanged();
    void configSaveFinished(bool ok, const QString &message);

private:
    void loadConfiguration();
    bool ensureLiveDevice();
    bool probeLiveDevice();
    void closeLiveDevice();
    bool configureLiveDevice();
    bool writeLiveRegister(quint8 reg, quint8 value);
    bool readLiveRegisters(quint8 startRegister, quint8 *data, qsizetype length);
    void pollLiveSample();
    void setLiveAvailable(bool available, const QString &statusText);
    void setLiveSample(double accelXG, double accelYG, double accelZG,
                       double gyroXDps, double gyroYDps, double gyroZDps,
                       const QString &statusText);
    void applyLivePollingRequests();
    void resetLiveAccelFilter();
    double mappedLiveAccel(int index) const;
    void setLevelReferenceValue(bool enabled,
                                double accelXG,
                                double accelYG,
                                double accelZG,
                                const QString &orientation);
    bool writeConfigValue(const QString &section,
                          const QString &key,
                          const QString &value,
                          QString *error);
    void setConfigPath(const QString &value);
    void setDriver(const QString &value);
    void setSyncMode(const QString &value);
    void setOrientationValue(const QString &value);
    void setSyncOffsetValue(double value);
    void setSampleRateHz(int value);
    void setBusNumber(int value);
    void setAddress(int value);
    void setAccelRangeG(int value);
    void setGyroRangeDps(int value);

    QString m_configPath;
    QString m_driver = QStringLiteral("mpu6050");
    QString m_syncMode = QStringLiteral("free_run");
    QString m_orientation = QStringLiteral("XYZ");
    double m_syncOffsetMs = 0.0;
    int m_sampleRateHz = 500;
    int m_busNumber = 1;
    int m_address = 0x68;
    int m_accelRangeG = 8;
    int m_gyroRangeDps = 1000;
    bool m_configEnabled = true;

    QTimer m_liveTimer;
    QElapsedTimer m_liveRetryTimer;
    QElapsedTimer m_autoProbeTimer;
    struct LivePollingRequest {
        bool accelOnly = true;
        int intervalMs = 80;
    };
    QHash<QString, LivePollingRequest> m_livePollingRequests;
    bool m_standby = false;
    int m_livePollingIntervalMs = 80;
    int m_liveFd = -1;
    bool m_liveConfigured = false;
    bool m_livePollingEnabled = false;
    bool m_liveAccelOnly = false;
    bool m_liveAvailable = false;
    QString m_liveStatusText = QStringLiteral("Live gyro idle");
    double m_accelXG = 0.0;
    double m_accelYG = 0.0;
    double m_accelZG = 0.0;
    double m_gyroXDps = 0.0;
    double m_gyroYDps = 0.0;
    double m_gyroZDps = 0.0;
    bool m_liveAccelFilterValid = false;
    double m_liveFilteredAccelXG = 0.0;
    double m_liveFilteredAccelYG = 0.0;
    double m_liveFilteredAccelZG = 0.0;
    bool m_levelReferenceSet = false;
    QString m_levelReferenceOrientation;
    double m_levelAccelXG = 0.0;
    double m_levelAccelYG = 0.0;
    double m_levelAccelZG = 0.0;
};
