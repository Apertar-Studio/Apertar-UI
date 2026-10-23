#include "WifiBridge.hpp"

#include <QCollator>
#include <QDateTime>
#include <QHash>
#include <QFileInfo>
#include <QDebug>

#include <algorithm>

namespace {

using NmcliBlock = QHash<QString, QString>;

constexpr QLatin1String kHotspotConnectionName("Apertar Hotspot");
constexpr int kAutoHotspotCheckIntervalMs = 10000;
constexpr qint64 kAutoHotspotStatusRefreshIntervalMs = 30000;

qint64 autoHotspotDelayMs()
{
    QString path = qEnvironmentVariable("APERTAR_HARDWARE_CONFIG").trimmed();
    if (path.isEmpty())
        path = QStringLiteral("/home/pi/apertar-hardware.conf");
    QSettings config(path, QSettings::IniFormat);
    config.setFallbacksEnabled(false);
    bool ok = false;
    qint64 seconds = config.value(QStringLiteral("network/hotspot_delay_seconds"), 60)
                         .toString().trimmed().toLongLong(&ok);
    if (!ok || seconds < 0 || seconds > 86400) {
        qWarning() << "Apertar hotspot: invalid hotspot_delay_seconds; using 60 seconds";
        seconds = 60;
    }
    qInfo() << "Apertar hotspot fallback: delay_seconds=" << seconds << "config=" << path;
    return seconds * 1000;
}

QList<NmcliBlock> parseNmcliMultiline(const QString &output)
{
    QList<NmcliBlock> blocks;
    NmcliBlock current;

    const QStringList lines = output.split(QLatin1Char('\n'));
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty()) {
            if (!current.isEmpty()) {
                blocks.append(current);
                current.clear();
            }
            continue;
        }

        const int separator = line.indexOf(QLatin1Char(':'));
        if (separator <= 0)
            continue;

        const QString key = line.left(separator).trimmed();
        const QString value = line.mid(separator + 1).trimmed();

        if (current.contains(key) && !current.isEmpty()) {
            blocks.append(current);
            current.clear();
        }

        current.insert(key, value);
    }

    if (!current.isEmpty())
        blocks.append(current);

    return blocks;
}

QString connectionStateText(const QString &nmcliState, const QString &currentSsid)
{
    const QString normalized = nmcliState.trimmed().toLower();
    if (normalized.contains(QStringLiteral("connected")) && !currentSsid.isEmpty())
        return QStringLiteral("Connected");
    if (normalized == QStringLiteral("disconnected"))
        return QStringLiteral("Not connected");
    if (normalized == QStringLiteral("unavailable"))
        return QStringLiteral("Wi-Fi unavailable");
    if (normalized == QStringLiteral("unmanaged"))
        return QStringLiteral("Wi-Fi unmanaged");
    if (normalized.isEmpty())
        return QStringLiteral("Wi-Fi unavailable");

    QString pretty = nmcliState.trimmed();
    if (!pretty.isEmpty())
        pretty[0] = pretty.at(0).toUpper();
    return pretty;
}

QString nmcliErrorText(const QString &stderrText, const QString &stdoutText, const QString &fallback)
{
    const QString trimmedErr = stderrText.trimmed();
    if (!trimmedErr.isEmpty())
        return trimmedErr;

    const QString trimmedOut = stdoutText.trimmed();
    if (!trimmedOut.isEmpty())
        return trimmedOut;

    return fallback;
}

bool isWifiType(const QString &typeText)
{
    const QString normalized = typeText.trimmed().toLower();
    return normalized == QStringLiteral("wifi")
        || normalized == QStringLiteral("wi-fi")
        || normalized == QStringLiteral("wireless")
        || normalized == QStringLiteral("802-11-wireless")
        || normalized.contains(QStringLiteral("wifi"))
        || normalized.contains(QStringLiteral("wireless"));
}

QString fieldAt(const QStringList &fields, int index)
{
    return index >= 0 && index < fields.size() ? fields.at(index).trimmed() : QString();
}

} // namespace

WifiBridge::WifiBridge(QObject *parent)
    : QObject(parent)
    , m_settings(QStringLiteral("CinePi"), QStringLiteral("UI"))
{
    m_autoHotspotDelayMs = autoHotspotDelayMs();
    m_hotspotSsid = m_settings.value(QStringLiteral("hotspotSsid"), m_hotspotSsid).toString().trimmed();
    if (m_hotspotSsid.isEmpty())
        m_hotspotSsid = QStringLiteral("Apertar");

    m_hotspotPassword = m_settings.value(QStringLiteral("hotspotPassword"), m_hotspotPassword).toString();
    if (m_hotspotPassword.size() < 8)
        m_hotspotPassword = QStringLiteral("apertarcam");

    connect(&m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            &WifiBridge::handleProcessFinished);
    connect(&m_process,
            &QProcess::errorOccurred,
            this,
            &WifiBridge::handleProcessError);

    m_autoHotspotTimer.setInterval(kAutoHotspotCheckIntervalMs);
    connect(&m_autoHotspotTimer,
            &QTimer::timeout,
            this,
            &WifiBridge::handleAutoHotspotTick);
    m_hotspotDelayTimer.setSingleShot(true);
    m_hotspotDelayTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_hotspotDelayTimer, &QTimer::timeout, this, &WifiBridge::handleAutoHotspotTick);
    if (qEnvironmentVariableIsSet("APERTAR_NETWORK_HELPER")
        || !QFileInfo::exists(QStringLiteral("/run/apertar-network/managed")))
        m_autoHotspotTimer.start();

    // NetworkManager queries can block briefly during early boot. Let the
    // camera page present before doing the first synchronous nmcli refresh.
    QTimer::singleShot(1500, this, [this]() {
        if (qEnvironmentVariableIsSet("APERTAR_NETWORK_HELPER"))
            handleAutoHotspotTick();
        else
            refresh();
    });
}

bool WifiBridge::refresh()
{
    if (m_scanning || m_connecting)
        return false;

    setLastError(QString());
    updateDeviceStatus();
    if (!m_wifiEnabled) {
        setNetworks({});
        return true;
    }

    setScanning(true);
    startProcess(PendingOperation::RefreshRescan, refreshRescanArguments());
    return true;
}

bool WifiBridge::setWifiEnabled(bool enabled)
{
    if (m_scanning || m_connecting || m_wifiEnabled == enabled)
        return false;

    setLastError(QString());
    setConnecting(true);
    startProcess(PendingOperation::SetWifiEnabled,
                 {QStringLiteral("radio"),
                  QStringLiteral("wifi"),
                  enabled ? QStringLiteral("on") : QStringLiteral("off")});
    return true;
}

bool WifiBridge::connectToNetwork(const QString &ssid, const QString &password)
{
    if (ssid.trimmed().isEmpty() || m_scanning || m_connecting || !m_wifiEnabled)
        return false;

    setLastError(QString());
    setConnecting(true);

    QStringList arguments {
        QStringLiteral("--wait"),
        QStringLiteral("20"),
        QStringLiteral("device"),
        QStringLiteral("wifi"),
        QStringLiteral("connect"),
        ssid
    };

    const QString trimmedPassword = password.trimmed();
    if (!trimmedPassword.isEmpty()) {
        arguments << QStringLiteral("password") << trimmedPassword;
    }

    if (!m_wifiDevice.trimmed().isEmpty())
        arguments << QStringLiteral("ifname") << m_wifiDevice.trimmed();

    startProcess(PendingOperation::Connect, arguments);
    return true;
}

bool WifiBridge::disconnectCurrent()
{
    if (m_scanning || m_connecting || m_wifiDevice.trimmed().isEmpty() || !m_wifiEnabled)
        return false;

    setLastError(QString());
    setConnecting(true);
    startProcess(PendingOperation::Disconnect,
                 {QStringLiteral("--wait"),
                  QStringLiteral("15"),
                  QStringLiteral("device"),
                  QStringLiteral("disconnect"),
                  m_wifiDevice.trimmed()});
    return true;
}

void WifiBridge::setHotspotSsid(const QString &ssid)
{
    QString normalized = ssid.trimmed();
    if (m_hotspotSsid == normalized)
        return;

    m_hotspotSsid = normalized;
    m_settings.setValue(QStringLiteral("hotspotSsid"), m_hotspotSsid);
    emit hotspotChanged();
}

void WifiBridge::setHotspotPassword(const QString &password)
{
    if (m_hotspotPassword == password)
        return;

    m_hotspotPassword = password;
    m_settings.setValue(QStringLiteral("hotspotPassword"), m_hotspotPassword);
    emit hotspotChanged();
}

bool WifiBridge::startHotspot(const QString &ssid, const QString &password)
{
    if (m_scanning || m_connecting)
        return false;

    updateDeviceStatus();
    if (!m_wifiAvailable && m_wifiDevice.trimmed().isEmpty()) {
        setLastError(QStringLiteral("No Wi-Fi adapter is available for hotspot mode."));
        return false;
    }

    if (!m_wifiEnabled) {
        setLastError(QStringLiteral("Turn Wi-Fi on before starting a hotspot."));
        return false;
    }

    const QString selectedSsid = ssid.trimmed().isEmpty() ? m_hotspotSsid.trimmed() : ssid.trimmed();
    const QString selectedPassword = password.isEmpty() ? m_hotspotPassword : password;
    if (selectedSsid.isEmpty()) {
        setLastError(QStringLiteral("Hotspot name cannot be empty."));
        return false;
    }

    if (selectedPassword.size() < 8) {
        setLastError(QStringLiteral("Hotspot password must be at least 8 characters."));
        return false;
    }

    setHotspotSsid(selectedSsid);
    setHotspotPassword(selectedPassword);
    setLastError(QString());
    setConnecting(true);
    setHotspotStatusText(QStringLiteral("Starting hotspot..."));

    cleanupHotspotProfile();

    QStringList arguments {
        QStringLiteral("--wait"),
        QStringLiteral("20"),
        QStringLiteral("device"),
        QStringLiteral("wifi"),
        QStringLiteral("hotspot"),
        QStringLiteral("con-name"),
        QString(kHotspotConnectionName),
        QStringLiteral("ssid"),
        selectedSsid,
        QStringLiteral("password"),
        selectedPassword
    };

    if (!m_wifiDevice.trimmed().isEmpty())
        arguments << QStringLiteral("ifname") << m_wifiDevice.trimmed();

    startProcess(PendingOperation::StartHotspot, arguments);
    return true;
}

bool WifiBridge::stopHotspot()
{
    if (m_scanning || m_connecting)
        return false;

    setLastError(QString());
    setConnecting(true);
    setHotspotStatusText(QStringLiteral("Stopping hotspot..."));
    startProcess(PendingOperation::StopHotspot,
                 {QStringLiteral("--wait"),
                  QStringLiteral("15"),
                  QStringLiteral("connection"),
                  QStringLiteral("down"),
                  QString(kHotspotConnectionName)});
    return true;
}

void WifiBridge::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const PendingOperation operation = m_pendingOperation;
    m_pendingOperation = PendingOperation::None;

    const QString stdoutText = QString::fromLocal8Bit(m_process.readAllStandardOutput());
    const QString stderrText = QString::fromLocal8Bit(m_process.readAllStandardError());
    const bool success = exitStatus == QProcess::NormalExit && exitCode == 0;

    if (operation == PendingOperation::RefreshRescan) {
        if (!success)
            setLastError(isNotAuthorizedError(stderrText) || isNotAuthorizedError(stdoutText)
                         ? QStringLiteral("Wi-Fi scan needs permission on this system. Showing last known networks instead.")
                         : nmcliErrorText(stderrText, stdoutText, QStringLiteral("Wi-Fi scan failed.")));

        QTimer::singleShot(900, this, [this]() {
            if (!m_scanning || m_pendingOperation != PendingOperation::None)
                return;
            startProcess(PendingOperation::RefreshList, refreshListArguments());
        });
        return;
    }

    if (operation == PendingOperation::RefreshList) {
        if (success) {
            applyNetworkList(stdoutText);
            updateDeviceStatus();
            setLastError(QString());
        } else {
            setLastError(nmcliErrorText(stderrText, stdoutText, QStringLiteral("Wi-Fi scan failed.")));
            updateDeviceStatus();
        }
        setScanning(false);
        QTimer::singleShot(0, this, &WifiBridge::handleAutoHotspotTick);
        return;
    }

    if (operation == PendingOperation::SetWifiEnabled) {
        setConnecting(false);
        if (!success) {
            setLastError(nmcliErrorText(stderrText, stdoutText, QStringLiteral("Could not change Wi-Fi state.")));
        } else {
            setLastError(QString());
        }

        updateDeviceStatus();
        if (success && m_wifiEnabled) {
            refresh();
        } else if (success) {
            setNetworks({});
        }
        return;
    }

    if (operation == PendingOperation::Connect) {
        setConnecting(false);
        if (!success)
            setLastError(nmcliErrorText(stderrText, stdoutText, QStringLiteral("Could not connect to Wi-Fi.")));
        else
            setLastError(QString());

        updateDeviceStatus();
        refresh();
        return;
    }

    if (operation == PendingOperation::Disconnect) {
        setConnecting(false);
        if (!success)
            setLastError(nmcliErrorText(stderrText, stdoutText, QStringLiteral("Could not disconnect Wi-Fi.")));
        else
            setLastError(QString());

        updateDeviceStatus();
        refresh();
        return;
    }

    if (operation == PendingOperation::StartHotspot) {
        setConnecting(false);
        if (!success) {
            setLastError(nmcliErrorText(stderrText, stdoutText, QStringLiteral("Could not start hotspot.")));
            setHotspotStatusText(QStringLiteral("Hotspot failed"));
        } else {
            setLastError(QString());
        }

        updateDeviceStatus();
        if (success)
            setNetworks({});
        return;
    }

    if (operation == PendingOperation::StopHotspot) {
        setConnecting(false);
        if (!success) {
            setLastError(nmcliErrorText(stderrText, stdoutText, QStringLiteral("Could not stop hotspot.")));
        } else {
            setLastError(QString());
            setHotspotActive(false);
            setHotspotStatusText(QStringLiteral("Hotspot off"));
        }

        updateDeviceStatus();
        if (m_wifiEnabled)
            refresh();
        return;
    }
}

void WifiBridge::handleProcessError(QProcess::ProcessError error)
{
    if (error == QProcess::Crashed)
        return;

    const PendingOperation operation = m_pendingOperation;
    m_pendingOperation = PendingOperation::None;

    if (operation == PendingOperation::RefreshRescan || operation == PendingOperation::RefreshList)
        setScanning(false);
    else if (operation == PendingOperation::SetWifiEnabled
             || operation == PendingOperation::Connect
             || operation == PendingOperation::Disconnect
             || operation == PendingOperation::StartHotspot
             || operation == PendingOperation::StopHotspot)
        setConnecting(false);

    if (error == QProcess::FailedToStart) {
        setLastError(QStringLiteral("Could not start nmcli. Make sure NetworkManager is installed."));
    } else {
        setLastError(QStringLiteral("Wi-Fi command failed."));
    }

    updateDeviceStatus();
}

void WifiBridge::handleAutoHotspotTick()
{
    const bool networkHelper = qEnvironmentVariableIsSet("APERTAR_NETWORK_HELPER");
    if (!networkHelper && QFileInfo::exists(QStringLiteral("/run/apertar-network/managed"))) {
        m_autoHotspotTimer.stop();
        m_hotspotDelayTimer.stop();
        m_noWifiTimer.invalidate();
        return;
    }
    if (m_pendingOperation != PendingOperation::None || m_scanning || m_connecting)
        return;

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_lastAutoHotspotStatusRefreshMs <= 0
        || now - m_lastAutoHotspotStatusRefreshMs >= kAutoHotspotStatusRefreshIntervalMs) {
        m_lastAutoHotspotStatusRefreshMs = now;
        updateDeviceStatus();
    }

    if (networkHelper) {
        m_settings.sync();
        const QString ssid = m_settings.value(QStringLiteral("hotspotSsid"), QStringLiteral("Apertar")).toString().trimmed();
        const QString password = m_settings.value(QStringLiteral("hotspotPassword"), QStringLiteral("apertarcam")).toString();
        if (!ssid.isEmpty() && password.size() >= 8) {
            m_hotspotSsid = ssid;
            m_hotspotPassword = password;
        }
    }

    const bool adapterReady = m_wifiEnabled && (m_wifiAvailable || !m_wifiDevice.trimmed().isEmpty());
    const bool hotspotCanStart =
        adapterReady
        && !m_hotspotActive
        && !m_hotspotSsid.trimmed().isEmpty()
        && m_hotspotPassword.size() >= 8;

    if (hasNormalWifiConnection() || m_hotspotActive || !hotspotCanStart) {
        m_noWifiTimer.invalidate();
        m_hotspotDelayTimer.stop();
        return;
    }

    if (!m_noWifiTimer.isValid())
        m_noWifiTimer.start();
    const qint64 remaining = m_autoHotspotDelayMs - m_noWifiTimer.elapsed();
    if (remaining > 0) {
        if (!m_hotspotDelayTimer.isActive() || m_hotspotDelayTimer.remainingTime() > remaining)
            m_hotspotDelayTimer.start(static_cast<int>(remaining));
        return;
    }

    // Confirm the current state before replacing a station connection; a
    // successful connection may have arrived since the last periodic query.
    updateDeviceStatus();
    if (hasNormalWifiConnection() || m_hotspotActive || !m_wifiEnabled) {
        m_noWifiTimer.invalidate();
        m_hotspotDelayTimer.stop();
        return;
    }

    m_noWifiTimer.invalidate();
    m_hotspotDelayTimer.stop();
    startHotspot(m_hotspotSsid, m_hotspotPassword);
}

void WifiBridge::startProcess(PendingOperation operation, const QStringList &arguments)
{
    if (m_process.state() != QProcess::NotRunning)
        m_process.kill();

    m_pendingOperation = operation;
    m_process.start(QStringLiteral("nmcli"), arguments);
}

QStringList WifiBridge::refreshRescanArguments() const
{
    QStringList arguments {
        QStringLiteral("device"),
        QStringLiteral("wifi"),
        QStringLiteral("rescan")
    };

    if (!m_wifiDevice.trimmed().isEmpty())
        arguments << QStringLiteral("ifname") << m_wifiDevice.trimmed();

    return arguments;
}

QStringList WifiBridge::refreshListArguments() const
{
    QStringList arguments {
        QStringLiteral("-m"),
        QStringLiteral("multiline"),
        QStringLiteral("-f"),
        QStringLiteral("IN-USE,SSID,SIGNAL,SECURITY"),
        QStringLiteral("device"),
        QStringLiteral("wifi"),
        QStringLiteral("list"),
        QStringLiteral("--rescan"),
        QStringLiteral("no")
    };

    if (!m_wifiDevice.trimmed().isEmpty())
        arguments << QStringLiteral("ifname") << m_wifiDevice.trimmed();

    return arguments;
}

bool WifiBridge::isNotAuthorizedError(const QString &text) const
{
    const QString normalized = text.trimmed().toLower();
    return normalized.contains(QStringLiteral("not authorized"))
        || normalized.contains(QStringLiteral("permission denied"))
        || normalized.contains(QStringLiteral("org.freedesktop.networkmanager.wifi.scan"));
}

bool WifiBridge::queryWifiRadioEnabled(bool *known) const
{
    if (known)
        *known = false;

    QProcess process;
    process.start(QStringLiteral("nmcli"),
                  {QStringLiteral("-t"),
                   QStringLiteral("-e"),
                   QStringLiteral("no"),
                   QStringLiteral("radio"),
                   QStringLiteral("wifi")});

    if (!process.waitForFinished(3000))
        return m_wifiEnabled;

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return m_wifiEnabled;

    const QString state = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed().toLower();
    if (state.isEmpty())
        return m_wifiEnabled;

    if (known)
        *known = true;

    return state == QStringLiteral("enabled")
        || state == QStringLiteral("on");
}

bool WifiBridge::isApertarHotspotConnection(const QString &connectionName) const
{
    return connectionName.compare(QString(kHotspotConnectionName), Qt::CaseInsensitive) == 0;
}

bool WifiBridge::hasNormalWifiConnection() const
{
    return m_wifiEnabled
        && !m_hotspotActive
        && !m_currentSsid.trimmed().isEmpty()
        && m_statusText.compare(QStringLiteral("Connected"), Qt::CaseInsensitive) == 0;
}

void WifiBridge::cleanupHotspotProfile() const
{
    QProcess process;
    process.start(QStringLiteral("nmcli"),
                  {QStringLiteral("connection"),
                   QStringLiteral("delete"),
                   QString(kHotspotConnectionName)});
    process.waitForFinished(3000);
}

void WifiBridge::setHotspotActive(bool active)
{
    if (m_hotspotActive == active)
        return;

    m_hotspotActive = active;
    emit hotspotChanged();
}

void WifiBridge::setHotspotStatusText(const QString &statusText)
{
    if (m_hotspotStatusText == statusText)
        return;

    m_hotspotStatusText = statusText;
    emit hotspotChanged();
}

void WifiBridge::updateDeviceStatus()
{
    bool radioKnown = false;
    const bool radioEnabled = queryWifiRadioEnabled(&radioKnown);

    QProcess process;
    process.start(QStringLiteral("nmcli"),
                  {QStringLiteral("-t"),
                   QStringLiteral("-e"),
                   QStringLiteral("no"),
                   QStringLiteral("-f"),
                   QStringLiteral("DEVICE,TYPE,STATE,CONNECTION"),
                   QStringLiteral("device"),
                   QStringLiteral("status")});

    if (!process.waitForFinished(3000)) {
        updateStatus(QString(), false, radioKnown ? radioEnabled : m_wifiEnabled, QStringLiteral("Wi-Fi unavailable"), QString());
        return;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        updateStatus(QString(), false, radioKnown ? radioEnabled : m_wifiEnabled, QStringLiteral("Wi-Fi unavailable"), QString());
        return;
    }

    const QStringList lines = QString::fromLocal8Bit(process.readAllStandardOutput())
                                  .split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty())
            continue;

        const QStringList parts = line.split(QLatin1Char(':'));
        if (parts.size() < 4)
            continue;

        const QString device = fieldAt(parts, 0);
        const QString type = fieldAt(parts, 1);
        const QString state = fieldAt(parts, 2);
        QString currentConnection = parts.mid(3).join(QStringLiteral(":")).trimmed();
        if (!isWifiType(type))
            continue;

        if (currentConnection == QStringLiteral("--"))
            currentConnection.clear();

        const bool hotspotActive = isApertarHotspotConnection(currentConnection);
        QString stateText = hotspotActive
            ? QStringLiteral("Hotspot Active")
            : connectionStateText(state, currentConnection);
        const QString currentName = hotspotActive ? m_hotspotSsid : currentConnection;
        const bool wifiEnabled = radioKnown ? radioEnabled : true;
        if (!wifiEnabled && currentConnection.isEmpty())
            stateText = QStringLiteral("Wi-Fi Off");

        updateStatus(device,
                     wifiEnabled,
                     wifiEnabled,
                     stateText,
                     currentName,
                     hotspotActive);
        return;
    }

    for (const QVariant &entry : m_networks) {
        const QVariantMap network = entry.toMap();
        if (!network.value(QStringLiteral("active")).toBool())
            continue;

        const QString activeSsid = network.value(QStringLiteral("ssid")).toString();
        if (activeSsid.isEmpty())
            continue;

        updateStatus(QString(),
                     true,
                     radioKnown ? radioEnabled : true,
                     QStringLiteral("Connected"),
                     activeSsid);
        return;
    }

    if (radioKnown && !radioEnabled) {
        setHotspotActive(false);
        setHotspotStatusText(QStringLiteral("Hotspot off"));
        updateStatus(QString(), false, false, QStringLiteral("Wi-Fi Off"), QString());
        return;
    }

    updateStatus(QString(),
                 false,
                 radioKnown ? radioEnabled : true,
                 QStringLiteral("No Wi-Fi adapter detected"),
                 QString());
}

void WifiBridge::applyNetworkList(const QString &output)
{
    const QList<NmcliBlock> blocks = parseNmcliMultiline(output);

    QHash<QString, QVariantMap> mergedBySsid;
    for (const NmcliBlock &block : blocks) {
        QString ssid = block.value(QStringLiteral("SSID")).trimmed();
        if (ssid.isEmpty())
            continue;

        const bool active = block.value(QStringLiteral("IN-USE")).trimmed() == QStringLiteral("*");
        const int signal = block.value(QStringLiteral("SIGNAL")).trimmed().toInt();

        QString security = block.value(QStringLiteral("SECURITY")).trimmed();
        if (security.isEmpty() || security == QStringLiteral("--"))
            security = QStringLiteral("Open");

        QVariantMap existing = mergedBySsid.value(ssid);
        if (existing.isEmpty()) {
            QVariantMap network;
            network.insert(QStringLiteral("ssid"), ssid);
            network.insert(QStringLiteral("signal"), signal);
            network.insert(QStringLiteral("security"), security);
            network.insert(QStringLiteral("active"), active);
            network.insert(QStringLiteral("secured"), security.compare(QStringLiteral("Open"), Qt::CaseInsensitive) != 0);
            mergedBySsid.insert(ssid, network);
            continue;
        }

        if (signal > existing.value(QStringLiteral("signal")).toInt())
            existing.insert(QStringLiteral("signal"), signal);
        if (active)
            existing.insert(QStringLiteral("active"), true);
        if (existing.value(QStringLiteral("security")).toString().compare(QStringLiteral("Open"), Qt::CaseInsensitive) == 0
            && security.compare(QStringLiteral("Open"), Qt::CaseInsensitive) != 0) {
            existing.insert(QStringLiteral("security"), security);
            existing.insert(QStringLiteral("secured"), true);
        }

        mergedBySsid.insert(ssid, existing);
    }

    QList<QVariantMap> sortedNetworks = mergedBySsid.values();
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    collator.setNumericMode(true);

    std::sort(sortedNetworks.begin(), sortedNetworks.end(), [&collator](const QVariantMap &lhs, const QVariantMap &rhs) {
        const bool lhsActive = lhs.value(QStringLiteral("active")).toBool();
        const bool rhsActive = rhs.value(QStringLiteral("active")).toBool();
        if (lhsActive != rhsActive)
            return lhsActive;

        const int lhsSignal = lhs.value(QStringLiteral("signal")).toInt();
        const int rhsSignal = rhs.value(QStringLiteral("signal")).toInt();
        if (lhsSignal != rhsSignal)
            return lhsSignal > rhsSignal;

        return collator.compare(lhs.value(QStringLiteral("ssid")).toString(),
                                rhs.value(QStringLiteral("ssid")).toString()) < 0;
    });

    QVariantList networks;
    for (const QVariantMap &network : sortedNetworks)
        networks.append(network);

    setNetworks(networks);
}

void WifiBridge::setNetworks(const QVariantList &networks)
{
    if (m_networks == networks)
        return;

    m_networks = networks;
    emit networksChanged();
}

void WifiBridge::setScanning(bool scanning)
{
    if (m_scanning == scanning)
        return;

    m_scanning = scanning;
    emit scanningChanged();
}

void WifiBridge::setConnecting(bool connecting)
{
    if (m_connecting == connecting)
        return;

    m_connecting = connecting;
    emit connectingChanged();
}

void WifiBridge::setLastError(const QString &errorText)
{
    if (m_lastError == errorText)
        return;

    m_lastError = errorText;
    emit lastErrorChanged();
}

void WifiBridge::updateStatus(const QString &wifiDevice,
                              bool wifiAvailable,
                              bool wifiEnabled,
                              const QString &stateText,
                              const QString &currentSsid,
                              bool hotspotActive)
{
    bool changed = false;
    bool hotspotStateChanged = false;

    if (m_wifiDevice != wifiDevice) {
        m_wifiDevice = wifiDevice;
        changed = true;
    }

    if (m_wifiAvailable != wifiAvailable) {
        m_wifiAvailable = wifiAvailable;
        changed = true;
    }

    if (m_wifiEnabled != wifiEnabled) {
        m_wifiEnabled = wifiEnabled;
        changed = true;
    }

    if (m_statusText != stateText) {
        m_statusText = stateText;
        changed = true;
    }

    if (m_currentSsid != currentSsid) {
        m_currentSsid = currentSsid;
        changed = true;
    }

    if (m_hotspotActive != hotspotActive) {
        m_hotspotActive = hotspotActive;
        hotspotStateChanged = true;
    }

    const QString hotspotStatusText = hotspotActive
        ? QStringLiteral("Hotspot active")
        : QStringLiteral("Hotspot off");
    if (m_hotspotStatusText != hotspotStatusText) {
        m_hotspotStatusText = hotspotStatusText;
        hotspotStateChanged = true;
    }

    if (changed)
        emit statusChanged();

    if (hotspotStateChanged)
        emit hotspotChanged();
}
