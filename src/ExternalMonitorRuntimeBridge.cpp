#include "ExternalMonitorRuntimeBridge.hpp"
#include "MediaStatusBridge.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QQuickWindow>
#include <QVariant>
#include <QtGlobal>

namespace {

QString runtimeSocketPath()
{
    const QString configured = qEnvironmentVariable("APERTAR_MONITOR_RUNTIME_SOCKET").trimmed();
    return configured.isEmpty()
        ? QStringLiteral("/tmp/apertar-ui-monitor-state.sock")
        : configured;
}

} // namespace

ExternalMonitorRuntimeBridge::ExternalMonitorRuntimeBridge(Mode mode, QObject *parent)
    : QObject(parent),
      m_mode(mode),
      m_socketPath(runtimeSocketPath())
{
    if (m_mode == Mode::Publisher) {
        QLocalServer::removeServer(m_socketPath);
        m_server = new QLocalServer(this);
        connect(m_server, &QLocalServer::newConnection,
                this, &ExternalMonitorRuntimeBridge::acceptConnections);
        if (!m_server->listen(m_socketPath)) {
            qWarning("Could not create external-monitor runtime socket: %s",
                     qPrintable(m_server->errorString()));
        }
        return;
    }

    m_reconnectTimer.setInterval(500);
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout,
            this, &ExternalMonitorRuntimeBridge::connectSubscriber);
    connectSubscriber();
}

ExternalMonitorRuntimeBridge::~ExternalMonitorRuntimeBridge()
{
    if (m_server && m_server->isListening()) {
        m_server->close();
        QLocalServer::removeServer(m_socketPath);
    }
}

void ExternalMonitorRuntimeBridge::setMainWindow(QQuickWindow *window)
{
    if (m_mode != Mode::Publisher || m_mainWindow == window)
        return;

    if (m_mainWindow)
        disconnect(m_mainWindow, nullptr, this, nullptr);

    m_mainWindow = window;
    if (m_mainWindow) {
        connect(m_mainWindow, SIGNAL(currentPageChanged()),
                this, SLOT(captureMainWindowState()));
        connect(m_mainWindow, SIGNAL(powerButtonShutdownOpenChanged()),
                this, SLOT(captureMainWindowState()));
        connect(m_mainWindow, SIGNAL(powerButtonShutdownTriggeredChanged()),
                this, SLOT(captureMainWindowState()));
        connect(m_mainWindow, SIGNAL(powerButtonShutdownHoldActiveChanged()),
                this, SLOT(captureMainWindowState()));
        connect(m_mainWindow, SIGNAL(powerButtonShutdownHoldProgressChanged()),
                this, SLOT(captureMainWindowState()));
        connect(m_mainWindow, SIGNAL(powerButtonShutdownErrorTextChanged()),
                this, SLOT(captureMainWindowState()));
        connect(m_mainWindow, SIGNAL(monitorRecordingActiveChanged()),
                this, SLOT(captureMainWindowState()));
        connect(m_mainWindow, SIGNAL(monitorTimelapseActiveChanged()),
                this, SLOT(captureMainWindowState()));
        connect(m_mainWindow, SIGNAL(monitorTimecodeChanged()),
                this, SLOT(captureMainWindowState()));
    }

    captureMainWindowState();
}

void ExternalMonitorRuntimeBridge::setMediaBridge(MediaStatusBridge *mediaBridge)
{
    if (m_mode != Mode::Publisher || m_mediaBridge == mediaBridge)
        return;

    if (m_mediaBridge)
        disconnect(m_mediaBridge, nullptr, this, nullptr);

    m_mediaBridge = mediaBridge;
    if (m_mediaBridge) {
        connect(m_mediaBridge, &MediaStatusBridge::mediaMountedChanged,
                this, &ExternalMonitorRuntimeBridge::captureMediaState);
        connect(m_mediaBridge, &MediaStatusBridge::remainingMinutesTextChanged,
                this, &ExternalMonitorRuntimeBridge::captureMediaState);
        connect(m_mediaBridge, &MediaStatusBridge::remainingStillsChanged,
                this, &ExternalMonitorRuntimeBridge::captureMediaState);
    }

    captureMediaState();
}

void ExternalMonitorRuntimeBridge::acceptConnections()
{
    if (!m_server)
        return;

    while (m_server->hasPendingConnections()) {
        QLocalSocket *client = m_server->nextPendingConnection();
        if (!client)
            continue;

        m_clients.push_back(client);
        connect(client, &QLocalSocket::disconnected,
                client, &QObject::deleteLater);
        connect(client, &QObject::destroyed, this, [this, client]() {
            m_clients.removeAll(client);
        });
        client->write(serializedState());
        client->flush();
    }
}

void ExternalMonitorRuntimeBridge::captureMainWindowState()
{
    if (m_mode != Mode::Publisher || !m_mainWindow)
        return;

    const bool cameraPageActive =
        m_mainWindow->property("currentPage").toString() == QStringLiteral("camera");
    const bool shutdownPopupVisible =
        m_mainWindow->property("powerButtonShutdownOpen").toBool();
    const bool shutdownPopupTriggered =
        m_mainWindow->property("powerButtonShutdownTriggered").toBool();
    const bool shutdownHoldActive =
        m_mainWindow->property("powerButtonShutdownHoldActive").toBool();
    const double shutdownHoldProgress =
        m_mainWindow->property("powerButtonShutdownHoldProgress").toDouble();
    const QString shutdownErrorText =
        m_mainWindow->property("powerButtonShutdownErrorText").toString();
    const bool recordingActive =
        m_mainWindow->property("monitorRecordingActive").toBool();
    const bool timelapseActive =
        m_mainWindow->property("monitorTimelapseActive").toBool();
    const QString timecode =
        m_mainWindow->property("monitorTimecode").toString();

    const bool pageChanged = m_cameraPageActive != cameraPageActive;
    const bool shutdownChanged = m_shutdownPopupVisible != shutdownPopupVisible
        || m_shutdownPopupTriggered != shutdownPopupTriggered
        || m_shutdownHoldActive != shutdownHoldActive
        || !qFuzzyCompare(m_shutdownHoldProgress + 1.0, shutdownHoldProgress + 1.0)
        || m_shutdownErrorText != shutdownErrorText;
    const bool captureChanged = !m_captureStateAvailable
        || m_recordingActive != recordingActive
        || m_timelapseActive != timelapseActive
        || m_timecode != timecode;

    m_cameraPageActive = cameraPageActive;
    m_shutdownPopupVisible = shutdownPopupVisible;
    m_shutdownPopupTriggered = shutdownPopupTriggered;
    m_shutdownHoldActive = shutdownHoldActive;
    m_shutdownHoldProgress = shutdownHoldProgress;
    m_shutdownErrorText = shutdownErrorText;
    m_captureStateAvailable = true;
    m_recordingActive = recordingActive;
    m_timelapseActive = timelapseActive;
    m_timecode = timecode;

    if (pageChanged)
        emit cameraPageActiveChanged();
    if (shutdownChanged)
        emit shutdownStateChanged();
    if (captureChanged)
        emit captureStateChanged();
    if (pageChanged || shutdownChanged || captureChanged)
        publishState();
}

void ExternalMonitorRuntimeBridge::captureMediaState()
{
    if (m_mode != Mode::Publisher || !m_mediaBridge)
        return;

    const bool mediaMounted = m_mediaBridge->mediaMounted();
    const QString remainingMinutesText = m_mediaBridge->remainingMinutesText();
    const QString remainingStillsText = m_mediaBridge->remainingStillsText();
    if (m_mediaMounted == mediaMounted
        && m_remainingMinutesText == remainingMinutesText
        && m_remainingStillsText == remainingStillsText) {
        return;
    }

    m_mediaMounted = mediaMounted;
    m_remainingMinutesText = remainingMinutesText;
    m_remainingStillsText = remainingStillsText;
    emit mediaStateChanged();
    publishState();
}

void ExternalMonitorRuntimeBridge::connectSubscriber()
{
    if (m_mode != Mode::Subscriber)
        return;
    if (m_subscriber && m_subscriber->state() != QLocalSocket::UnconnectedState)
        return;

    if (m_subscriber)
        m_subscriber->deleteLater();

    m_subscriber = new QLocalSocket(this);
    connect(m_subscriber, &QLocalSocket::readyRead,
            this, &ExternalMonitorRuntimeBridge::readSubscriberMessages);
    connect(m_subscriber, &QLocalSocket::disconnected,
            this, &ExternalMonitorRuntimeBridge::scheduleReconnect);
    connect(m_subscriber, &QLocalSocket::errorOccurred,
            this, [this](QLocalSocket::LocalSocketError) { scheduleReconnect(); });
    m_subscriber->connectToServer(m_socketPath);
}

void ExternalMonitorRuntimeBridge::readSubscriberMessages()
{
    if (!m_subscriber)
        return;

    m_readBuffer.append(m_subscriber->readAll());
    while (true) {
        const int newline = m_readBuffer.indexOf('\n');
        if (newline < 0)
            break;

        const QByteArray line = m_readBuffer.left(newline).trimmed();
        m_readBuffer.remove(0, newline + 1);
        if (!line.isEmpty())
            applyState(line);
    }
}

QByteArray ExternalMonitorRuntimeBridge::serializedState() const
{
    QJsonObject state;
    state.insert(QStringLiteral("cameraPageActive"), m_cameraPageActive);
    state.insert(QStringLiteral("shutdownPopupVisible"), m_shutdownPopupVisible);
    state.insert(QStringLiteral("shutdownPopupTriggered"), m_shutdownPopupTriggered);
    state.insert(QStringLiteral("shutdownHoldActive"), m_shutdownHoldActive);
    state.insert(QStringLiteral("shutdownHoldProgress"), m_shutdownHoldProgress);
    state.insert(QStringLiteral("shutdownErrorText"), m_shutdownErrorText);
    state.insert(QStringLiteral("mediaMounted"), m_mediaMounted);
    state.insert(QStringLiteral("remainingMinutesText"), m_remainingMinutesText);
    state.insert(QStringLiteral("remainingStillsText"), m_remainingStillsText);
    state.insert(QStringLiteral("captureStateAvailable"), m_captureStateAvailable);
    state.insert(QStringLiteral("recordingActive"), m_recordingActive);
    state.insert(QStringLiteral("timelapseActive"), m_timelapseActive);
    state.insert(QStringLiteral("timecode"), m_timecode);
    return QJsonDocument(state).toJson(QJsonDocument::Compact) + '\n';
}

void ExternalMonitorRuntimeBridge::publishState()
{
    if (m_clients.isEmpty())
        return;

    const QByteArray payload = serializedState();
    for (auto it = m_clients.begin(); it != m_clients.end();) {
        QLocalSocket *client = it->data();
        if (!client || client->state() != QLocalSocket::ConnectedState) {
            it = m_clients.erase(it);
            continue;
        }

        client->write(payload);
        client->flush();
        ++it;
    }
}

void ExternalMonitorRuntimeBridge::applyState(const QByteArray &line)
{
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(line, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject())
        return;

    const QJsonObject state = document.object();
    const bool cameraPageActive = state.value(QStringLiteral("cameraPageActive")).toBool(true);
    const bool shutdownPopupVisible = state.value(QStringLiteral("shutdownPopupVisible")).toBool();
    const bool shutdownPopupTriggered = state.value(QStringLiteral("shutdownPopupTriggered")).toBool();
    const bool shutdownHoldActive = state.value(QStringLiteral("shutdownHoldActive")).toBool();
    const double shutdownHoldProgress = state.value(QStringLiteral("shutdownHoldProgress")).toDouble();
    const QString shutdownErrorText = state.value(QStringLiteral("shutdownErrorText")).toString();
    const bool mediaMounted = state.value(QStringLiteral("mediaMounted")).toBool();
    const QString remainingMinutesText =
        state.value(QStringLiteral("remainingMinutesText")).toString(QStringLiteral("N/A"));
    const QString remainingStillsText =
        state.value(QStringLiteral("remainingStillsText")).toString(QStringLiteral("N/A"));
    const bool captureStateAvailable =
        state.value(QStringLiteral("captureStateAvailable")).toBool(
            state.contains(QStringLiteral("recordingActive")));
    const bool recordingActive = state.value(QStringLiteral("recordingActive")).toBool();
    const bool timelapseActive = state.value(QStringLiteral("timelapseActive")).toBool();
    const QString timecode = state.value(QStringLiteral("timecode")).toString();

    const bool pageChanged = m_cameraPageActive != cameraPageActive;
    const bool shutdownChanged = m_shutdownPopupVisible != shutdownPopupVisible
        || m_shutdownPopupTriggered != shutdownPopupTriggered
        || m_shutdownHoldActive != shutdownHoldActive
        || !qFuzzyCompare(m_shutdownHoldProgress + 1.0, shutdownHoldProgress + 1.0)
        || m_shutdownErrorText != shutdownErrorText;
    const bool mediaChanged = m_mediaMounted != mediaMounted
        || m_remainingMinutesText != remainingMinutesText
        || m_remainingStillsText != remainingStillsText;
    const bool captureChanged = m_captureStateAvailable != captureStateAvailable
        || m_recordingActive != recordingActive
        || m_timelapseActive != timelapseActive
        || m_timecode != timecode;

    m_cameraPageActive = cameraPageActive;
    m_shutdownPopupVisible = shutdownPopupVisible;
    m_shutdownPopupTriggered = shutdownPopupTriggered;
    m_shutdownHoldActive = shutdownHoldActive;
    m_shutdownHoldProgress = shutdownHoldProgress;
    m_shutdownErrorText = shutdownErrorText;
    m_mediaMounted = mediaMounted;
    m_remainingMinutesText = remainingMinutesText;
    m_remainingStillsText = remainingStillsText;
    m_captureStateAvailable = captureStateAvailable;
    m_recordingActive = recordingActive;
    m_timelapseActive = timelapseActive;
    m_timecode = timecode;

    if (pageChanged)
        emit cameraPageActiveChanged();
    if (shutdownChanged)
        emit shutdownStateChanged();
    if (mediaChanged)
        emit mediaStateChanged();
    if (captureChanged)
        emit captureStateChanged();
}

void ExternalMonitorRuntimeBridge::scheduleReconnect()
{
    if (m_mode == Mode::Subscriber && m_captureStateAvailable) {
        m_captureStateAvailable = false;
        emit captureStateChanged();
    }
    if (m_mode == Mode::Subscriber && !m_reconnectTimer.isActive())
        m_reconnectTimer.start();
}
