#include "ApertarControlBridge.hpp"

#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QtGlobal>

namespace {

bool isAutoSelection(const QString &displayValue)
{
    return displayValue.trimmed().compare(QStringLiteral("Auto"), Qt::CaseInsensitive) == 0;
}

double snapToNearestPreset(double value, const double *presets, int presetCount, double tolerance)
{
    double closest = value;
    double smallestDelta = tolerance;

    for (int i = 0; i < presetCount; ++i) {
        const double delta = qAbs(value - presets[i]);
        if (delta <= smallestDelta) {
            smallestDelta = delta;
            closest = presets[i];
        }
    }

    return closest;
}

double normalizedShutterAngleForDisplay(double value)
{
    static const double kAnglePresets[] = {
        11.25, 15.0, 22.5, 30.0, 37.5, 45.0, 60.0, 72.0, 75.0, 90.0,
        108.0, 120.0, 144.0, 150.0, 172.8, 180.0, 216.0, 270.0, 324.0, 360.0
    };
    return snapToNearestPreset(value,
                               kAnglePresets,
                               static_cast<int>(sizeof(kAnglePresets) / sizeof(kAnglePresets[0])),
                               0.05);
}

double normalizedShutterSpeedForDisplay(double value)
{
    static const double kSpeedPresets[] = {
        24.0, 30.0, 40.0, 48.0, 50.0, 60.0, 80.0, 100.0,
        125.0, 160.0, 200.0, 250.0, 320.0, 500.0, 1000.0
    };
    return snapToNearestPreset(value,
                               kSpeedPresets,
                               static_cast<int>(sizeof(kSpeedPresets) / sizeof(kSpeedPresets[0])),
                               0.05);
}

}

ApertarControlBridge::ApertarControlBridge(QObject *parent)
    : QObject(parent)
    , m_settings(QStringLiteral("CinePi"), QStringLiteral("UI"))
{
    loadPersistedCameraSettings();

    m_retryTimer.setInterval(1000);
    connect(&m_retryTimer, &QTimer::timeout, this, &ApertarControlBridge::refreshFromCore);
    refreshFromCore();
    m_retryTimer.start();
}

QString ApertarControlBridge::fps() const
{
    return m_fps;
}

QString ApertarControlBridge::iso() const
{
    return m_iso;
}

QString ApertarControlBridge::shutterAngle() const
{
    return m_shutterAngle;
}

QString ApertarControlBridge::shutterSpeed() const
{
    return m_shutterSpeed;
}

QString ApertarControlBridge::whiteBalance() const
{
    return m_whiteBalance;
}

QString ApertarControlBridge::resolution() const
{
    return m_resolution;
}

bool ApertarControlBridge::recording() const
{
    return m_recording;
}

bool ApertarControlBridge::connected() const
{
    return m_connected;
}

QString ApertarControlBridge::lastError() const
{
    return m_lastError;
}

void ApertarControlBridge::refresh()
{
    refreshFromCore();
}

bool ApertarControlBridge::applyFps(const QString &displayValue)
{
    const double fpsValue = parseFpsValue(displayValue);
    if (fpsValue <= 0.0) {
        setLastError(QStringLiteral("Invalid FPS value."));
        return false;
    }

    // Preserve the user-selected shutter angle across FPS changes. The
    // set_fps reply reports the old shutter time in microseconds, which would
    // otherwise cause the bridge to recompute and preserve exposure time
    // instead of the chosen angle.
    const double preservedAngleValue = qMax(1.0, parseShutterAngleValue(m_shutterAngle));

    QJsonObject fpsReply;
    if (!sendCoreCommand(QStringLiteral("set_fps"), { { QStringLiteral("value"), fpsValue } }, &fpsReply))
        return false;

    setFps(formatFpsValue(fpsValue));
    if (m_shutterAuto) {
        setShutterAngle(QStringLiteral("Auto"));
        setShutterSpeed(QStringLiteral("Auto"));
        saveCurrentCameraSettings();
        return true;
    }

    const int shutterUs = shutterUsForAngle(preservedAngleValue, fpsValue);
    QJsonObject shutterReply;
    if (!sendCoreCommand(QStringLiteral("set_shutter_us"), { { QStringLiteral("value"), shutterUs } }, &shutterReply))
        return false;

    applyShutterFromMicroseconds(shutterUs, fpsValue);
    saveCurrentCameraSettings(shutterUs);
    return true;
}

bool ApertarControlBridge::applyIso(const QString &displayValue)
{
    if (isAutoSelection(displayValue)) {
        if (!sendCoreCommand(QStringLiteral("set_iso_auto")))
            return false;

        m_isoAuto = true;
        setIso(QStringLiteral("Auto"));
        saveCurrentCameraSettings();
        return true;
    }

    const double isoValue = parseIsoValue(displayValue);
    if (isoValue <= 0.0) {
        setLastError(QStringLiteral("Invalid ISO value."));
        return false;
    }

    if (!sendCoreCommand(QStringLiteral("set_iso"), { { QStringLiteral("value"), qRound64(isoValue) } }))
        return false;

    m_isoAuto = false;
    setIso(formatIsoValue(isoValue));
    saveCurrentCameraSettings();
    return true;
}

bool ApertarControlBridge::applyShutterAngle(const QString &displayValue)
{
    if (isAutoSelection(displayValue)) {
        if (!sendCoreCommand(QStringLiteral("set_shutter_auto")))
            return false;

        m_shutterAuto = true;
        setShutterAngle(QStringLiteral("Auto"));
        setShutterSpeed(QStringLiteral("Auto"));
        saveCurrentCameraSettings();
        return true;
    }

    const double angleValue = parseShutterAngleValue(displayValue);
    if (angleValue <= 0.0) {
        setLastError(QStringLiteral("Invalid shutter angle value."));
        return false;
    }

    const double fpsValue = currentFpsValue();
    const int shutterUs = shutterUsForAngle(angleValue, fpsValue);
    if (!sendCoreCommand(QStringLiteral("set_shutter_us"), { { QStringLiteral("value"), shutterUs } }))
        return false;

    m_shutterAuto = false;
    applyShutterFromMicroseconds(shutterUs, fpsValue);
    saveCurrentCameraSettings(shutterUs);
    return true;
}

bool ApertarControlBridge::applyShutterSpeed(const QString &displayValue)
{
    if (isAutoSelection(displayValue)) {
        if (!sendCoreCommand(QStringLiteral("set_shutter_auto")))
            return false;

        m_shutterAuto = true;
        setShutterAngle(QStringLiteral("Auto"));
        setShutterSpeed(QStringLiteral("Auto"));
        saveCurrentCameraSettings();
        return true;
    }

    const double speedValue = parseShutterSpeedValue(displayValue);
    if (speedValue <= 0.0) {
        setLastError(QStringLiteral("Invalid shutter speed value."));
        return false;
    }

    const int shutterUs = shutterUsForSpeed(speedValue);
    if (!sendCoreCommand(QStringLiteral("set_shutter_us"), { { QStringLiteral("value"), shutterUs } }))
        return false;

    m_shutterAuto = false;
    applyShutterFromMicroseconds(shutterUs, currentFpsValue());
    saveCurrentCameraSettings(shutterUs);
    return true;
}

bool ApertarControlBridge::applyWhiteBalance(const QString &displayValue)
{
    const double kelvin = parseWhiteBalanceValue(displayValue);
    if (kelvin <= 0.0) {
        setLastError(QStringLiteral("Invalid white balance value."));
        return false;
    }

    if (!sendCoreCommand(QStringLiteral("set_wb"), { { QStringLiteral("kelvin"), qRound(kelvin) } }))
        return false;

    setWhiteBalance(QString::number(qRound(kelvin)) + QStringLiteral("K"));
    saveCurrentCameraSettings();
    return true;
}

bool ApertarControlBridge::applyResolution(const QString &displayValue)
{
    int width = 0;
    int height = 0;
    if (!parseResolutionValue(displayValue, &width, &height)) {
        setLastError(QStringLiteral("Invalid resolution value."));
        return false;
    }

    if (!sendCoreCommand(QStringLiteral("set_resolution"), {
            { QStringLiteral("width"), width },
            { QStringLiteral("height"), height }
        })) {
        return false;
    }

    setResolution(formatResolutionValue(width, height));
    saveCurrentCameraSettings();
    return true;
}

bool ApertarControlBridge::setRecording(bool recording)
{
    if (!sendCoreCommand(recording ? QStringLiteral("record_start") : QStringLiteral("record_stop")))
        return false;

    setRecordingState(recording);
    return true;
}

bool ApertarControlBridge::capturePhoto()
{
    return sendCoreCommand(QStringLiteral("capture_photo"));
}

void ApertarControlBridge::refreshFromCore()
{
    if (m_pendingInitialSettingsSync) {
        if (!syncPersistedCameraSettingsToCore())
            return;

        m_pendingInitialSettingsSync = false;
    }

    sendCoreCommand(QStringLiteral("get_state"));
}

void ApertarControlBridge::loadPersistedCameraSettings()
{
    if (!m_settings.value(QStringLiteral("camera/hasSaved"), false).toBool())
        return;

    const double fpsValue = m_settings.value(QStringLiteral("camera/fps"), parseFpsValue(m_fps)).toDouble();
    if (fpsValue > 0.0)
        m_fps = formatFpsValue(fpsValue);

    m_isoAuto = m_settings.value(QStringLiteral("camera/isoAuto"), false).toBool();
    const double isoValue = m_settings.value(QStringLiteral("camera/iso"), parseIsoValue(m_iso)).toDouble();
    if (m_isoAuto) {
        m_iso = QStringLiteral("Auto");
    } else if (isoValue > 0.0) {
        m_iso = formatIsoValue(isoValue);
    }

    m_shutterAuto = m_settings.value(QStringLiteral("camera/shutterAuto"), false).toBool();
    const int shutterUs = m_settings.value(QStringLiteral("camera/shutterUs"), 0).toInt();
    if (m_shutterAuto) {
        m_shutterAngle = QStringLiteral("Auto");
        m_shutterSpeed = QStringLiteral("Auto");
    } else if (shutterUs > 0) {
        applyShutterFromMicroseconds(shutterUs, fpsValue > 0.0 ? fpsValue : currentFpsValue());
    }

    const int kelvin = m_settings.value(QStringLiteral("camera/whiteBalanceKelvin"), qRound(parseWhiteBalanceValue(m_whiteBalance))).toInt();
    if (kelvin > 0)
        m_whiteBalance = QString::number(kelvin) + QStringLiteral("K");

    const QString resolution = m_settings.value(QStringLiteral("camera/resolution"), m_resolution).toString();
    int width = 0;
    int height = 0;
    if (parseResolutionValue(resolution, &width, &height))
        m_resolution = formatResolutionValue(width, height);

    m_pendingInitialSettingsSync = true;
}

void ApertarControlBridge::saveCurrentCameraSettings(int shutterUsOverride)
{
    const double fpsValue = currentFpsValue();
    const double isoValue = parseIsoValue(m_iso);
    const double kelvin = parseWhiteBalanceValue(m_whiteBalance);
    const int shutterUs = shutterUsOverride > 0
        ? shutterUsOverride
        : shutterUsForAngle(qMax(1.0, parseShutterAngleValue(m_shutterAngle)), fpsValue);

    m_settings.setValue(QStringLiteral("camera/hasSaved"), true);
    m_settings.setValue(QStringLiteral("camera/fps"), fpsValue);
    m_settings.setValue(QStringLiteral("camera/isoAuto"), m_isoAuto);
    m_settings.setValue(QStringLiteral("camera/shutterAuto"), m_shutterAuto);
    if (!m_isoAuto && isoValue > 0.0)
        m_settings.setValue(QStringLiteral("camera/iso"), qRound64(isoValue));
    if (!m_shutterAuto && shutterUs > 0)
        m_settings.setValue(QStringLiteral("camera/shutterUs"), shutterUs);
    if (kelvin > 0.0)
        m_settings.setValue(QStringLiteral("camera/whiteBalanceKelvin"), qRound(kelvin));
    m_settings.setValue(QStringLiteral("camera/resolution"), m_resolution);
    m_settings.sync();
    m_pendingInitialSettingsSync = false;
}

bool ApertarControlBridge::syncPersistedCameraSettingsToCore()
{
    const double fpsValue = currentFpsValue();
    const double isoValue = parseIsoValue(m_iso);
    const int shutterUs = shutterUsForAngle(qMax(1.0, parseShutterAngleValue(m_shutterAngle)), fpsValue);
    const double kelvin = parseWhiteBalanceValue(m_whiteBalance);
    int width = 0;
    int height = 0;

    bool ok = true;
    m_syncingPersistedSettings = true;

    if (parseResolutionValue(m_resolution, &width, &height)) {
        ok = sendCoreCommand(QStringLiteral("set_resolution"), {
            { QStringLiteral("width"), width },
            { QStringLiteral("height"), height }
        });
    }

    if (ok)
        ok = sendCoreCommand(QStringLiteral("set_fps"), { { QStringLiteral("value"), fpsValue } });
    if (ok && m_isoAuto)
        ok = sendCoreCommand(QStringLiteral("set_iso_auto"));
    else if (ok)
        ok = sendCoreCommand(QStringLiteral("set_iso"), { { QStringLiteral("value"), qRound64(isoValue) } });
    if (ok && m_shutterAuto)
        ok = sendCoreCommand(QStringLiteral("set_shutter_auto"));
    else if (ok)
        ok = sendCoreCommand(QStringLiteral("set_shutter_us"), { { QStringLiteral("value"), shutterUs } });
    if (ok)
        ok = sendCoreCommand(QStringLiteral("set_wb"), { { QStringLiteral("kelvin"), qRound(kelvin) } });

    m_syncingPersistedSettings = false;
    return ok;
}

bool ApertarControlBridge::sendCoreCommand(const QString &commandName, const QJsonObject &arguments, QJsonObject *replyObject)
{
    constexpr int kConnectTimeoutMs = 350;
    constexpr int kReplyTimeoutMs = 3000;
    const QString socketPath = QStringLiteral("/tmp/apertar-core.sock");

    QLocalSocket socket;
    socket.connectToServer(socketPath);
    if (!socket.waitForConnected(kConnectTimeoutMs)) {
        setConnected(false);
        setLastError(QStringLiteral("Waiting for ApertarCore control socket..."));
        return false;
    }

    const int commandId = m_nextCommandId++;
    QJsonObject command = arguments;
    command.insert(QStringLiteral("cmd"), commandName);
    command.insert(QStringLiteral("id"), commandId);

    QByteArray payload = QJsonDocument(command).toJson(QJsonDocument::Compact);
    payload.append('\n');

    if (socket.write(payload) != payload.size() || !socket.waitForBytesWritten(500)) {
        setConnected(false);
        setLastError(QStringLiteral("Could not send command to ApertarCore."));
        return false;
    }

    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < kReplyTimeoutMs) {
        while (socket.canReadLine()) {
            const QByteArray line = socket.readLine().trimmed();
            if (line.isEmpty())
                continue;

            const QJsonDocument doc = QJsonDocument::fromJson(line);
            if (!doc.isObject())
                continue;

            const QJsonObject message = doc.object();
            applyCoreMessage(message);

            if (message.value(QStringLiteral("id")).toInt(-1) != commandId)
                continue;

            setConnected(true);
            if (replyObject)
                *replyObject = message;

            if (!message.value(QStringLiteral("ok")).toBool(false)) {
                setLastError(message.value(QStringLiteral("error")).toString(QStringLiteral("ApertarCore command failed.")));
                return false;
            }

            setLastError(QString());
            return true;
        }

        const int remaining = qMax(1, kReplyTimeoutMs - int(timer.elapsed()));
        if (!socket.waitForReadyRead(qMin(remaining, 250))) {
            if (socket.state() == QLocalSocket::UnconnectedState)
                break;
        }
    }

    setConnected(false);
    setLastError(QStringLiteral("Timed out waiting for ApertarCore reply."));
    return false;
}

void ApertarControlBridge::applyCoreMessage(const QJsonObject &message)
{
    const QString event = message.value(QStringLiteral("event")).toString();
    if (event == QStringLiteral("recording")) {
        setRecordingState(message.value(QStringLiteral("active")).toBool(false));
        return;
    }

    if (event == QStringLiteral("settings") || message.contains(QStringLiteral("ok")))
        applyCoreState(message);
}

void ApertarControlBridge::applyCoreState(const QJsonObject &state)
{
    if (m_syncingPersistedSettings)
        return;

    if (state.contains(QStringLiteral("iso_auto")))
        m_isoAuto = state.value(QStringLiteral("iso_auto")).toBool(m_isoAuto);

    if (state.contains(QStringLiteral("shutter_auto")))
        m_shutterAuto = state.value(QStringLiteral("shutter_auto")).toBool(m_shutterAuto);

    if (state.contains(QStringLiteral("fps")))
        setFps(formatFpsValue(state.value(QStringLiteral("fps")).toDouble(currentFpsValue())));

    if (m_isoAuto) {
        setIso(QStringLiteral("Auto"));
    } else if (state.contains(QStringLiteral("iso"))) {
        setIso(formatIsoValue(state.value(QStringLiteral("iso")).toDouble(parseIsoValue(m_iso))));
    }

    const double fpsValue = state.contains(QStringLiteral("fps"))
        ? state.value(QStringLiteral("fps")).toDouble(currentFpsValue())
        : currentFpsValue();
    if (m_shutterAuto) {
        setShutterAngle(QStringLiteral("Auto"));
        setShutterSpeed(QStringLiteral("Auto"));
    } else if (state.contains(QStringLiteral("shutter_us"))) {
        applyShutterFromMicroseconds(state.value(QStringLiteral("shutter_us")).toDouble(0.0), fpsValue);
    }

    if (state.contains(QStringLiteral("wb_kelvin"))) {
        const int kelvin = qRound(state.value(QStringLiteral("wb_kelvin")).toDouble(parseWhiteBalanceValue(m_whiteBalance)));
        if (kelvin > 0)
            setWhiteBalance(QString::number(kelvin) + QStringLiteral("K"));
    }

    if (state.contains(QStringLiteral("width")) && state.contains(QStringLiteral("height"))) {
        const int width = state.value(QStringLiteral("width")).toInt();
        const int height = state.value(QStringLiteral("height")).toInt();
        if (width > 0 && height > 0)
            setResolution(formatResolutionValue(width, height));
    } else if (state.contains(QStringLiteral("resolution"))) {
        int width = 0;
        int height = 0;
        const QString resolution = state.value(QStringLiteral("resolution")).toString();
        if (parseResolutionValue(resolution, &width, &height))
            setResolution(formatResolutionValue(width, height));
    }

    if (state.contains(QStringLiteral("recording")))
        setRecordingState(state.value(QStringLiteral("recording")).toBool(false));
}

void ApertarControlBridge::applyShutterFromMicroseconds(double shutterUs, double fps)
{
    if (shutterUs <= 0.0)
        return;

    m_shutterAuto = false;
    const double safeFps = fps > 0.0 ? fps : currentFpsValue();
    const double angle = (shutterUs * safeFps * 360.0) / 1000000.0;
    const double denominator = 1000000.0 / shutterUs;

    setShutterAngle(formatShutterAngleValue(angle));
    setShutterSpeed(formatShutterSpeedValue(denominator));
}

int ApertarControlBridge::shutterUsForAngle(double angle, double fps) const
{
    const double safeFps = fps > 0.0 ? fps : 24.0;
    const double clampedAngle = qBound(1.0, angle, 360.0);
    return qMax(1, qRound((clampedAngle / 360.0) * (1000000.0 / safeFps)));
}

int ApertarControlBridge::shutterUsForSpeed(double denominator) const
{
    const double safeDenominator = denominator > 0.0 ? denominator : 48.0;
    return qMax(1, qRound(1000000.0 / safeDenominator));
}

double ApertarControlBridge::currentFpsValue() const
{
    const double fpsValue = parseFpsValue(m_fps);
    return fpsValue > 0.0 ? fpsValue : 24.0;
}

QString ApertarControlBridge::formatFpsValue(double value) const
{
    return QString::number(value, 'f', 3);
}

QString ApertarControlBridge::formatIsoValue(double value) const
{
    return QString::number(qRound64(value));
}

QString ApertarControlBridge::formatShutterAngleValue(double value) const
{
    return compactNumber(normalizedShutterAngleForDisplay(value)) + QStringLiteral("°");
}

QString ApertarControlBridge::formatShutterSpeedValue(double value) const
{
    return QStringLiteral("1/") + compactNumber(normalizedShutterSpeedForDisplay(value));
}

QString ApertarControlBridge::formatResolutionValue(int width, int height) const
{
    if (width == 1332 && height == 990)
        return QStringLiteral("1332x990");
    if (width == 2028 && height == 1080)
        return QStringLiteral("2028x1080");
    if (width == 2028 && height == 1520)
        return QStringLiteral("2028x1520");
    if (width == 4056 && height == 2160)
        return QStringLiteral("4056x2160");
    if (width == 4056 && height == 3040)
        return QStringLiteral("4056x3040");
    if (width == 1928 && height == 1090)
        return QStringLiteral("1928x1090");
    if (width == 3856 && height == 2180)
        return QStringLiteral("3856x2180");
    return QStringLiteral("%1x%2").arg(width).arg(height);
}

double ApertarControlBridge::parseFpsValue(const QString &displayValue) const
{
    return displayValue.trimmed().toDouble();
}

double ApertarControlBridge::parseIsoValue(const QString &displayValue) const
{
    return displayValue.trimmed().toDouble();
}

double ApertarControlBridge::parseShutterAngleValue(const QString &displayValue) const
{
    QString normalized = displayValue.trimmed();
    normalized.remove(QChar(0x00B0));
    return normalized.toDouble();
}

double ApertarControlBridge::parseShutterSpeedValue(const QString &displayValue) const
{
    const QString normalized = displayValue.trimmed();
    if (normalized.startsWith(QStringLiteral("1/")))
        return normalized.mid(2).toDouble();
    return normalized.toDouble();
}

double ApertarControlBridge::parseWhiteBalanceValue(const QString &displayValue) const
{
    QString normalized = displayValue.trimmed().toUpper();
    normalized.remove(QLatin1Char('K'));
    return normalized.toDouble();
}

bool ApertarControlBridge::parseResolutionValue(const QString &displayValue, int *width, int *height) const
{
    const QString normalized = displayValue.trimmed().toLower().remove(QLatin1Char(' '));
    if (normalized == QStringLiteral("1332x990")) {
        if (width)
            *width = 1332;
        if (height)
            *height = 990;
        return true;
    }
    if (normalized == QStringLiteral("2028x1080")) {
        if (width)
            *width = 2028;
        if (height)
            *height = 1080;
        return true;
    }
    if (normalized == QStringLiteral("2028x1520")) {
        if (width)
            *width = 2028;
        if (height)
            *height = 1520;
        return true;
    }
    if (normalized == QStringLiteral("4056x2160")) {
        if (width)
            *width = 4056;
        if (height)
            *height = 2160;
        return true;
    }
    if (normalized == QStringLiteral("4056x3040")) {
        if (width)
            *width = 4056;
        if (height)
            *height = 3040;
        return true;
    }
    if (normalized == QStringLiteral("1928x1090")) {
        if (width)
            *width = 1928;
        if (height)
            *height = 1090;
        return true;
    }
    if (normalized == QStringLiteral("3856x2180")) {
        if (width)
            *width = 3856;
        if (height)
            *height = 2180;
        return true;
    }
    return false;
}

QString ApertarControlBridge::compactNumber(double value, int decimals) const
{
    QString text = QString::number(value, 'f', decimals);
    while (text.contains(QLatin1Char('.')) && (text.endsWith(QLatin1Char('0')) || text.endsWith(QLatin1Char('.')))) {
        if (text.endsWith(QLatin1Char('.'))) {
            text.chop(1);
            break;
        }
        text.chop(1);
    }
    return text;
}

void ApertarControlBridge::setFps(const QString &value)
{
    if (m_fps == value)
        return;

    m_fps = value;
    emit fpsChanged();
}

void ApertarControlBridge::setIso(const QString &value)
{
    if (m_iso == value)
        return;

    m_iso = value;
    emit isoChanged();
}

void ApertarControlBridge::setShutterAngle(const QString &value)
{
    if (m_shutterAngle == value)
        return;

    m_shutterAngle = value;
    emit shutterAngleChanged();
}

void ApertarControlBridge::setShutterSpeed(const QString &value)
{
    if (m_shutterSpeed == value)
        return;

    m_shutterSpeed = value;
    emit shutterSpeedChanged();
}

void ApertarControlBridge::setWhiteBalance(const QString &value)
{
    if (m_whiteBalance == value)
        return;

    m_whiteBalance = value;
    emit whiteBalanceChanged();
}

void ApertarControlBridge::setResolution(const QString &value)
{
    if (m_resolution == value)
        return;

    m_resolution = value;
    emit resolutionChanged();
}

void ApertarControlBridge::setRecordingState(bool value)
{
    if (m_recording == value)
        return;

    m_recording = value;
    emit recordingChanged();
}

void ApertarControlBridge::setConnected(bool value)
{
    if (m_connected == value)
        return;

    m_connected = value;
    emit connectedChanged();
}

void ApertarControlBridge::setLastError(const QString &value)
{
    if (m_lastError == value)
        return;

    m_lastError = value;
    emit lastErrorChanged();
}
