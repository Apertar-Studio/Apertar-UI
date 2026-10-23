#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <QVariantList>

class WifiBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList networks READ networks NOTIFY networksChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(bool connecting READ connecting NOTIFY connectingChanged)
    Q_PROPERTY(bool wifiAvailable READ wifiAvailable NOTIFY statusChanged)
    Q_PROPERTY(bool wifiEnabled READ wifiEnabled NOTIFY statusChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString currentSsid READ currentSsid NOTIFY statusChanged)
    Q_PROPERTY(QString wifiDevice READ wifiDevice NOTIFY statusChanged)
    Q_PROPERTY(bool hotspotActive READ hotspotActive NOTIFY hotspotChanged)
    Q_PROPERTY(QString hotspotSsid READ hotspotSsid WRITE setHotspotSsid NOTIFY hotspotChanged)
    Q_PROPERTY(QString hotspotPassword READ hotspotPassword WRITE setHotspotPassword NOTIFY hotspotChanged)
    Q_PROPERTY(QString hotspotStatusText READ hotspotStatusText NOTIFY hotspotChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit WifiBridge(QObject *parent = nullptr);

    QVariantList networks() const { return m_networks; }
    bool scanning() const { return m_scanning; }
    bool connecting() const { return m_connecting; }
    bool wifiAvailable() const { return m_wifiAvailable; }
    bool wifiEnabled() const { return m_wifiEnabled; }
    QString statusText() const { return m_statusText; }
    QString currentSsid() const { return m_currentSsid; }
    QString wifiDevice() const { return m_wifiDevice; }
    bool hotspotActive() const { return m_hotspotActive; }
    QString hotspotSsid() const { return m_hotspotSsid; }
    void setHotspotSsid(const QString &ssid);
    QString hotspotPassword() const { return m_hotspotPassword; }
    void setHotspotPassword(const QString &password);
    QString hotspotStatusText() const { return m_hotspotStatusText; }
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE bool refresh();
    Q_INVOKABLE bool setWifiEnabled(bool enabled);
    Q_INVOKABLE bool connectToNetwork(const QString &ssid, const QString &password = QString());
    Q_INVOKABLE bool disconnectCurrent();
    Q_INVOKABLE bool startHotspot(const QString &ssid = QString(), const QString &password = QString());
    Q_INVOKABLE bool stopHotspot();

signals:
    void networksChanged();
    void scanningChanged();
    void connectingChanged();
    void statusChanged();
    void hotspotChanged();
    void lastErrorChanged();

private slots:
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleProcessError(QProcess::ProcessError error);
    void handleAutoHotspotTick();

private:
    enum class PendingOperation {
        None,
        RefreshRescan,
        RefreshList,
        SetWifiEnabled,
        Connect,
        Disconnect,
        StartHotspot,
        StopHotspot
    };

    void startProcess(PendingOperation operation, const QStringList &arguments);
    QStringList refreshRescanArguments() const;
    QStringList refreshListArguments() const;
    bool isNotAuthorizedError(const QString &text) const;
    bool queryWifiRadioEnabled(bool *known = nullptr) const;
    bool isApertarHotspotConnection(const QString &connectionName) const;
    bool hasNormalWifiConnection() const;
    void cleanupHotspotProfile() const;
    void setHotspotActive(bool active);
    void setHotspotStatusText(const QString &statusText);
    void updateDeviceStatus();
    void applyNetworkList(const QString &output);
    void setNetworks(const QVariantList &networks);
    void setScanning(bool scanning);
    void setConnecting(bool connecting);
    void setLastError(const QString &errorText);
    void updateStatus(const QString &wifiDevice,
                      bool wifiAvailable,
                      bool wifiEnabled,
                      const QString &stateText,
                      const QString &currentSsid,
                      bool hotspotActive = false);

    QProcess m_process;
    QTimer m_autoHotspotTimer;
    QTimer m_hotspotDelayTimer;
    QElapsedTimer m_noWifiTimer;
    QSettings m_settings;
    QVariantList m_networks;
    bool m_scanning = false;
    bool m_connecting = false;
    bool m_wifiAvailable = false;
    bool m_wifiEnabled = true;
    QString m_statusText = QStringLiteral("Wi-Fi unavailable");
    QString m_currentSsid;
    QString m_wifiDevice;
    bool m_hotspotActive = false;
    QString m_hotspotSsid = QStringLiteral("Apertar");
    QString m_hotspotPassword = QStringLiteral("apertarcam");
    QString m_hotspotStatusText = QStringLiteral("Hotspot off");
    QString m_lastError;
    qint64 m_autoHotspotDelayMs = 60000;
    qint64 m_lastAutoHotspotStatusRefreshMs = 0;
    PendingOperation m_pendingOperation = PendingOperation::None;
};
