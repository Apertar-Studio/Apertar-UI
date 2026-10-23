#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>

class QLocalServer;
class QLocalSocket;
class QQuickWindow;
class MediaStatusBridge;

class ExternalMonitorRuntimeBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool cameraPageActive READ cameraPageActive NOTIFY cameraPageActiveChanged)
    Q_PROPERTY(bool shutdownPopupVisible READ shutdownPopupVisible NOTIFY shutdownStateChanged)
    Q_PROPERTY(bool shutdownPopupTriggered READ shutdownPopupTriggered NOTIFY shutdownStateChanged)
    Q_PROPERTY(bool shutdownHoldActive READ shutdownHoldActive NOTIFY shutdownStateChanged)
    Q_PROPERTY(double shutdownHoldProgress READ shutdownHoldProgress NOTIFY shutdownStateChanged)
    Q_PROPERTY(QString shutdownErrorText READ shutdownErrorText NOTIFY shutdownStateChanged)
    Q_PROPERTY(bool mediaMounted READ mediaMounted NOTIFY mediaStateChanged)
    Q_PROPERTY(QString remainingMinutesText READ remainingMinutesText NOTIFY mediaStateChanged)
    Q_PROPERTY(QString remainingStillsText READ remainingStillsText NOTIFY mediaStateChanged)
    Q_PROPERTY(bool captureStateAvailable READ captureStateAvailable NOTIFY captureStateChanged)
    Q_PROPERTY(bool recordingActive READ recordingActive NOTIFY captureStateChanged)
    Q_PROPERTY(bool timelapseActive READ timelapseActive NOTIFY captureStateChanged)
    Q_PROPERTY(QString timecode READ timecode NOTIFY captureStateChanged)

public:
    enum class Mode {
        Publisher,
        Subscriber
    };

    explicit ExternalMonitorRuntimeBridge(Mode mode, QObject *parent = nullptr);
    ~ExternalMonitorRuntimeBridge() override;

    bool cameraPageActive() const { return m_cameraPageActive; }
    bool shutdownPopupVisible() const { return m_shutdownPopupVisible; }
    bool shutdownPopupTriggered() const { return m_shutdownPopupTriggered; }
    bool shutdownHoldActive() const { return m_shutdownHoldActive; }
    double shutdownHoldProgress() const { return m_shutdownHoldProgress; }
    QString shutdownErrorText() const { return m_shutdownErrorText; }
    bool mediaMounted() const { return m_mediaMounted; }
    QString remainingMinutesText() const { return m_remainingMinutesText; }
    QString remainingStillsText() const { return m_remainingStillsText; }
    bool captureStateAvailable() const { return m_captureStateAvailable; }
    bool recordingActive() const { return m_recordingActive; }
    bool timelapseActive() const { return m_timelapseActive; }
    QString timecode() const { return m_timecode; }

    void setMainWindow(QQuickWindow *window);
    void setMediaBridge(MediaStatusBridge *mediaBridge);

signals:
    void cameraPageActiveChanged();
    void shutdownStateChanged();
    void mediaStateChanged();
    void captureStateChanged();

private slots:
    void acceptConnections();
    void captureMainWindowState();
    void captureMediaState();
    void connectSubscriber();
    void readSubscriberMessages();

private:
    QByteArray serializedState() const;
    void publishState();
    void applyState(const QByteArray &line);
    void scheduleReconnect();

    Mode m_mode;
    QString m_socketPath;
    QLocalServer *m_server = nullptr;
    QLocalSocket *m_subscriber = nullptr;
    QList<QPointer<QLocalSocket>> m_clients;
    QPointer<QQuickWindow> m_mainWindow;
    QPointer<MediaStatusBridge> m_mediaBridge;
    QTimer m_reconnectTimer;
    QByteArray m_readBuffer;
    bool m_cameraPageActive = true;
    bool m_shutdownPopupVisible = false;
    bool m_shutdownPopupTriggered = false;
    bool m_shutdownHoldActive = false;
    double m_shutdownHoldProgress = 0.0;
    QString m_shutdownErrorText;
    bool m_mediaMounted = false;
    QString m_remainingMinutesText = QStringLiteral("N/A");
    QString m_remainingStillsText = QStringLiteral("N/A");
    bool m_captureStateAvailable = false;
    bool m_recordingActive = false;
    bool m_timelapseActive = false;
    QString m_timecode;
};
