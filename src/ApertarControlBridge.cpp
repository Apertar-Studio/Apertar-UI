#include "ApertarControlBridge.hpp"

#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLocalSocket>
#include <QRegularExpression>
#include <QStringList>
#include <QUrl>
#include <QtGlobal>

namespace {

constexpr int kReconnectPollIntervalMs = 1000;
constexpr int kSteadyStatePollIntervalMs = 30000;

bool isAutoSelection(const QString &displayValue)
{
    return displayValue.trimmed().compare(QStringLiteral("Auto"), Qt::CaseInsensitive) == 0;
}

QString normalizeLocalFilePath(QString value)
{
    value = value.trimmed();
    if (value.startsWith(QStringLiteral("file://")))
        value = QUrl(value).toLocalFile();
    return value.isEmpty() ? QString() : QDir::cleanPath(value);
}

QString normalizeCdngCompressionValue(QString value)
{
    const QString trimmed = value.trimmed().toLower();
    QString compact = trimmed;
    compact.remove(QStringLiteral(" "));
    compact.remove(QStringLiteral("_"));
    compact.remove(QStringLiteral("-"));

    if (compact == QStringLiteral("losslessjpeg") || compact == QStringLiteral("lossless"))
        return QStringLiteral("lossless_jpeg");
    if (compact == QStringLiteral("lossy3:1") || compact == QStringLiteral("lossy3to1") ||
        compact == QStringLiteral("3:1") || compact == QStringLiteral("3to1"))
        return QStringLiteral("lossy_3to1");
    if (compact == QStringLiteral("lossy5:1") || compact == QStringLiteral("lossy5to1") ||
        compact == QStringLiteral("5:1") || compact == QStringLiteral("5to1"))
        return QStringLiteral("lossy_5to1");
    if (compact == QStringLiteral("none") || compact == QStringLiteral("uncompressed") || compact.isEmpty())
        return QStringLiteral("none");
    return QString();
}

bool isLossyCdngCompressionValue(const QString &value)
{
    const QString normalized = normalizeCdngCompressionValue(value);
    return normalized == QStringLiteral("lossy_3to1") || normalized == QStringLiteral("lossy_5to1");
}

QString lossyRatioForCdngCompression(const QString &value)
{
    return normalizeCdngCompressionValue(value) == QStringLiteral("lossy_5to1")
        ? QStringLiteral("5to1")
        : QStringLiteral("3to1");
}

QString cdngCompressionForLossyRatio(const QString &ratio)
{
    return ratio.trimmed().compare(QStringLiteral("5to1"), Qt::CaseInsensitive) == 0
        ? QStringLiteral("lossy_5to1")
        : QStringLiteral("lossy_3to1");
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

    connect(&m_eventSocket, &QLocalSocket::readyRead, this, &ApertarControlBridge::processEventSocketMessages);
    connect(&m_eventSocket, &QLocalSocket::connected, this, [this]() {
        setLastError(QString());
        updateRefreshTimer();
        QTimer::singleShot(0, this, &ApertarControlBridge::refreshFromCore);
    });
    connect(&m_eventSocket, &QLocalSocket::disconnected, this, [this]() {
        m_pendingStateRequestId = -1;
        setAudioInputLevel(0);
        setConnected(false);
        updateRefreshTimer();
    });
    connect(&m_eventSocket,
            qOverload<QLocalSocket::LocalSocketError>(&QLocalSocket::errorOccurred),
            this,
            [this](QLocalSocket::LocalSocketError) {
                setAudioInputLevel(0);
                setConnected(false);
                updateRefreshTimer();
            });

    connect(&m_retryTimer, &QTimer::timeout, this, &ApertarControlBridge::refreshFromCore);
    ensureEventSocketConnected();
    updateRefreshTimer();
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

QString ApertarControlBridge::recordingFormat() const
{
    return m_recordingFormat;
}

QString ApertarControlBridge::rawBitDepth() const
{
    return m_rawBitDepth;
}

QString ApertarControlBridge::h264Resolution() const
{
    return m_h264Resolution;
}

QString ApertarControlBridge::sensorName() const
{
    return m_sensorName;
}

QString ApertarControlBridge::autofocusMode() const
{
    return m_autofocusMode;
}

QString ApertarControlBridge::autofocusRange() const
{
    return m_autofocusRange;
}

QString ApertarControlBridge::autofocusSpeed() const
{
    return m_autofocusSpeed;
}

QString ApertarControlBridge::autofocusWindow() const
{
    return m_autofocusWindow;
}

bool ApertarControlBridge::recording() const
{
    return m_recording;
}

bool ApertarControlBridge::timelapseActive() const
{
    return m_timelapseActive;
}

double ApertarControlBridge::stillExposureRemainingSeconds() const
{
    return m_stillExposureActive ? std::max(0.0, m_stillExposureRemainingSeconds -
        (m_stillExposureTimer.isValid() ? m_stillExposureTimer.elapsed() / 1000.0 : 0.0)) : 0.0;
}

int ApertarControlBridge::audioInputLevel() const
{
    return m_audioInputLevel;
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
    if (!m_whiteBalanceAvailable) {
        setLastError(QStringLiteral("White balance is unavailable for the selected tuning file."));
        return false;
    }
    if (isAutoSelection(displayValue)) {
        if (!sendCoreCommand(QStringLiteral("set_wb_auto")))
            return false;

        m_whiteBalanceAuto = true;
        setWhiteBalance(QStringLiteral("Auto"));
        saveCurrentCameraSettings();
        return true;
    }

    const double kelvin = parseWhiteBalanceValue(displayValue);
    if (kelvin <= 0.0) {
        setLastError(QStringLiteral("Invalid white balance value."));
        return false;
    }

    if (!sendCoreCommand(QStringLiteral("set_wb"), { { QStringLiteral("kelvin"), qRound(kelvin) } }))
        return false;

    m_whiteBalanceAuto = false;
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

bool ApertarControlBridge::applyRecordingFormat(const QString &displayValue)
{
    const QString normalized = parseRecordingFormatValue(displayValue);
    if (!m_rawRecordingSupported && (normalized == QStringLiteral("cdng") ||
        normalized == QStringLiteral("lossydng") || normalized == QStringLiteral("mlv"))) {
        setLastError(QStringLiteral("RAW clip recording is not available on this board. Use H.264."));
        return false;
    }
    if ((normalized == QStringLiteral("prores") && !m_proresSupported) ||
        (normalized == QStringLiteral("lossydng") && !m_rawCompressionSupported)) {
        setLastError(QStringLiteral("This recording mode requires Raspberry Pi 5-class hardware."));
        return false;
    }
    if (normalized.isEmpty()) {
        setLastError(QStringLiteral("Invalid recording format value."));
        return false;
    }
    QString coreFormat = normalized;
    const QString profile = displayValue.trimmed().compare(QStringLiteral("ProRes"), Qt::CaseInsensitive) == 0 ? m_proresProfile
        : displayValue.contains(QStringLiteral("Proxy"), Qt::CaseInsensitive) ? QStringLiteral("proxy")
        : displayValue.endsWith(QStringLiteral("LT"), Qt::CaseInsensitive) ? QStringLiteral("lt")
        : displayValue.endsWith(QStringLiteral("HQ"), Qt::CaseInsensitive) ? QStringLiteral("hq") : QStringLiteral("422");
    QString lossyRatio = parseLossyDngRatioValue(displayValue);
    if (normalized == QStringLiteral("lossydng")) {
        m_cdngCompression = cdngCompressionForLossyRatio(lossyRatio);
        emit cdngCompressionChanged();
    }

    if (!sendCoreCommand(QStringLiteral("set_record_format"), { { QStringLiteral("value"), coreFormat } }))
        return false;
    if (coreFormat == QStringLiteral("lossydng") &&
        !sendCoreCommand(QStringLiteral("set_lossy_dng_ratio"), { { QStringLiteral("value"), lossyRatio } })) {
        return false;
    }

    if (coreFormat == QStringLiteral("lossydng"))
        m_lossyDngRatio = lossyRatio;
    if (coreFormat == QStringLiteral("prores") &&
        displayValue.trimmed().compare(QStringLiteral("ProRes"), Qt::CaseInsensitive) != 0) {
        if (!sendCoreCommand(QStringLiteral("set_prores_profile"), {{QStringLiteral("value"), profile}}))
            return false;
        setProresProfileValue(profile);
    }
    setRecordingFormatValue(formatRecordingFormatValue(coreFormat));
    saveCurrentCameraSettings();
    return true;
}

bool ApertarControlBridge::applyProresColorProfile(const QString &value)
{
    if (m_recording || m_timelapseActive || !m_proresSupported ||
        !QStringList{QStringLiteral("standard"), QStringLiteral("arri_logc4"), QStringLiteral("sony_slog3"),
                     QStringLiteral("davinci_intermediate"), QStringLiteral("p3_d60_linear")}.contains(value)) return false;
    if (!sendCoreCommand(QStringLiteral("set_prores_color_profile"), {{QStringLiteral("value"), value}})) return false;
    return true;
}

bool ApertarControlBridge::applyCdngPreviewColorProfile(const QString &value)
{
    if (m_recording || m_timelapseActive ||
        !QStringList{QStringLiteral("standard"), QStringLiteral("arri_logc4"), QStringLiteral("sony_slog3"),
                     QStringLiteral("davinci_intermediate"), QStringLiteral("p3_d60_linear")}.contains(value)) return false;
    return sendCoreCommand(QStringLiteral("set_cdng_preview_color_profile"), {{QStringLiteral("value"), value}});
}

bool ApertarControlBridge::applyProresProfile(const QString &value)
{
    if (!m_proresSupported) {
        setLastError(QStringLiteral("ProRes recording requires Raspberry Pi 5-class hardware."));
        return false;
    }
    const QString profile = value.trimmed().toLower();
    if (profile != QStringLiteral("hq") && profile != QStringLiteral("422") &&
        profile != QStringLiteral("lt") && profile != QStringLiteral("proxy")) {
        setLastError(QStringLiteral("Invalid ProRes profile."));
        return false;
    }
    if (m_recording || !m_recordingFormat.startsWith(QStringLiteral("ProRes")))
        return false;
    if (!sendCoreCommand(QStringLiteral("set_prores_profile"), {{QStringLiteral("value"), profile}}))
        return false;

    setProresProfileValue(profile);
    saveCurrentCameraSettings();
    return true;
}

bool ApertarControlBridge::applyCdngCompression(const QString &value)
{
    const QString normalized = normalizeCdngCompressionValue(value);
    if (!normalized.isEmpty() && normalized != QStringLiteral("none") && !m_rawCompressionSupported) {
        setLastError(QStringLiteral("RAW compression requires Raspberry Pi 5-class hardware."));
        return false;
    }
    if (normalized.isEmpty())
        return false;

    if (isLossyCdngCompressionValue(normalized)) {
        const QString ratio = lossyRatioForCdngCompression(normalized);
        if (!sendCoreCommand(QStringLiteral("set_record_format"), {{QStringLiteral("value"), QStringLiteral("lossydng")}}))
            return false;
        if (!sendCoreCommand(QStringLiteral("set_lossy_dng_ratio"), {{QStringLiteral("value"), ratio}}))
            return false;
        m_lossyDngRatio = ratio;
        setRecordingFormatValue(QStringLiteral("cDNG"));
    } else {
        if (!sendCoreCommand(QStringLiteral("set_record_format"), {{QStringLiteral("value"), QStringLiteral("cdng")}}))
            return false;
        if (!sendCoreCommand(QStringLiteral("set_cdng_compression"), {{QStringLiteral("value"), normalized}}))
            return false;
        setRecordingFormatValue(QStringLiteral("cDNG"));
    }

    if (m_cdngCompression != normalized) {
        m_cdngCompression = normalized;
        emit cdngCompressionChanged();
    }
    saveCurrentCameraSettings();
    return true;
}

bool ApertarControlBridge::applyTuningFile(const QString &path)
{
    const QString normalized = normalizeLocalFilePath(path);
    if (!normalized.isEmpty()) {
        const QFileInfo info(normalized);
        if (!info.exists() || !info.isFile() || !info.isReadable()) {
            setLastError(QStringLiteral("Tuning file is not readable."));
            return false;
        }
        if (info.suffix().compare(QStringLiteral("json"), Qt::CaseInsensitive) != 0) {
            setLastError(QStringLiteral("Tuning file must be a .json file."));
            return false;
        }
    }

    if (!sendCoreCommand(QStringLiteral("set_tuning_file"), {
            { QStringLiteral("path"), normalized }
        })) {
        return false;
    }

    setTuningFileValue(normalized);
    saveCurrentCameraSettings();
    return true;
}

bool ApertarControlBridge::applyRawBitDepth(const QString &displayValue)
{
    if (m_recordingFormat == QStringLiteral("H.264") || m_recordingFormat.startsWith(QStringLiteral("ProRes"))) {
        setRawBitDepthValue(QStringLiteral("10-bit"));
        saveCurrentCameraSettings();
        return true;
    }

    const int bitDepth = parseRawBitDepthValue(displayValue);
    if (bitDepth != 8 && bitDepth != 10 && bitDepth != 12) {
        setLastError(QStringLiteral("Invalid raw bit depth value."));
        return false;
    }

    if (!sendCoreCommand(QStringLiteral("set_raw_bit_depth"), {
            { QStringLiteral("value"), bitDepth }
        })) {
        return false;
    }

    setRawBitDepthValue(formatRawBitDepthValue(bitDepth));
    saveCurrentCameraSettings();
    return true;
}

bool ApertarControlBridge::applyH264Resolution(const QString &displayValue)
{
    int width = 0;
    int height = 0;
    if (!parseH264ResolutionValue(displayValue, &width, &height)) {
        setLastError(QStringLiteral("Invalid H.264 resolution value."));
        return false;
    }

    if (!sendCoreCommand(QStringLiteral("set_h264_resolution"), {
            { QStringLiteral("width"), width },
            { QStringLiteral("height"), height }
        })) {
        return false;
    }

    setH264ResolutionValue(formatH264ResolutionValue(width, height));
    saveCurrentCameraSettings();
    return true;
}

bool ApertarControlBridge::applyTimecodeMode(const QString &displayValue)
{
    const QString normalized = parseTimecodeModeValue(displayValue);
    if (normalized.isEmpty()) {
        setLastError(QStringLiteral("Invalid timecode mode value."));
        return false;
    }

    if (!sendCoreCommand(QStringLiteral("set_timecode_mode"), { { QStringLiteral("value"), normalized } }))
        return false;

    m_settings.setValue(QStringLiteral("timecodeMode"), displayValue);
    m_settings.sync();
    return true;
}

bool ApertarControlBridge::applyAutofocusMode(const QString &displayValue)
{
    const QString normalized = parseAutofocusModeValue(displayValue);
    if (normalized.isEmpty()) {
        setLastError(QStringLiteral("Invalid autofocus mode value."));
        return false;
    }

    if (!sendCoreCommand(QStringLiteral("set_af_mode"), { { QStringLiteral("value"), normalized } }))
        return false;

    setAutofocusModeValue(formatAutofocusModeValue(normalized));
    saveCurrentCameraSettings();
    return true;
}

bool ApertarControlBridge::applyAutofocusRange(const QString &displayValue)
{
    const QString normalized = parseAutofocusRangeValue(displayValue);
    if (normalized.isEmpty()) {
        setLastError(QStringLiteral("Invalid autofocus range value."));
        return false;
    }

    if (!sendCoreCommand(QStringLiteral("set_af_range"), { { QStringLiteral("value"), normalized } }))
        return false;

    setAutofocusRangeValue(formatAutofocusRangeValue(normalized));
    saveCurrentCameraSettings();
    return true;
}

bool ApertarControlBridge::applyAutofocusSpeed(const QString &displayValue)
{
    const QString normalized = parseAutofocusSpeedValue(displayValue);
    if (normalized.isEmpty()) {
        setLastError(QStringLiteral("Invalid autofocus speed value."));
        return false;
    }

    if (!sendCoreCommand(QStringLiteral("set_af_speed"), { { QStringLiteral("value"), normalized } }))
        return false;

    setAutofocusSpeedValue(formatAutofocusSpeedValue(normalized));
    saveCurrentCameraSettings();
    return true;
}

bool ApertarControlBridge::applyAutofocusWindow(const QString &displayValue)
{
    double x = 0.25;
    double y = 0.25;
    double width = 0.5;
    double height = 0.5;
    if (!autofocusWindowPreset(displayValue, &x, &y, &width, &height)) {
        setLastError(QStringLiteral("Invalid autofocus window value."));
        return false;
    }

    if (!sendCoreCommand(QStringLiteral("set_af_window"), {
            { QStringLiteral("x"), x },
            { QStringLiteral("y"), y },
            { QStringLiteral("width"), width },
            { QStringLiteral("height"), height }
        })) {
        return false;
    }

    m_afX = x; m_afY = y; m_afWidth = width; m_afHeight = height;
    setAutofocusWindowValue(formatAutofocusWindowValue(x, y, width, height));
    saveCurrentCameraSettings();
    return true;
}

bool ApertarControlBridge::applyPhotoTimelapse(const QString &displayValue, const QString &format)
{
    QJsonObject arguments;
    arguments.insert(QStringLiteral("value"), displayValue.trimmed().isEmpty()
                                             ? QStringLiteral("Off")
                                             : displayValue.trimmed());
    arguments.insert(QStringLiteral("format"), format.trimmed().isEmpty()
                                              ? QStringLiteral("DNG")
                                              : format.trimmed());
    return sendCoreCommand(QStringLiteral("set_photo_timelapse"), arguments);
}

bool ApertarControlBridge::applyPhotoModeEnabled(bool enabled)
{
    QJsonObject arguments;
    arguments.insert(QStringLiteral("enabled"), enabled ? QStringLiteral("true") : QStringLiteral("false"));
    return sendCoreCommand(QStringLiteral("set_photo_mode_enabled"), arguments);
}

bool ApertarControlBridge::applyCaptureMode(const QString &mode)
{
    return sendCoreCommand(QStringLiteral("set_capture_mode"), {{QStringLiteral("value"), mode}});
}

bool ApertarControlBridge::applyPhotoLongExposureEnabled(bool enabled)
{
    return sendCoreCommand(QStringLiteral("set_photo_long_exposure_enabled"), {
        {QStringLiteral("enabled"), enabled ? QStringLiteral("true") : QStringLiteral("false")}
    });
}

bool ApertarControlBridge::applyPhotoBracketingEnabled(bool enabled)
{
    return sendCoreCommand(QStringLiteral("set_photo_bracketing_enabled"), {
        {QStringLiteral("enabled"), enabled ? QStringLiteral("true") : QStringLiteral("false")}
    });
}

bool ApertarControlBridge::setTimelapseActive(bool active, const QString &displayValue, const QString &format)
{
    QJsonObject arguments;
    if (active) {
        arguments.insert(QStringLiteral("value"), displayValue.trimmed().isEmpty()
                                                 ? QStringLiteral("5s")
                                                 : displayValue.trimmed());
        arguments.insert(QStringLiteral("format"), format.trimmed().isEmpty()
                                                  ? QStringLiteral("DNG")
                                                  : format.trimmed());
    }

    QJsonObject reply;
    if (!sendCoreCommand(active ? QStringLiteral("timelapse_start")
                                : QStringLiteral("timelapse_stop"),
                         arguments,
                         &reply)) {
        return false;
    }

    if (reply.contains(QStringLiteral("timelapse_active")))
        setTimelapseActiveState(reply.value(QStringLiteral("timelapse_active")).toBool(active));
    else
        setTimelapseActiveState(active);
    return true;
}

bool ApertarControlBridge::setRecording(bool recording)
{
    QJsonObject arguments;
    if (recording) {
        m_settings.sync();
        const bool soundRecorderSetting = m_settings.value(QStringLiteral("soundRecorderEnabled"), false).toBool();
        const bool audioEnabledSetting = m_settings.value(QStringLiteral("recordAudioEnabled"), false).toBool();
        const bool gyroEnabledSetting = m_settings.value(QStringLiteral("recordGyroEnabled"), false).toBool();
        const bool audioMonitoringEnabledSetting = m_settings.value(QStringLiteral("liveAudioMonitoringEnabled"), true).toBool();
        const int inputVolume = qBound(0, m_settings.value(QStringLiteral("inputVolume"), 60).toInt(), 100);
        const int outputVolume = qBound(0, m_settings.value(QStringLiteral("headphoneVolume"), 55).toInt(), 100);
        const QString inputDevice = audioDeviceIdFromSelection(
            m_settings.value(QStringLiteral("audioInputDevice"), QString()).toString());
        const QString outputDevice = audioDeviceIdFromSelection(
            m_settings.value(QStringLiteral("audioOutputDevice"), QString()).toString());
        const bool audioEnabled = soundRecorderSetting || (audioEnabledSetting && !inputDevice.isEmpty());
        const bool audioMonitoringEnabled = audioEnabled
                                            && audioMonitoringEnabledSetting
                                            && !outputDevice.isEmpty()
                                            && outputVolume > 0;

        arguments.insert(QStringLiteral("sound_only"), soundRecorderSetting ? QStringLiteral("true") : QStringLiteral("false"));
        arguments.insert(QStringLiteral("audio_enabled"), audioEnabled ? QStringLiteral("true") : QStringLiteral("false"));
        arguments.insert(QStringLiteral("gyro_enabled"), (!soundRecorderSetting && gyroEnabledSetting) ? QStringLiteral("true") : QStringLiteral("false"));
        arguments.insert(QStringLiteral("audio_monitoring_enabled"), audioMonitoringEnabled ? QStringLiteral("true") : QStringLiteral("false"));
        arguments.insert(QStringLiteral("audio_input_volume"), inputVolume);
        arguments.insert(QStringLiteral("audio_output_volume"), outputVolume);
        if (audioEnabled)
            arguments.insert(QStringLiteral("audio_input_device"), inputDevice);
        if (audioMonitoringEnabled)
            arguments.insert(QStringLiteral("audio_output_device"), outputDevice);
    }

    if (!sendCoreCommand(recording ? QStringLiteral("record_start") : QStringLiteral("record_stop"), arguments))
        return false;

    setRecordingState(recording);
    return true;
}

bool ApertarControlBridge::applyPhotoBracketingEv(double ev)
{
    return sendCoreCommand(QStringLiteral("set_photo_bracketing_ev"), {{QStringLiteral("value"), ev}});
}

bool ApertarControlBridge::capturePhoto(const QString &format, int count)
{
    QJsonObject arguments;
    arguments.insert(QStringLiteral("count"), count);
    arguments.insert(QStringLiteral("format"), format.trimmed().isEmpty()
                                             ? QStringLiteral("DNG")
                                             : format.trimmed());
    return sendCoreCommand(QStringLiteral("capture_photo"), arguments);
}

bool ApertarControlBridge::calibrateGyro()
{
    QJsonObject reply;
    if (!sendCoreCommand(QStringLiteral("calibrate_gyro"),
                         { { QStringLiteral("seconds"), 2.5 } },
                         &reply)) {
        emit gyroCalibrationFinished(false, m_lastError);
        return false;
    }

    const int samples = reply.value(QStringLiteral("samples")).toInt(0);
    const double biasX = reply.value(QStringLiteral("gyro_bias_x")).toDouble(0.0);
    const double biasY = reply.value(QStringLiteral("gyro_bias_y")).toDouble(0.0);
    const double biasZ = reply.value(QStringLiteral("gyro_bias_z")).toDouble(0.0);
    const QString message = QStringLiteral("Saved gyro bias X %1, Y %2, Z %3 from %4 samples.")
        .arg(biasX, 0, 'f', 1)
        .arg(biasY, 0, 'f', 1)
        .arg(biasZ, 0, 'f', 1)
        .arg(samples);
    emit gyroCalibrationFinished(true, message);
    return true;
}

void ApertarControlBridge::refreshFromCore()
{
    ensureEventSocketConnected();
    if (m_eventSocket.state() != QLocalSocket::ConnectedState || m_pendingStateRequestId >= 0)
        return;
    // Hardware mode switches can take seconds. Poll asynchronously so a status
    // refresh cannot block the GUI thread behind that camera operation.
    const int id = m_nextCommandId++;
    m_pendingStateRequestId = id;
    const QJsonObject request{{QStringLiteral("cmd"), QStringLiteral("get_state")},
                              {QStringLiteral("id"), id}};
    m_eventSocket.write(QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n');
    QTimer::singleShot(10000, this, [this, id]() {
        if (m_pendingStateRequestId != id) return;
        m_pendingStateRequestId = -1;
        setLastError(QStringLiteral("Timed out waiting for ApertarCore status."));
        m_eventSocket.abort();
    });
}

void ApertarControlBridge::updateRefreshTimer()
{
    const bool needsFastPolling = m_eventSocket.state() != QLocalSocket::ConnectedState
        || !m_connected;
    m_retryTimer.setInterval(needsFastPolling
                             ? kReconnectPollIntervalMs
                             : kSteadyStatePollIntervalMs);
}

void ApertarControlBridge::loadPersistedCameraSettings()
{
    // Cached values are placeholders only. Core owns restart/format restoration.
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

    m_whiteBalanceAuto = m_settings.value(QStringLiteral("camera/whiteBalanceAuto"), false).toBool();
    if (m_settings.value(QStringLiteral("camera/whiteBalanceLocked"), false).toBool()) {
        m_whiteBalanceAuto = true;
        m_settings.setValue(QStringLiteral("camera/whiteBalanceAuto"), true);
    }
    m_settings.remove(QStringLiteral("camera/whiteBalanceLocked"));
    const int kelvin = m_settings.value(QStringLiteral("camera/whiteBalanceKelvin"), qRound(parseWhiteBalanceValue(m_whiteBalance))).toInt();
    if (m_whiteBalanceAuto) {
        m_whiteBalance = QStringLiteral("Auto");
    } else if (kelvin > 0) {
        m_whiteBalance = QString::number(kelvin) + QStringLiteral("K");
    }

    const QString resolution = m_settings.value(QStringLiteral("camera/resolution"), m_resolution).toString();
    int width = 0;
    int height = 0;
    if (parseResolutionValue(resolution, &width, &height))
        m_resolution = formatResolutionValue(width, height);

    const QString recordingFormat = parseRecordingFormatValue(
        m_settings.value(QStringLiteral("camera/recordingFormat"), QStringLiteral("cdng")).toString());
    m_proresProfile = m_settings.value(QStringLiteral("camera/proresProfile"), QStringLiteral("422")).toString();
    m_proresColorProfile = m_settings.value(QStringLiteral("camera/proresColorProfile"), QStringLiteral("standard")).toString();
    m_cdngPreviewColorProfile = m_settings.value(QStringLiteral("camera/cdngPreviewColorProfile"), QStringLiteral("standard")).toString();
    m_lossyDngRatio = parseLossyDngRatioValue(
        m_settings.value(QStringLiteral("camera/lossyDngRatio"), QStringLiteral("3to1")).toString());
    if (!recordingFormat.isEmpty())
        m_recordingFormat = formatRecordingFormatValue(recordingFormat);
    m_cdngCompression = normalizeCdngCompressionValue(
        m_settings.value(QStringLiteral("camera/cdngCompression"), QStringLiteral("none")).toString());
    if (m_cdngCompression.isEmpty())
        m_cdngCompression = QStringLiteral("none");
    if (recordingFormat == QStringLiteral("lossydng") && !isLossyCdngCompressionValue(m_cdngCompression))
        m_cdngCompression = cdngCompressionForLossyRatio(m_lossyDngRatio);
    m_tuningFile = normalizeLocalFilePath(m_settings.value(QStringLiteral("camera/tuningFile"), QString()).toString());
    m_rawBitDepth = formatRawBitDepthValue(m_settings.value(QStringLiteral("camera/rawBitDepth"), 12).toInt());
    if (m_recordingFormat == QStringLiteral("H.264") || m_recordingFormat.startsWith(QStringLiteral("ProRes")))
        m_rawBitDepth = QStringLiteral("10-bit");
    int h264Width = 0;
    int h264Height = 0;
    if (parseH264ResolutionValue(
            m_settings.value(QStringLiteral("camera/h264Resolution"), QStringLiteral("Match Capture")).toString(),
            &h264Width,
            &h264Height)) {
        m_h264Resolution = formatH264ResolutionValue(h264Width, h264Height);
    }

    m_autofocusMode = formatAutofocusModeValue(
        m_settings.value(QStringLiteral("camera/autofocusMode"), QStringLiteral("continuous")).toString());
    m_autofocusRange = formatAutofocusRangeValue(
        m_settings.value(QStringLiteral("camera/autofocusRange"), QStringLiteral("normal")).toString());
    m_autofocusSpeed = formatAutofocusSpeedValue(
        m_settings.value(QStringLiteral("camera/autofocusSpeed"), QStringLiteral("normal")).toString());
    const double afX = m_settings.value(QStringLiteral("camera/autofocusWindowX"), 0.25).toDouble();
    const double afY = m_settings.value(QStringLiteral("camera/autofocusWindowY"), 0.25).toDouble();
    const double afW = m_settings.value(QStringLiteral("camera/autofocusWindowWidth"), 0.5).toDouble();
    const double afH = m_settings.value(QStringLiteral("camera/autofocusWindowHeight"), 0.5).toDouble();
    m_autofocusWindow = formatAutofocusWindowValue(afX, afY, afW, afH);
    m_afX = afX; m_afY = afY; m_afWidth = afW; m_afHeight = afH;

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
    m_settings.setValue(QStringLiteral("camera/whiteBalanceAuto"), m_whiteBalanceAuto);
    if (!m_isoAuto && isoValue > 0.0)
        m_settings.setValue(QStringLiteral("camera/iso"), qRound64(isoValue));
    if (!m_shutterAuto && shutterUs > 0)
        m_settings.setValue(QStringLiteral("camera/shutterUs"), shutterUs);
    if (!m_whiteBalanceAuto && kelvin > 0.0)
        m_settings.setValue(QStringLiteral("camera/whiteBalanceKelvin"), qRound(kelvin));
    m_settings.setValue(QStringLiteral("camera/resolution"), m_resolution);
    m_settings.setValue(QStringLiteral("camera/recordingFormat"), parseRecordingFormatValue(m_recordingFormat));
    m_settings.setValue(QStringLiteral("camera/cdngCompression"), m_cdngCompression);
    m_settings.setValue(QStringLiteral("camera/lossyDngRatio"), m_lossyDngRatio);
    m_settings.setValue(QStringLiteral("camera/proresProfile"), m_proresProfile);
    m_settings.setValue(QStringLiteral("camera/proresColorProfile"), m_proresColorProfile);
    m_settings.setValue(QStringLiteral("camera/cdngPreviewColorProfile"), m_cdngPreviewColorProfile);
    m_settings.setValue(QStringLiteral("camera/tuningFile"), m_tuningFile);
    m_settings.setValue(QStringLiteral("camera/rawBitDepth"), parseRawBitDepthValue(m_rawBitDepth));
    m_settings.setValue(QStringLiteral("camera/h264Resolution"), m_h264Resolution);
    m_settings.setValue(QStringLiteral("camera/autofocusMode"), parseAutofocusModeValue(m_autofocusMode));
    m_settings.setValue(QStringLiteral("camera/autofocusRange"), parseAutofocusRangeValue(m_autofocusRange));
    m_settings.setValue(QStringLiteral("camera/autofocusSpeed"), parseAutofocusSpeedValue(m_autofocusSpeed));
    m_settings.setValue(QStringLiteral("camera/autofocusWindowX"), m_afX);
    m_settings.setValue(QStringLiteral("camera/autofocusWindowY"), m_afY);
    m_settings.setValue(QStringLiteral("camera/autofocusWindowWidth"), m_afWidth);
    m_settings.setValue(QStringLiteral("camera/autofocusWindowHeight"), m_afHeight);
    m_settings.sync();
}

bool ApertarControlBridge::sendCoreCommand(const QString &commandName, const QJsonObject &arguments, QJsonObject *replyObject)
{
    constexpr int kConnectTimeoutMs = 350;
    const bool reconfigureCommand = commandName == QStringLiteral("set_resolution") ||
                                    commandName == QStringLiteral("set_capture_mode") ||
                                    commandName == QStringLiteral("set_raw_bit_depth") ||
                                    commandName == QStringLiteral("set_record_format") ||
                                    commandName == QStringLiteral("set_tuning_file") ||
                                    commandName == QStringLiteral("set_prores_color_profile") ||
                                    commandName == QStringLiteral("set_cdng_preview_color_profile") ||
                                    commandName == QStringLiteral("set_h264_resolution") ||
                                    commandName == QStringLiteral("capture_photo") ||
                                    commandName == QStringLiteral("timelapse_start") ||
                                    commandName == QStringLiteral("wake") ||
                                    commandName == QStringLiteral("enter_standby") ||
                                    commandName == QStringLiteral("apply_preset");
    const int replyTimeoutMs = commandName == QStringLiteral("record_stop") ? 60000
        : commandName == QStringLiteral("calibrate_gyro") ? 8000
        : (reconfigureCommand ? 10000 : 3000);
    const QString socketPath = qEnvironmentVariable("APERTAR_CONTROL_SOCKET", QStringLiteral("/tmp/apertar-core.sock"));

    QLocalSocket socket;
    socket.connectToServer(socketPath);
    if (!socket.waitForConnected(kConnectTimeoutMs)) {
        ensureEventSocketConnected();
        setAudioInputLevel(0);
        setConnected(false);
        setLastError(QStringLiteral("Waiting for ApertarCore control socket..."));
        updateRefreshTimer();
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
        updateRefreshTimer();
        return false;
    }

    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < replyTimeoutMs) {
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
                updateRefreshTimer();
                return false;
            }

            setLastError(QString());
            updateRefreshTimer();
            return true;
        }

        const int remaining = qMax(1, replyTimeoutMs - int(timer.elapsed()));
        if (!socket.waitForReadyRead(qMin(remaining, 250))) {
            if (socket.state() == QLocalSocket::UnconnectedState)
                break;
        }
    }

    setConnected(false);
    setAudioInputLevel(0);
    setLastError(QStringLiteral("Timed out waiting for ApertarCore reply."));
    updateRefreshTimer();
    return false;
}

bool ApertarControlBridge::applyExposureCompensation(double ev)
{
    return sendCoreCommand(QStringLiteral("set_exposure_compensation"), {{QStringLiteral("value"), ev}});
}

bool ApertarControlBridge::applyExposureMetering(const QString &mode)
{
    return sendCoreCommand(QStringLiteral("set_exposure_metering"), {{QStringLiteral("value"), mode}});
}

QJsonObject ApertarControlBridge::cameraPresetSettings() const
{
    QJsonObject result;
    result.insert("cameraFps", m_fps);
    result.insert("cameraIso", m_iso);
    result.insert("cameraExposureCompensation", m_exposureCompensation);
    result.insert("cameraExposureMetering", m_exposureMetering);
    result.insert("cameraShutterAngle", m_shutterAngle);
    result.insert("cameraShutterSpeed", m_shutterSpeed);
    result.insert("cameraWhiteBalance", m_whiteBalance);
    result.insert("cameraResolution", m_resolution);
    int width = 0, height = 0;
    if (parseResolutionValue(m_resolution, &width, &height)) {
        result.insert("cameraResolutionWidth", width);
        result.insert("cameraResolutionHeight", height);
    }
    const bool lossy = isLossyCdngCompressionValue(m_cdngCompression) && m_recordingFormat == "cDNG";
    result.insert("cameraRecordingFormat", lossy ? QStringLiteral("lossydng") : parseRecordingFormatValue(m_recordingFormat));
    result.insert("cameraCdngCompression", lossy ? QStringLiteral("none") : m_cdngCompression);
    result.insert("cameraLossyDngRatio", m_lossyDngRatio);
    result.insert("cameraProresProfile", m_proresProfile);
    result.insert("cameraProresColorProfile", m_proresColorProfile);
    result.insert("cameraCdngPreviewColorProfile", m_cdngPreviewColorProfile);
    result.insert("cameraTuningFile", m_tuningFile);
    result.insert("cameraRawBitDepth", m_rawBitDepth);
    result.insert("cameraH264Resolution", m_h264Resolution);
    result.insert("cameraAutofocusMode", parseAutofocusModeValue(m_autofocusMode));
    result.insert("cameraAutofocusRange", parseAutofocusRangeValue(m_autofocusRange));
    result.insert("cameraAutofocusSpeed", parseAutofocusSpeedValue(m_autofocusSpeed));
    result.insert("cameraAutofocusWindowX", m_afX); result.insert("cameraAutofocusWindowY", m_afY);
    result.insert("cameraAutofocusWindowWidth", m_afWidth); result.insert("cameraAutofocusWindowHeight", m_afHeight);
    return result;
}

bool ApertarControlBridge::applyCameraPreset(const QJsonObject &settings)
{
    QJsonObject capture;
    for (auto it = settings.begin(); it != settings.end(); ++it)
        if (it.key().startsWith("camera") || it.key() == "timecodeMode" || it.key() == "photoBracketingEv")
            capture.insert(it.key(), it.value());
    if (capture.isEmpty()) return true;
    QJsonObject reply;
    if (!sendCoreCommand("apply_preset", capture, &reply)) return false;
    applyCoreState(reply.value("state").toObject());
    saveCurrentCameraSettings();
    return true;
}

bool ApertarControlBridge::enterStandby()
{
    return sendCoreCommand(QStringLiteral("enter_standby"));
}

bool ApertarControlBridge::wakeFromStandby()
{
    return sendCoreCommand(QStringLiteral("wake"));
}

void ApertarControlBridge::notifyUserActivity()
{
    if (m_eventSocket.state() == QLocalSocket::ConnectedState)
        m_eventSocket.write("{\"cmd\":\"user_activity\",\"id\":-2}\n");
}

void ApertarControlBridge::applyCoreMessage(const QJsonObject &message)
{
    const QString event = message.value(QStringLiteral("event")).toString();
    if (event == QStringLiteral("photo_write_error")) {
        m_lastPhotoWriteError = message.value(QStringLiteral("photo_write_error")).toString();
        setLastError(m_lastPhotoWriteError);
        emit photoCaptureFailed(m_lastPhotoWriteError);
        return;
    }
    if (event == QStringLiteral("standby") || event == QStringLiteral("still_exposure") ||
        event == QStringLiteral("timelapse_progress") || event == QStringLiteral("media_write_state")) {
        applyCoreState(message);
        return;
    }
    if (event == QStringLiteral("audio_level")) {
        setAudioInputLevel(message.value(QStringLiteral("input_level")).toInt(0));
        return;
    }

    if (event == QStringLiteral("recording")) {
        applyCoreState(message);
        setRecordingState(message.value(QStringLiteral("active")).toBool(false));
        const auto error = message.value(QStringLiteral("error")).toString();
        m_lastRecordingError = error;
        if (!error.isEmpty()) {
            setLastError(error);
            emit recordingFailed(error);
        }
        return;
    }

    if (event == QStringLiteral("recording_health")) {
        applyCoreState(message);
        return;
    }

    if (event == QStringLiteral("timelapse")) {
        setTimelapseActiveState(message.value(QStringLiteral("active")).toBool(false));
        if (message.value(QStringLiteral("active")).toBool(false)) {
            const double seconds = message.value(QStringLiteral("interval_seconds")).toDouble(0);
            const QString interval = seconds > 0
                ? QString::number(seconds) + QStringLiteral("s")
                : m_settings.value(QStringLiteral("photoTimelapse"), QStringLiteral("5s")).toString();
            emit sharedPhotoStateChanged(true,
                                         m_settings.value(QStringLiteral("photoFormat"), QStringLiteral("DNG")).toString(),
                                         interval,
                                         m_settings.value(QStringLiteral("photoLongExposureEnabled"), false).toBool(),
                                         m_settings.value(QStringLiteral("photoBracketingEnabled"), false).toBool(),
                                         m_settings.value(QStringLiteral("photoBracketingEv"), 1.0).toDouble());
        }
        return;
    }

    if (event == QStringLiteral("timelapse_frame")) {
        emit timelapseFrameCaptured();
        return;
    }

    if (event == QStringLiteral("photo_capture")) {
        emit photoCaptured();
        return;
    }

    if (event == QStringLiteral("prepare_recording")) {
        emit externalRecordingPreparationRequested();
        return;
    }

    if (event == QStringLiteral("hardware_action")) {
        const QString action = message.value(QStringLiteral("action")).toString().trimmed();
        if (!action.isEmpty())
            emit hardwareActionTriggered(action);
        return;
    }

    if (event == QStringLiteral("settings") || message.contains(QStringLiteral("ok")))
        applyCoreState(message);
}

void ApertarControlBridge::applyCoreState(const QJsonObject &state)
{
    if (state.contains(QStringLiteral("preview_exposure_reference"))) {
        const auto reference=state.value(QStringLiteral("preview_exposure_reference")).toObject().toVariantMap();
        if (reference!=m_previewExposureReference) {
            m_previewExposureReference=reference;
            emit previewExposureReferenceChanged();
        }
    }
    if (state.contains(QStringLiteral("recording_has_dropped_frames"))) {
        const bool dropped = state.value(QStringLiteral("recording_has_dropped_frames")).toBool();
        if (m_recordingHasDroppedFrames != dropped) {
            m_recordingHasDroppedFrames = dropped;
            emit recordingHealthChanged();
        }
    }
    if (state.contains(QStringLiteral("recording_error"))) {
        const auto recordingError = state.value(QStringLiteral("recording_error")).toString();
        const bool changed = recordingError != m_lastRecordingError;
        m_lastRecordingError = recordingError;
        if (!recordingError.isEmpty() && changed) {
            setLastError(recordingError);
            emit recordingFailed(recordingError);
        }
    }
    if (state.value(QStringLiteral("recording_capabilities")).isObject()) {
        const auto caps = state.value(QStringLiteral("recording_capabilities")).toObject();
        const bool raw = caps.value(QStringLiteral("raw_compression")).toBool();
        const bool rawRecording = caps.value(QStringLiteral("raw_recording")).toBool(true);
        const bool prores = caps.value(QStringLiteral("prores")).toBool();
        const bool hardwareH264 = caps.value(QStringLiteral("h264_hardware")).toBool();
        const int h264Width = caps.value(QStringLiteral("h264_max_width")).toInt(2560);
        const double h264Fps = caps.value(QStringLiteral("h264_max_fps")).toDouble();
        if (raw != m_rawCompressionSupported || rawRecording != m_rawRecordingSupported || prores != m_proresSupported ||
            hardwareH264 != m_h264HardwareEncoding || h264Width != m_h264MaxWidth || h264Fps != m_h264MaxFps) {
            m_rawCompressionSupported = raw;
            m_rawRecordingSupported = rawRecording;
            m_proresSupported = prores;
            m_h264HardwareEncoding = hardwareH264;
            m_h264MaxWidth = h264Width;
            m_h264MaxFps = h264Fps;
            emit recordingCapabilitiesChanged();
        }
    }
    if (state.contains(QStringLiteral("standby")) || state.contains(QStringLiteral("state"))) {
        const bool standby = state.contains(QStringLiteral("standby"))
            ? state.value(QStringLiteral("standby")).toBool()
            : state.value(QStringLiteral("state")).toString() == QStringLiteral("standby");
        if (m_standby != standby) {
            m_standby = standby;
            emit standbyChanged();
        }
    }
    if (state.contains(QStringLiteral("sensor"))) {
        setSensorNameValue(state.value(QStringLiteral("sensor")).toString());
    } else if (state.contains(QStringLiteral("capture")) &&
               state.value(QStringLiteral("capture")).isObject()) {
        const QJsonObject capture = state.value(QStringLiteral("capture")).toObject();
        if (capture.contains(QStringLiteral("sensor")))
            setSensorNameValue(capture.value(QStringLiteral("sensor")).toString());
    }

    const bool previousAutoExposure = autoExposureEnabled();
    if (state.contains(QStringLiteral("iso_auto")))
        m_isoAuto = state.value(QStringLiteral("iso_auto")).toBool(m_isoAuto);

    if (state.contains(QStringLiteral("shutter_auto")))
        m_shutterAuto = state.value(QStringLiteral("shutter_auto")).toBool(m_shutterAuto);

    const double ev = state.value(QStringLiteral("exposure_compensation")).toDouble(m_exposureCompensation);
    const bool evAvailable = state.value(QStringLiteral("exposure_compensation_available")).toBool(m_exposureCompensationAvailable);
    if (ev != m_exposureCompensation || evAvailable != m_exposureCompensationAvailable || previousAutoExposure != autoExposureEnabled()) {
        m_exposureCompensation = ev;
        m_exposureCompensationAvailable = evAvailable;
        emit exposureCompensationChanged();
    }

    const QString metering = state.value(QStringLiteral("exposure_metering")).toString(m_exposureMetering);
    QVariantList meteringOptions = m_exposureMeteringOptions;
    if (state.value(QStringLiteral("exposure_metering_modes")).isArray()) {
        meteringOptions.clear();
        for (const auto &entry : state.value(QStringLiteral("exposure_metering_modes")).toArray()) {
            const QString mode = entry.toString();
            const QString label = mode == QStringLiteral("centre-weighted") ? QStringLiteral("Center-weighted")
                : mode == QStringLiteral("matrix") ? QStringLiteral("Average")
                : mode == QStringLiteral("spot") ? QStringLiteral("Spot") : QString();
            if (!label.isEmpty())
                meteringOptions.append(QVariantMap{{QStringLiteral("value"), mode}, {QStringLiteral("label"), label}});
        }
    }
    if (metering != m_exposureMetering || meteringOptions != m_exposureMeteringOptions) {
        m_exposureMetering = metering;
        m_exposureMeteringOptions = meteringOptions;
        emit exposureMeteringChanged();
    }

    if (state.contains(QStringLiteral("wb_auto")))
        m_whiteBalanceAuto = state.value(QStringLiteral("wb_auto")).toBool(m_whiteBalanceAuto);

    if (state.contains(QStringLiteral("wb_available"))) {
        const bool available = state.value(QStringLiteral("wb_available")).toBool();
        if (available != m_whiteBalanceAvailable) {
            m_whiteBalanceAvailable = available;
            emit whiteBalanceAvailableChanged();
        }
    }

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

    if (m_whiteBalanceAuto) {
        setWhiteBalance(QStringLiteral("Auto"));
    } else if (state.contains(QStringLiteral("wb_kelvin"))) {
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

    if (state.contains(QStringLiteral("lossy_dng_ratio"))) {
        const QString ratio = parseLossyDngRatioValue(state.value(QStringLiteral("lossy_dng_ratio")).toString());
        if (!ratio.isEmpty())
            m_lossyDngRatio = ratio;
    }

    QString stateRecordingFormat;
    if (state.contains(QStringLiteral("prores_profile"))) {
        setProresProfileValue(state.value(QStringLiteral("prores_profile")).toString(QStringLiteral("422")));
    }
    if (state.contains(QStringLiteral("prores_color_profile")))
        setProresColorProfileValue(state.value(QStringLiteral("prores_color_profile")).toString(QStringLiteral("standard")));
    if (state.contains(QStringLiteral("cdng_preview_color_profile")))
        setCdngPreviewColorProfileValue(state.value(QStringLiteral("cdng_preview_color_profile")).toString(QStringLiteral("standard")));
    if (state.contains(QStringLiteral("recording_format"))) {
        stateRecordingFormat = parseRecordingFormatValue(state.value(QStringLiteral("recording_format")).toString());
        if (!stateRecordingFormat.isEmpty())
            setRecordingFormatValue(formatRecordingFormatValue(stateRecordingFormat));
    }

    if (stateRecordingFormat == QStringLiteral("lossydng") ||
        (stateRecordingFormat.isEmpty() && state.contains(QStringLiteral("lossy_dng_ratio")) &&
         isLossyCdngCompressionValue(m_cdngCompression))) {
        const QString compression = cdngCompressionForLossyRatio(m_lossyDngRatio);
        if (m_cdngCompression != compression) {
            m_cdngCompression = compression;
            emit cdngCompressionChanged();
        }
    } else if (state.contains(QStringLiteral("cdng_compression"))) {
        const QString value = state.value(QStringLiteral("cdng_compression")).toString();
        const QString normalized = normalizeCdngCompressionValue(value);
        if ((normalized == QStringLiteral("none") || normalized == QStringLiteral("lossless_jpeg")) &&
            m_cdngCompression != normalized) {
            m_cdngCompression = normalized;
            emit cdngCompressionChanged();
        }
    }

    if (state.contains(QStringLiteral("tuning_file")))
        setTuningFileValue(state.value(QStringLiteral("tuning_file")).toString());

    if (state.contains(QStringLiteral("raw_bit_depth")))
        setRawBitDepthValue(formatRawBitDepthValue(state.value(QStringLiteral("raw_bit_depth")).toInt(12)));

    if (state.contains(QStringLiteral("h264_width")) && state.contains(QStringLiteral("h264_height"))) {
        setH264ResolutionValue(formatH264ResolutionValue(
            state.value(QStringLiteral("h264_width")).toInt(0),
            state.value(QStringLiteral("h264_height")).toInt(0)));
    } else if (state.contains(QStringLiteral("h264_resolution"))) {
        int h264Width = 0;
        int h264Height = 0;
        if (parseH264ResolutionValue(state.value(QStringLiteral("h264_resolution")).toString(),
                                     &h264Width,
                                     &h264Height)) {
            setH264ResolutionValue(formatH264ResolutionValue(h264Width, h264Height));
        }
    }

    if (state.contains(QStringLiteral("timecode_mode"))) {
        const QString normalized = parseTimecodeModeValue(state.value(QStringLiteral("timecode_mode")).toString());
        const QString displayValue = normalized == QStringLiteral("rec_run")
            ? QStringLiteral("Rec Run")
            : QStringLiteral("Free Run");
        m_settings.setValue(QStringLiteral("timecodeMode"), displayValue);
        m_settings.sync();
    }

    if (state.contains(QStringLiteral("af_mode")))
        setAutofocusModeValue(formatAutofocusModeValue(state.value(QStringLiteral("af_mode")).toString()));

    if (state.contains(QStringLiteral("af_range")))
        setAutofocusRangeValue(formatAutofocusRangeValue(state.value(QStringLiteral("af_range")).toString()));

    if (state.contains(QStringLiteral("af_speed")))
        setAutofocusSpeedValue(formatAutofocusSpeedValue(state.value(QStringLiteral("af_speed")).toString()));

    if (state.contains(QStringLiteral("af_window_x")) &&
        state.contains(QStringLiteral("af_window_y")) &&
        state.contains(QStringLiteral("af_window_w")) &&
        state.contains(QStringLiteral("af_window_h"))) {
        m_afX = state.value(QStringLiteral("af_window_x")).toDouble(0.25);
        m_afY = state.value(QStringLiteral("af_window_y")).toDouble(0.25);
        m_afWidth = state.value(QStringLiteral("af_window_w")).toDouble(0.5);
        m_afHeight = state.value(QStringLiteral("af_window_h")).toDouble(0.5);
        setAutofocusWindowValue(formatAutofocusWindowValue(m_afX, m_afY, m_afWidth, m_afHeight));
    }

    if (state.contains(QStringLiteral("recording")))
        setRecordingState(state.value(QStringLiteral("recording")).toBool(false));

    if (state.contains(QStringLiteral("photo_write_error"))) {
        const QString error = state.value(QStringLiteral("photo_write_error")).toString();
        if (!error.isEmpty() && error != m_lastPhotoWriteError) {
            setLastError(error);
            emit photoCaptureFailed(error);
        }
        m_lastPhotoWriteError = error;
    }
    const qint64 writeRevision = state.value(QStringLiteral("media_write_revision")).toInteger(-1);
    if (state.contains(QStringLiteral("media_writes_pending")) && writeRevision >= m_mediaWriteRevision) {
        m_mediaWriteRevision = writeRevision;
        const bool pending = state.value(QStringLiteral("media_writes_pending")).toBool();
        if (pending != m_mediaWritesPending) {
            m_mediaWritesPending = pending;
            emit mediaWritesPendingChanged();
        }
    }
    if (state.contains(QStringLiteral("still_exposure_active"))) {
        m_stillExposureActive = state.value(QStringLiteral("still_exposure_active")).toBool();
        m_stillExposureRemainingSeconds = std::max(0.0, state.value(QStringLiteral("still_exposure_remaining_seconds")).toDouble());
        m_stillExposureTimer.start();
        emit stillExposureChanged();
    }

    if (state.contains(QStringLiteral("timelapse_active")))
        setTimelapseActiveState(state.value(QStringLiteral("timelapse_active")).toBool(false));
    if (state.contains(QStringLiteral("timelapse_shot_count")))
        setTimelapseShotCount(static_cast<qulonglong>(std::max<qint64>(0,
            state.value(QStringLiteral("timelapse_shot_count")).toInteger())));

    if (state.contains(QStringLiteral("photo_mode_enabled")) ||
        state.contains(QStringLiteral("photo_bracketing_enabled")) ||
        state.contains(QStringLiteral("photo_bracketing_ev")) ||
        state.contains(QStringLiteral("photo_format")) ||
        state.contains(QStringLiteral("photo_timelapse"))) {
        const QString photoTimelapse = state.contains(QStringLiteral("photo_timelapse"))
            ? state.value(QStringLiteral("photo_timelapse")).toString(QStringLiteral("Off"))
            : m_settings.value(QStringLiteral("photoTimelapse"), QStringLiteral("Off")).toString();
        bool photoModeEnabled = state.contains(QStringLiteral("photo_mode_enabled"))
            ? state.value(QStringLiteral("photo_mode_enabled")).toBool(false)
            : m_settings.value(QStringLiteral("photoModeEnabled"), false).toBool();
        if (photoTimelapse.compare(QStringLiteral("Off"), Qt::CaseInsensitive) != 0)
            photoModeEnabled = true;
        const QString photoFormat = state.contains(QStringLiteral("photo_format"))
            ? state.value(QStringLiteral("photo_format")).toString(QStringLiteral("DNG"))
            : m_settings.value(QStringLiteral("photoFormat"), QStringLiteral("DNG")).toString();
        const bool longExposure = state.contains(QStringLiteral("photo_long_exposure_enabled"))
            ? state.value(QStringLiteral("photo_long_exposure_enabled")).toBool()
            : m_settings.value(QStringLiteral("photoLongExposureEnabled"), false).toBool();
        const bool bracketing = state.contains(QStringLiteral("photo_bracketing_enabled"))
            ? state.value(QStringLiteral("photo_bracketing_enabled")).toBool()
            : m_settings.value(QStringLiteral("photoBracketingEnabled"), false).toBool();
        const double bracketingEv = state.contains(QStringLiteral("photo_bracketing_ev"))
            ? state.value(QStringLiteral("photo_bracketing_ev")).toDouble(1.0)
            : m_settings.value(QStringLiteral("photoBracketingEv"), 1.0).toDouble();
        emit sharedPhotoStateChanged(photoModeEnabled, photoFormat, photoTimelapse, longExposure, bracketing, bracketingEv);
    }
}

void ApertarControlBridge::ensureEventSocketConnected()
{
    if (m_eventSocket.state() == QLocalSocket::ConnectedState
        || m_eventSocket.state() == QLocalSocket::ConnectingState) {
        return;
    }

    if (m_eventSocket.state() != QLocalSocket::UnconnectedState)
        m_eventSocket.abort();

    m_eventSocket.connectToServer(qEnvironmentVariable("APERTAR_CONTROL_SOCKET", QStringLiteral("/tmp/apertar-core.sock")));
}

void ApertarControlBridge::processEventSocketMessages()
{
    while (m_eventSocket.canReadLine()) {
        const QByteArray line = m_eventSocket.readLine().trimmed();
        if (line.isEmpty())
            continue;

        const QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject())
            continue;

        const auto message = doc.object();
        if (m_pendingStateRequestId >= 0 && message.value(QStringLiteral("id")).toInt(-1) == m_pendingStateRequestId) {
            m_pendingStateRequestId = -1;
            setConnected(message.value(QStringLiteral("ok")).toBool(false));
            updateRefreshTimer();
        }
        applyCoreMessage(message);
    }
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
    if (value > 0.0 && value <= 1.0)
        return compactNumber(1.0 / value) + QStringLiteral("s");
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
    if ((width == 1920 && height == 1080) || (width == 1928 && height == 1090))
        return QStringLiteral("1920x1080");
    if ((width == 3840 && height == 2160) || (width == 3856 && height == 2180))
        return QStringLiteral("3840x2160");
    return QStringLiteral("%1x%2").arg(width).arg(height);
}

QString ApertarControlBridge::formatH264ResolutionValue(int width, int height) const
{
    if (width <= 0 || height <= 0)
        return QStringLiteral("Match Capture");
    return QStringLiteral("%1x%2").arg(width).arg(height);
}

QString ApertarControlBridge::formatRecordingFormatValue(const QString &value) const
{
    if (value.compare(QStringLiteral("prores"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("ProRes ") + (m_proresProfile == QStringLiteral("proxy") ? QStringLiteral("Proxy")
            : m_proresProfile == QStringLiteral("lt") ? QStringLiteral("LT")
            : m_proresProfile == QStringLiteral("hq") ? QStringLiteral("HQ") : QStringLiteral("422"));
    if (value.compare(QStringLiteral("lossydng"), Qt::CaseInsensitive) == 0 ||
        value.compare(QStringLiteral("lossy_dng"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("cDNG");
    if (value.compare(QStringLiteral("mlv"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("MLV");
    if (value.compare(QStringLiteral("h264"), Qt::CaseInsensitive) == 0 ||
        value.compare(QStringLiteral("h.264"), Qt::CaseInsensitive) == 0 ||
        value.compare(QStringLiteral("mp4"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("H.264");
    }
    return QStringLiteral("cDNG");
}

QString ApertarControlBridge::parseLossyDngRatioValue(const QString &displayValue) const
{
    QString normalized = displayValue.trimmed().toLower();
    normalized.remove(QStringLiteral(" "));
    normalized.remove(QStringLiteral("_"));
    normalized.remove(QStringLiteral("-"));
    if (normalized == QStringLiteral("5") ||
        normalized == QStringLiteral("5:1") ||
        normalized == QStringLiteral("5to1") ||
        normalized == QStringLiteral("r5") ||
        normalized == QStringLiteral("cdng5:1") ||
        normalized == QStringLiteral("cdng5to1") ||
        normalized == QStringLiteral("cinemadng5:1") ||
        normalized == QStringLiteral("cinemadng5to1") ||
        normalized == QStringLiteral("lossydng5to1")) {
        return QStringLiteral("5to1");
    }
    return QStringLiteral("3to1");
}

QString ApertarControlBridge::formatRawBitDepthValue(int value) const
{
    if (value == 8)
        return QStringLiteral("8-bit");
    return value == 10 ? QStringLiteral("10-bit") : QStringLiteral("12-bit");
}

QString ApertarControlBridge::formatAutofocusModeValue(const QString &value) const
{
    const QString normalized = parseAutofocusModeValue(value);
    if (normalized == QStringLiteral("manual"))
        return QStringLiteral("Manual");
    if (normalized == QStringLiteral("auto"))
        return QStringLiteral("Auto");
    return QStringLiteral("Continuous");
}

QString ApertarControlBridge::formatAutofocusRangeValue(const QString &value) const
{
    const QString normalized = parseAutofocusRangeValue(value);
    if (normalized == QStringLiteral("macro"))
        return QStringLiteral("Macro");
    if (normalized == QStringLiteral("full"))
        return QStringLiteral("Full");
    return QStringLiteral("Normal");
}

QString ApertarControlBridge::formatAutofocusSpeedValue(const QString &value) const
{
    return parseAutofocusSpeedValue(value) == QStringLiteral("fast")
        ? QStringLiteral("Fast")
        : QStringLiteral("Normal");
}

QString ApertarControlBridge::formatAutofocusWindowValue(double x, double y, double width, double height) const
{
    const bool center = qAbs(x - 0.25) < 0.001 &&
                        qAbs(y - 0.25) < 0.001 &&
                        qAbs(width - 0.5) < 0.001 &&
                        qAbs(height - 0.5) < 0.001;
    if (center)
        return QStringLiteral("Center");

    const bool fullFrame = qAbs(x) < 0.001 &&
                           qAbs(y) < 0.001 &&
                           qAbs(width - 1.0) < 0.001 &&
                           qAbs(height - 1.0) < 0.001;
    return fullFrame ? QStringLiteral("Full Frame") : QStringLiteral("Custom");
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
    if (normalized.endsWith(QLatin1Char('s'))) {
        const double seconds = normalized.chopped(1).toDouble();
        return seconds > 0.0 && seconds <= 30.0 ? 1.0 / seconds : 0.0;
    }
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
    if (normalized == QStringLiteral("1920x1080")) {
        if (width)
            *width = 1920;
        if (height)
            *height = 1080;
        return true;
    }
    if (normalized == QStringLiteral("1920x1038")) {
        if (width)
            *width = 1920;
        if (height)
            *height = 1038;
        return true;
    }
    if (normalized == QStringLiteral("1920x1012")) {
        if (width)
            *width = 1920;
        if (height)
            *height = 1012;
        return true;
    }
    if (normalized == QStringLiteral("1920x960")) {
        if (width)
            *width = 1920;
        if (height)
            *height = 960;
        return true;
    }
    if (normalized == QStringLiteral("1920x872")) {
        if (width)
            *width = 1920;
        if (height)
            *height = 872;
        return true;
    }
    if (normalized == QStringLiteral("1920x864")) {
        if (width)
            *width = 1920;
        if (height)
            *height = 864;
        return true;
    }
    if (normalized == QStringLiteral("1920x816")) {
        if (width)
            *width = 1920;
        if (height)
            *height = 816;
        return true;
    }
    if (normalized == QStringLiteral("1920x804")) {
        if (width)
            *width = 1920;
        if (height)
            *height = 804;
        return true;
    }
    if (normalized == QStringLiteral("1920x768")) {
        if (width)
            *width = 1920;
        if (height)
            *height = 768;
        return true;
    }
    if (normalized == QStringLiteral("1920x752")) {
        if (width)
            *width = 1920;
        if (height)
            *height = 752;
        return true;
    }
    if (normalized == QStringLiteral("3840x2160")) {
        if (width)
            *width = 3840;
        if (height)
            *height = 2160;
        return true;
    }
    if (normalized == QStringLiteral("3840x2076")) {
        if (width)
            *width = 3840;
        if (height)
            *height = 2076;
        return true;
    }
    if (normalized == QStringLiteral("3840x2024")) {
        if (width)
            *width = 3840;
        if (height)
            *height = 2024;
        return true;
    }
    if (normalized == QStringLiteral("3840x1920")) {
        if (width)
            *width = 3840;
        if (height)
            *height = 1920;
        return true;
    }
    if (normalized == QStringLiteral("3840x1744")) {
        if (width)
            *width = 3840;
        if (height)
            *height = 1744;
        return true;
    }
    if (normalized == QStringLiteral("3840x1728")) {
        if (width)
            *width = 3840;
        if (height)
            *height = 1728;
        return true;
    }
    if (normalized == QStringLiteral("3840x1632")) {
        if (width)
            *width = 3840;
        if (height)
            *height = 1632;
        return true;
    }
    if (normalized == QStringLiteral("3840x1608")) {
        if (width)
            *width = 3840;
        if (height)
            *height = 1608;
        return true;
    }
    if (normalized == QStringLiteral("3840x1536")) {
        if (width)
            *width = 3840;
        if (height)
            *height = 1536;
        return true;
    }
    if (normalized == QStringLiteral("3840x1504")) {
        if (width)
            *width = 3840;
        if (height)
            *height = 1504;
        return true;
    }
    if (normalized == QStringLiteral("1928x1090")) {
        if (width)
            *width = 1920;
        if (height)
            *height = 1080;
        return true;
    }
    if (normalized == QStringLiteral("3856x2180")) {
        if (width)
            *width = 3840;
        if (height)
            *height = 2160;
        return true;
    }

    const QRegularExpression genericSizePattern(QStringLiteral(R"(^(\d{2,5})x(\d{2,5})$)"));
    const QRegularExpressionMatch genericMatch = genericSizePattern.match(normalized);
    if (genericMatch.hasMatch()) {
        const int parsedWidth = genericMatch.captured(1).toInt();
        const int parsedHeight = genericMatch.captured(2).toInt();
        if (parsedWidth > 0 && parsedWidth <= 10000 && parsedHeight > 0 && parsedHeight <= 10000) {
            if (width)
                *width = parsedWidth;
            if (height)
                *height = parsedHeight;
            return true;
        }
    }

    return false;
}

bool ApertarControlBridge::parseH264ResolutionValue(const QString &displayValue, int *width, int *height) const
{
    QString normalized = displayValue.trimmed().toLower();
    normalized.remove(QLatin1Char(' '));
    normalized.remove(QStringLiteral("h.264"));
    normalized.remove(QStringLiteral("h264"));
    normalized.remove(QStringLiteral("resolution"));

    if (normalized.isEmpty() ||
        normalized == QStringLiteral("match") ||
        normalized == QStringLiteral("matchcapture") ||
        normalized == QStringLiteral("capture")) {
        if (width)
            *width = 0;
        if (height)
            *height = 0;
        return true;
    }

    const QRegularExpression sizePattern(QStringLiteral(R"(^(\d+)x(\d+)$)"));
    const QRegularExpressionMatch match = sizePattern.match(normalized);
    if (!match.hasMatch())
        return false;

    const int parsedWidth = match.captured(1).toInt();
    const int parsedHeight = match.captured(2).toInt();
    const QString selectedSize = QStringLiteral("%1x%2").arg(parsedWidth).arg(parsedHeight);
    static const QStringList supportedSizes = {
        QStringLiteral("1920x1080"), QStringLiteral("1920x1038"), QStringLiteral("1920x1012"),
        QStringLiteral("1920x960"), QStringLiteral("1920x872"), QStringLiteral("1920x864"),
        QStringLiteral("1920x816"), QStringLiteral("1920x804"), QStringLiteral("1920x768"),
        QStringLiteral("1920x752"),
        QStringLiteral("2560x1440"), QStringLiteral("2560x1384"), QStringLiteral("2560x1348"),
        QStringLiteral("2560x1280"), QStringLiteral("2560x1164"), QStringLiteral("2560x1152"),
        QStringLiteral("2560x1088"), QStringLiteral("2560x1072"), QStringLiteral("2560x1024"),
        QStringLiteral("2560x1004"),
        QStringLiteral("3840x2160"), QStringLiteral("3840x2076"), QStringLiteral("3840x2024"),
        QStringLiteral("3840x1920"), QStringLiteral("3840x1744"), QStringLiteral("3840x1728"),
        QStringLiteral("3840x1632"), QStringLiteral("3840x1608"), QStringLiteral("3840x1536"),
        QStringLiteral("3840x1504"),
        QStringLiteral("4056x3040"), QStringLiteral("4056x2280"), QStringLiteral("4056x2192"),
        QStringLiteral("4056x2160"), QStringLiteral("4056x2136"), QStringLiteral("4056x2028"),
        QStringLiteral("4056x1844"), QStringLiteral("4056x1828"), QStringLiteral("4056x1728"),
        QStringLiteral("4056x1696"), QStringLiteral("4056x1624"), QStringLiteral("4056x1592")
    };
    const bool supported = supportedSizes.contains(selectedSize);
    if (!supported)
        return false;

    if (width)
        *width = parsedWidth;
    if (height)
        *height = parsedHeight;
    return true;
}

QString ApertarControlBridge::parseRecordingFormatValue(const QString &displayValue) const
{
    const QString normalized = displayValue.trimmed().toLower();
    if (normalized.startsWith(QStringLiteral("prores")))
        return QStringLiteral("prores");
    QString compact = normalized;
    compact.remove(QStringLiteral(" "));
    compact.remove(QStringLiteral("_"));
    compact.remove(QStringLiteral("-"));
    if (compact == QStringLiteral("lossydng") ||
        compact == QStringLiteral("cdng3:1") ||
        compact == QStringLiteral("cdng3to1") ||
        compact == QStringLiteral("cdng5:1") ||
        compact == QStringLiteral("cdng5to1") ||
        compact == QStringLiteral("cinemadng3:1") ||
        compact == QStringLiteral("cinemadng3to1") ||
        compact == QStringLiteral("cinemadng5:1") ||
        compact == QStringLiteral("cinemadng5to1"))
        return QStringLiteral("lossydng");
    if (normalized == QStringLiteral("mlv"))
        return QStringLiteral("mlv");
    if (normalized == QStringLiteral("h264") ||
        normalized == QStringLiteral("h.264") ||
        normalized == QStringLiteral("mp4")) {
        return QStringLiteral("h264");
    }
    if (normalized == QStringLiteral("cdng"))
        return QStringLiteral("cdng");
    if (normalized == QStringLiteral("c.dng"))
        return QStringLiteral("cdng");
    return QString();
}

int ApertarControlBridge::parseRawBitDepthValue(const QString &displayValue) const
{
    QString normalized = displayValue.trimmed().toLower();
    normalized.remove(QStringLiteral("-bit"));
    normalized.remove(QStringLiteral("bit"));
    normalized.remove(QLatin1Char(' '));
    const int parsed = normalized.toInt();
    if (parsed == 8)
        return 8;
    return parsed == 10 ? 10 : 12;
}

QString ApertarControlBridge::parseTimecodeModeValue(const QString &displayValue) const
{
    const QString normalized = displayValue.trimmed().toLower();
    if (normalized == QStringLiteral("free run"))
        return QStringLiteral("free_run");
    if (normalized == QStringLiteral("rec run"))
        return QStringLiteral("rec_run");
    if (normalized == QStringLiteral("free_run"))
        return QStringLiteral("free_run");
    if (normalized == QStringLiteral("rec_run"))
        return QStringLiteral("rec_run");
    return QString();
}

QString ApertarControlBridge::parseAutofocusModeValue(const QString &displayValue) const
{
    const QString normalized = displayValue.trimmed().toLower();
    if (normalized == QStringLiteral("manual"))
        return QStringLiteral("manual");
    if (normalized == QStringLiteral("auto"))
        return QStringLiteral("auto");
    if (normalized == QStringLiteral("continuous"))
        return QStringLiteral("continuous");
    return QStringLiteral("continuous");
}

QString ApertarControlBridge::parseAutofocusRangeValue(const QString &displayValue) const
{
    const QString normalized = displayValue.trimmed().toLower();
    if (normalized == QStringLiteral("macro"))
        return QStringLiteral("macro");
    if (normalized == QStringLiteral("full"))
        return QStringLiteral("full");
    if (normalized == QStringLiteral("normal"))
        return QStringLiteral("normal");
    return QStringLiteral("normal");
}

QString ApertarControlBridge::parseAutofocusSpeedValue(const QString &displayValue) const
{
    const QString normalized = displayValue.trimmed().toLower();
    if (normalized == QStringLiteral("fast"))
        return QStringLiteral("fast");
    return QStringLiteral("normal");
}

bool ApertarControlBridge::autofocusWindowPreset(const QString &displayValue,
                                                 double *x,
                                                 double *y,
                                                 double *width,
                                                 double *height) const
{
    const QString normalized = displayValue.trimmed().toLower();
    if (normalized == QStringLiteral("full") || normalized == QStringLiteral("full frame") ||
        normalized == QStringLiteral("full_frame")) {
        if (x)
            *x = 0.0;
        if (y)
            *y = 0.0;
        if (width)
            *width = 1.0;
        if (height)
            *height = 1.0;
        return true;
    }

    if (normalized == QStringLiteral("center") || normalized == QStringLiteral("centre") ||
        normalized == QStringLiteral("center box") || normalized == QStringLiteral("centre box") ||
        normalized == QStringLiteral("custom")) {
        if (x)
            *x = 0.25;
        if (y)
            *y = 0.25;
        if (width)
            *width = 0.5;
        if (height)
            *height = 0.5;
        return true;
    }

    return false;
}

QString ApertarControlBridge::audioDeviceIdFromSelection(const QString &displayValue) const
{
    if (displayValue.trimmed().isEmpty() || isAutoSelection(displayValue))
        return QString();

    const QString normalized = displayValue.trimmed();
    if (normalized.startsWith(QStringLiteral("hdmi:"), Qt::CaseInsensitive)
        || normalized.startsWith(QStringLiteral("sysdefault:"), Qt::CaseInsensitive)
        || normalized.compare(QStringLiteral("default"), Qt::CaseInsensitive) == 0
        || normalized.startsWith(QStringLiteral("plughw:"), Qt::CaseInsensitive)) {
        return normalized;
    }

    static const QRegularExpression pattern(QStringLiteral(R"(((?:plug)?hw:\d+,\d+))"),
                                            QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = pattern.match(normalized);
    if (match.hasMatch()) {
        const QString deviceId = match.captured(1);
        if (deviceId.startsWith(QStringLiteral("hw:"), Qt::CaseInsensitive))
            return QStringLiteral("plughw:%1").arg(deviceId.mid(3));
        return deviceId;
    }

    return QString();
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

void ApertarControlBridge::setRecordingFormatValue(const QString &value)
{
    if (m_recordingFormat == value)
        return;

    m_recordingFormat = value;
    emit recordingFormatChanged();
}

void ApertarControlBridge::setProresColorProfileValue(const QString &value)
{
    if (m_proresColorProfile == value) return;
    m_proresColorProfile = value;
    emit proresColorProfileChanged();
}

void ApertarControlBridge::setCdngPreviewColorProfileValue(const QString &value)
{
    if (m_cdngPreviewColorProfile == value) return;
    m_cdngPreviewColorProfile = value;
    emit cdngPreviewColorProfileChanged();
}

void ApertarControlBridge::setProresProfileValue(const QString &value)
{
    if (m_proresProfile == value)
        return;

    m_proresProfile = value;
    emit proresProfileChanged();
    if (m_recordingFormat.startsWith(QStringLiteral("ProRes")))
        setRecordingFormatValue(formatRecordingFormatValue(QStringLiteral("prores")));
}

void ApertarControlBridge::setTuningFileValue(const QString &value)
{
    const QString normalized = normalizeLocalFilePath(value);
    if (m_tuningFile == normalized)
        return;

    m_tuningFile = normalized;
    emit tuningFileChanged();
}

void ApertarControlBridge::setRawBitDepthValue(const QString &value)
{
    const QString normalized = formatRawBitDepthValue(parseRawBitDepthValue(value));
    if (m_rawBitDepth == normalized)
        return;

    m_rawBitDepth = normalized;
    emit rawBitDepthChanged();
}

void ApertarControlBridge::setH264ResolutionValue(const QString &value)
{
    int width = 0;
    int height = 0;
    if (!parseH264ResolutionValue(value, &width, &height))
        return;

    const QString normalized = formatH264ResolutionValue(width, height);
    if (m_h264Resolution == normalized)
        return;

    m_h264Resolution = normalized;
    emit h264ResolutionChanged();
}

void ApertarControlBridge::setSensorNameValue(const QString &value)
{
    const QString normalized = value.trimmed().isEmpty()
        ? QStringLiteral("Unknown")
        : value.trimmed();
    if (m_sensorName == normalized)
        return;

    m_sensorName = normalized;
    emit sensorNameChanged();
}

void ApertarControlBridge::setAutofocusModeValue(const QString &value)
{
    if (m_autofocusMode == value)
        return;

    m_autofocusMode = value;
    emit autofocusModeChanged();
}

void ApertarControlBridge::setAutofocusRangeValue(const QString &value)
{
    if (m_autofocusRange == value)
        return;

    m_autofocusRange = value;
    emit autofocusRangeChanged();
}

void ApertarControlBridge::setAutofocusSpeedValue(const QString &value)
{
    if (m_autofocusSpeed == value)
        return;

    m_autofocusSpeed = value;
    emit autofocusSpeedChanged();
}

void ApertarControlBridge::setAutofocusWindowValue(const QString &value)
{
    if (m_autofocusWindow == value)
        return;

    m_autofocusWindow = value;
    emit autofocusWindowChanged();
}

void ApertarControlBridge::setRecordingState(bool value)
{
    if (m_recording == value)
        return;

    m_recording = value;
    if (!m_recording)
        setAudioInputLevel(0);
    emit recordingChanged();
}

void ApertarControlBridge::setTimelapseActiveState(bool value)
{
    if (m_timelapseActive == value)
        return;

    m_timelapseActive = value;
    emit timelapseActiveChanged();
}

void ApertarControlBridge::setTimelapseShotCount(qulonglong value)
{
    if (m_timelapseShotCount == value) return;
    m_timelapseShotCount = value;
    emit timelapseShotCountChanged();
}

void ApertarControlBridge::setAudioInputLevel(int value)
{
    const int clamped = qBound(0, value, 100);
    if (m_audioInputLevel == clamped)
        return;

    m_audioInputLevel = clamped;
    emit audioInputLevelChanged();
}

void ApertarControlBridge::setConnected(bool value)
{
    if (!value) m_mediaWriteRevision = -1;
    if (m_connected == value)
        return;

    m_connected = value;
    if (!value && m_stillExposureActive) {
        m_stillExposureActive = false;
        m_stillExposureRemainingSeconds = 0;
        emit stillExposureChanged();
    }
    emit connectedChanged();
}

void ApertarControlBridge::setLastError(const QString &value)
{
    if (m_lastError == value)
        return;

    m_lastError = value;
    emit lastErrorChanged();
}
