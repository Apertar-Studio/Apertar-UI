#include "SettingsBridge.hpp"
#include "ApertarControlBridge.hpp"

#include <algorithm>
#include <cmath>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>
#include <QRegularExpression>

namespace {

QString stringValue(const QJsonObject &object, const char *key, const QString &fallback = QString())
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isString() ? value.toString() : fallback;
}

bool boolValue(const QJsonObject &object, const char *key, bool fallback)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isBool() ? value.toBool() : fallback;
}

int intValue(const QJsonObject &object, const char *key, int fallback)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isDouble() ? value.toInt(fallback) : fallback;
}

double doubleValue(const QJsonObject &object, const char *key, double fallback)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isDouble() ? value.toDouble(fallback) : fallback;
}

double clampCameraControlsOpacity(double value)
{
    return qBound(0.15, value, 1.0);
}

QString normalizeCameraControlsMode(const QString &value)
{
    return value.trimmed().compare(QStringLiteral("Dark"), Qt::CaseInsensitive) == 0
        ? QStringLiteral("Dark")
        : QStringLiteral("Light");
}

QStringList parseSimpleStringList(const QString &value);
QStringList stringListValue(const QJsonObject &object, const char *key, const QStringList &fallback = {})
{
    const QJsonValue value = object.value(QLatin1String(key));
    if (value.isString()) return parseSimpleStringList(value.toString());
    if (!value.isArray())
        return fallback;

    QStringList out;
    const QJsonArray values = value.toArray();
    out.reserve(values.size());
    for (const QJsonValue &entry : values) {
        if (entry.isString())
            out.push_back(entry.toString());
    }
    return out;
}

QStringList parseSimpleStringList(const QString &value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty())
        return {};

    const QChar delimiter = trimmed.contains(QLatin1Char('|')) ? QLatin1Char('|') : QLatin1Char(',');
    QStringList parts = trimmed.split(delimiter, Qt::SkipEmptyParts);
    QStringList out;
    out.reserve(parts.size());
    for (QString part : parts) {
        part = part.trimmed();
        if (part.size() >= 2 && part.startsWith(QLatin1Char('"')) && part.endsWith(QLatin1Char('"')))
            part = part.mid(1, part.size() - 2).trimmed();
        if (!part.isEmpty())
            out.push_back(part);
    }
    return out;
}

QStringList settingsListValue(const QVariant &value)
{
    if (!value.isValid())
        return {};

    if (value.metaType().id() == QMetaType::QStringList)
        return value.toStringList();

    return parseSimpleStringList(value.toString());
}

QString normalizeLutPreviewPath(QString value)
{
    value = value.trimmed();
    if (value.startsWith(QStringLiteral("file://")))
        value = QUrl(value).toLocalFile();
    if (value.isEmpty())
        return {};
    return QDir::cleanPath(value);
}

QString normalizeTuningFilePath(QString value)
{
    value = value.trimmed();
    if (value.startsWith(QStringLiteral("file://")))
        value = QUrl(value).toLocalFile();
    if (value.isEmpty())
        return {};
    return QDir::cleanPath(value);
}

QString normalizeAssistColor(const QString &value, const QString &fallback)
{
    const QString normalized = value.trimmed();
    if (normalized.compare(QStringLiteral("Red"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("Red");
    if (normalized.compare(QStringLiteral("Green"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("Green");
    if (normalized.compare(QStringLiteral("Blue"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("Blue");
    if (normalized.compare(QStringLiteral("Yellow"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("Yellow");
    if (normalized.compare(QStringLiteral("Pink"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("Pink");
    return fallback;
}

QStringList lutSearchDirectories()
{
    QStringList directories;
    directories << QStringLiteral("/home/pi/LUTs");

    const QString mediaRoot = QString::fromLocal8Bit(qgetenv("APERTAR_MEDIA_ROOT")).trimmed();
    if (!mediaRoot.isEmpty())
        directories << QDir(mediaRoot).absoluteFilePath(QStringLiteral("LUTs"));
    directories << QStringLiteral("/media/RAW/LUTs");

    QDir mediaDirectory(QStringLiteral("/media"));
    const QFileInfoList mediaEntries = mediaDirectory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : mediaEntries)
        directories << QDir(entry.absoluteFilePath()).absoluteFilePath(QStringLiteral("LUTs"));

    QStringList unique;
    for (const QString &directory : directories) {
        const QString clean = QDir::cleanPath(directory);
        if (!clean.isEmpty() && !unique.contains(clean))
            unique.push_back(clean);
    }
    return unique;
}

QStringList tuningFileSearchDirectories()
{
    QStringList directories;
    directories << QStringLiteral("/home/pi/Tuning-Files");

    const QString mediaRoot = QString::fromLocal8Bit(qgetenv("APERTAR_MEDIA_ROOT")).trimmed();
    if (!mediaRoot.isEmpty())
        directories << QDir(mediaRoot).absoluteFilePath(QStringLiteral("Tuning-Files"));
    directories << QStringLiteral("/media/RAW/Tuning-Files");

    QDir mediaDirectory(QStringLiteral("/media"));
    const QFileInfoList mediaEntries = mediaDirectory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : mediaEntries)
        directories << QDir(entry.absoluteFilePath()).absoluteFilePath(QStringLiteral("Tuning-Files"));

    QStringList unique;
    for (const QString &directory : directories) {
        const QString clean = QDir::cleanPath(directory);
        if (!clean.isEmpty() && !unique.contains(clean))
            unique.push_back(clean);
    }
    return unique;
}

QString normalizePhotoFormat(const QString &value)
{
    const QString trimmed = value.trimmed();
    const QString normalized = trimmed.toLower();
    if (normalized == QStringLiteral("jpg") || normalized == QStringLiteral("jpeg"))
        return QStringLiteral("JPEG");
    if (normalized == QStringLiteral("png"))
        return QStringLiteral("PNG");
    if (normalized == QStringLiteral("tif") || normalized == QStringLiteral("tiff"))
        return QStringLiteral("TIFF");
    if (normalized == QStringLiteral("dng+jpeg") ||
        normalized == QStringLiteral("dng + jpeg") ||
        normalized == QStringLiteral("dng_jpeg") ||
        normalized == QStringLiteral("dng-jpeg")) {
        return QStringLiteral("DNG + JPEG");
    }
    if (normalized == QStringLiteral("dng+png") ||
        normalized == QStringLiteral("dng + png") ||
        normalized == QStringLiteral("dng_png") ||
        normalized == QStringLiteral("dng-png")) {
        return QStringLiteral("DNG + PNG");
    }
    if (normalized == QStringLiteral("dng+tif") ||
        normalized == QStringLiteral("dng + tif") ||
        normalized == QStringLiteral("dng_tif") ||
        normalized == QStringLiteral("dng-tif") ||
        normalized == QStringLiteral("dng+tiff") ||
        normalized == QStringLiteral("dng + tiff") ||
        normalized == QStringLiteral("dng_tiff") ||
        normalized == QStringLiteral("dng-tiff")) {
        return QStringLiteral("DNG + TIFF");
    }
    return QStringLiteral("DNG");
}

QString normalizePhotoTimelapse(const QString &value)
{
    const QString trimmed = value.trimmed();
    const QString normalized = trimmed.toLower();
    if (normalized.isEmpty() ||
        normalized == QStringLiteral("off") ||
        normalized == QStringLiteral("none") ||
        normalized == QStringLiteral("disabled")) {
        return QStringLiteral("Off");
    }

    const bool minutes = normalized.endsWith(QLatin1Char('m'));
    const bool hasUnit = minutes || normalized.endsWith(QLatin1Char('s'));
    const QString digits = hasUnit ? normalized.chopped(1) : normalized;
    const int dot = digits.indexOf(QLatin1Char('.'));
    if (digits.isEmpty() || (dot >= 0 && (minutes || dot + 2 != digits.size())))
        return QStringLiteral("Off");
    QString wholeDigits = digits;
    if (dot >= 0)
        wholeDigits.remove(dot, 1);
    if (wholeDigits.isEmpty() || !std::all_of(wholeDigits.begin(), wholeDigits.end(), [](QChar ch) { return ch >= QLatin1Char('0') && ch <= QLatin1Char('9'); }))
        return QStringLiteral("Off");

    bool ok = false;
    const int amount = wholeDigits.toInt(&ok);
    if (!ok || amount < 1 || amount > (minutes ? 1440 : (dot < 0 ? 86400 : 864000)))
        return QStringLiteral("Off");
    const int tenths = amount * (minutes ? 600 : (dot < 0 ? 10 : 1));
    if ((minutes || tenths >= 1200) && tenths % 600 == 0)
        return QString::number(tenths / 600) + QStringLiteral("m");
    return QString::number(tenths / 10)
        + (tenths % 10 ? QStringLiteral(".") + QString::number(tenths % 10) : QString())
        + QStringLiteral("s");
}

QString normalizeBatteryType(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("np-f") ||
        normalized == QStringLiteral("npf") ||
        normalized == QStringLiteral("sony np-f")) {
        return QStringLiteral("NP-F");
    }
    if (normalized == QStringLiteral("lp-e6") ||
        normalized == QStringLiteral("lpe6") ||
        normalized == QStringLiteral("canon lp-e6")) {
        return QStringLiteral("LP-E6");
    }
    if (normalized == QStringLiteral("usb-c / dc") ||
        normalized == QStringLiteral("usb-c") ||
        normalized == QStringLiteral("dc") ||
        normalized == QStringLiteral("dc input") ||
        normalized == QStringLiteral("external")) {
        return QStringLiteral("USB-C / DC");
    }
    if (normalized == QStringLiteral("custom"))
        return QStringLiteral("Custom");
    return QStringLiteral("V-Mount");
}

}

SettingsBridge::SettingsBridge(QObject *parent)
    : QObject(parent),
      m_settings("CinePi", "UI")
{
    auto normalizeExternalMonitorMode = [](const QString &value) {
        return value == QStringLiteral("Overlay Feed")
               || value == QStringLiteral("Assist Feed")
            ? QStringLiteral("Assist Feed")
            : QStringLiteral("Clean Feed");
    };
    auto normalizeExternalMonitorInfoOverlay = [](const QString &value) {
        return value == QStringLiteral("Off")
            ? QStringLiteral("Off")
            : QStringLiteral("On");
    };
    auto normalizeExternalMonitorOrientation = [](const QString &value) {
        if (value == QStringLiteral("Upside Down")) {
            return value;
        }
        return QStringLiteral("Landscape");
    };
    auto normalizeUiOrientation = [](const QString &value) {
        if (value == QStringLiteral("Left Side")
            || value == QStringLiteral("Right Side")
            || value == QStringLiteral("Upside Down")) {
            return value;
        }
        if (value == QStringLiteral("Left"))
            return QStringLiteral("Right Side");
        if (value == QStringLiteral("Right"))
            return QStringLiteral("Left Side");
        return QStringLiteral("Landscape");
    };
    auto normalizeAudioDeviceSelection = [](const QString &value) {
        return value.trimmed().compare(QStringLiteral("Auto"), Qt::CaseInsensitive) == 0
            ? QString()
            : value.trimmed();
    };

    m_zebraEnabled = m_settings.value("zebraEnabled", false).toBool();
    m_zebraThreshold = m_settings.value("zebraThreshold", 0.70).toFloat();

    m_focusPeakingEnabled = m_settings.value("focusPeakingEnabled", false).toBool();
    m_focusPeakingThreshold = m_settings.value("focusPeakingThreshold", 0.04).toFloat();
    m_focusPeakingColor = m_settings.value("focusPeakingColor", "Red").toString();

    m_grayscaleEnabled = m_settings.value("grayscaleEnabled", false).toBool();
    m_smpteEnabled = m_settings.value("smpteEnabled", false).toBool();
    m_anamorphicDesqueezeEnabled = m_settings.value("anamorphicDesqueezeEnabled", false).toBool();
    m_anamorphicRatio = m_settings.value("anamorphicRatio", "1.33x").toString();

    m_falseColorEnabled = m_settings.value("falseColorEnabled", false).toBool();
    m_falseColorMode = m_settings.value("falseColorMode", 0).toInt();
    m_lutPreviewEnabled = m_settings.value("lutPreviewEnabled", false).toBool();
    m_lutPreviewFile = normalizeLutPreviewPath(
        m_settings.value("lutPreviewFile", QString()).toString());
    refreshLutPreviewOptions();
    refreshTuningFileOptions();
    m_horizontalLevelEnabled = m_settings.value("horizontalLevelEnabled", false).toBool();
    m_horizontalLevelColor = normalizeAssistColor(
        m_settings.value("horizontalLevelColor", "Blue").toString(),
        QStringLiteral("Blue"));

    m_guidesEnabled = m_settings.value("guidesEnabled", true).toBool();
    m_guidesType = m_settings.value("guidesType", "Thirds").toString();
    m_guidesThickness = qBound(1, m_settings.value("guidesThickness", 1).toInt(), 4);

    m_centerMarkerEnabled = m_settings.value("centerMarkerEnabled", true).toBool();
    m_centerMarkerType = m_settings.value("centerMarkerType", "Circle/Dot").toString();
    m_uiOrientation = normalizeUiOrientation(
        m_settings.value("uiOrientation", "Landscape").toString());
    m_cameraControlsOpacity = clampCameraControlsOpacity(
        m_settings.value("cameraControlsOpacity", 1.0).toDouble());
    m_cameraControlsMode = normalizeCameraControlsMode(
        m_settings.value("cameraControlsMode", "Light").toString());
    m_externalMonitorEnabled = m_settings.value("externalMonitorEnabled", false).toBool();
    m_externalMonitorMode = normalizeExternalMonitorMode(
        m_settings.value("externalMonitorMode", "Clean Feed").toString());
    m_externalMonitorInfoOverlay = normalizeExternalMonitorInfoOverlay(
        m_settings.value("externalMonitorInfoOverlay", "On").toString());
    m_externalMonitorOrientation = normalizeExternalMonitorOrientation(
        m_settings.value("externalMonitorOrientation", "Landscape").toString());
    m_dateTimeOverlayEnabled = m_settings.value("dateTimeOverlayEnabled", false).toBool();
    m_timecodeMode = m_settings.value("timecodeMode", "Free Run").toString();
    m_photoModeEnabled = m_settings.value("photoModeEnabled", false).toBool();
    m_photoLongExposureEnabled = m_settings.value("photoLongExposureEnabled", false).toBool();
    m_photoBracketingEnabled = m_settings.value("photoBracketingEnabled", false).toBool();
    setPhotoBracketingEv(m_settings.value("photoBracketingEv", 1.0).toDouble());
    m_photoTimer = m_settings.value("photoTimer", "Off").toString();
    m_photoBurst = m_settings.value("photoBurst", "Single").toString();
    m_photoFormat = normalizePhotoFormat(m_settings.value("photoFormat", "DNG").toString());
    m_photoTimelapse = normalizePhotoTimelapse(m_settings.value("photoTimelapse", "Off").toString());
    m_soundRecorderEnabled = m_settings.value("soundRecorderEnabled", false).toBool();
    m_recordAudioEnabled = m_settings.value("recordAudioEnabled", false).toBool();
    m_recordGyroEnabled = m_settings.value("recordGyroEnabled", false).toBool();
    m_liveAudioMonitoringEnabled = m_settings.value("liveAudioMonitoringEnabled", true).toBool();
    m_audioMeterEnabled = m_settings.value("audioMeterEnabled", false).toBool();
    m_audioInputDevice = normalizeAudioDeviceSelection(
        m_settings.value("audioInputDevice", "").toString());
    m_audioOutputDevice = normalizeAudioDeviceSelection(
        m_settings.value("audioOutputDevice", "").toString());
    m_audioSampleFormat = m_settings.value("audioSampleFormat", "auto").toString();
    m_audioSampleRate = m_settings.value("audioSampleRate", 0).toInt();
    m_audioChannels = m_settings.value("audioChannels", "auto").toString();
    m_inputVolume = m_settings.value("inputVolume", 60).toInt();
    m_headphoneVolume = m_settings.value("headphoneVolume", 55).toInt();
    m_batteryType = normalizeBatteryType(m_settings.value("batteryType", "V-Mount").toString());
    m_batteryCapacity = m_settings.value("batteryCapacity", "150Wh").toString();
    m_customBatteryWh = m_settings.value("customBatteryWh", 150).toInt();
    m_customBatteryFullVoltage = m_settings.value("customBatteryFullVoltage", 16.8).toDouble();
    m_customBatteryEmptyVoltage = m_settings.value("customBatteryEmptyVoltage", 13.2).toDouble();
    m_sleepMode = m_settings.value("sleepMode", "Off").toString();
    m_standbyTimeoutSeconds = qBound(0, m_settings.value("standbyTimeoutSeconds", 0).toInt(), 86400);
    m_standbyUsbPowerOff = m_settings.value("standbyUsbPowerOff", false).toBool();
    m_monitoringOrder = settingsListValue(m_settings.value("monitoringOrder"));
    m_recordOrder = settingsListValue(m_settings.value("recordOrder"));
    m_audioOrder = settingsListValue(m_settings.value("audioOrder"));
    m_powerOrder = settingsListValue(m_settings.value("powerOrder"));
    m_systemOrder = settingsListValue(m_settings.value("systemOrder"));
    m_wifiOrder = settingsListValue(m_settings.value("wifiOrder"));
    m_infoOrder = settingsListValue(m_settings.value("infoOrder"));
    m_sidebarOrder = settingsListValue(m_settings.value("sidebarOrder"));
    m_favoritesOrder = settingsListValue(m_settings.value("favoritesOrder"));
    reloadPresetNames();
    m_presetDirectorySignature = currentPresetDirectorySignature();

    m_settingsFilePath = m_settings.fileName();
    updateSettingsFileState();
    connect(&m_externalSettingsWatcher, &QFileSystemWatcher::fileChanged,
            this, [this]() {
                m_settingsFileSize = -1; // Reconcile even equal-size writes within one mtime tick.
                pollExternalSettings();
                ensureExternalSettingsWatchPaths();
            });
    connect(&m_externalSettingsWatcher, &QFileSystemWatcher::directoryChanged,
            this, [this]() {
                reloadPresetNames();
                ensureExternalSettingsWatchPaths();
            });
    ensureExternalSettingsWatchPaths();

    // inotify handles normal changes immediately. This slow timer is only a
    // fallback for atomic replacements and filesystems that miss notifications.
    m_externalSettingsPollTimer.setInterval(10000);
    connect(&m_externalSettingsPollTimer, &QTimer::timeout,
            this, &SettingsBridge::pollExternalSettings);
    m_externalSettingsPollTimer.start();
}

void SettingsBridge::setControlBridge(ApertarControlBridge *controlBridge)
{
    if (m_controlBridge)
        disconnect(m_controlBridge, nullptr, this, nullptr);

    m_controlBridge = controlBridge;
    if (!m_controlBridge)
        return;

    connect(m_controlBridge,
            &ApertarControlBridge::sharedPhotoStateChanged,
            this,
            [this](bool photoModeEnabled, const QString &photoFormat, const QString &photoTimelapse, bool longExposureEnabled, bool bracketingEnabled, double bracketingEv) {
                setPhotoModeEnabled(photoModeEnabled);
                setPhotoLongExposureEnabled(longExposureEnabled);
                setPhotoBracketingEnabled(bracketingEnabled);
                setPhotoBracketingEv(bracketingEv);
                setPhotoFormat(photoFormat);
                setPhotoTimelapse(photoModeEnabled ? photoTimelapse : QStringLiteral("Off"));
            });
}

void SettingsBridge::setDefaultCameraControlsOpacity(double value)
{
    if (m_settings.contains(QStringLiteral("cameraControlsOpacity")))
        return;

    const double clamped = clampCameraControlsOpacity(value);
    if (qFuzzyCompare(m_cameraControlsOpacity + 1.0, clamped + 1.0))
        return;

    m_cameraControlsOpacity = clamped;
    emit cameraControlsOpacityChanged();
}

void SettingsBridge::updateSettingsFileState()
{
    // A local save can merge unseen Core/HDMI changes. Only acknowledge them
    // after pollExternalSettings has copied every field into the live UI.
    if (m_settingsSnapshotInitialized && !m_externalSettingsSyncing) return;
    m_settingsSnapshotInitialized = true;
    const QFileInfo info(m_settingsFilePath);
    if (!info.exists()) {
        m_settingsFileLastModified = QDateTime();
        m_settingsFileSize = -1;
        return;
    }

    m_settingsFileLastModified = info.lastModified();
    m_settingsFileSize = info.size();
}

void SettingsBridge::ensureExternalSettingsWatchPaths()
{
    if (!m_settingsFilePath.isEmpty()
        && QFileInfo::exists(m_settingsFilePath)
        && !m_externalSettingsWatcher.files().contains(m_settingsFilePath)) {
        m_externalSettingsWatcher.addPath(m_settingsFilePath);
    }

    const QString presetPath = presetsDirectoryPath();
    if (!presetPath.isEmpty()
        && QDir(presetPath).exists()
        && !m_externalSettingsWatcher.directories().contains(presetPath)) {
        m_externalSettingsWatcher.addPath(presetPath);
    }
}

void SettingsBridge::pollExternalSettings()
{
    if (m_externalSettingsSyncing || m_settingsFilePath.isEmpty())
        return;

    const QStringList presetDirectorySignature = currentPresetDirectorySignature();
    if (presetDirectorySignature != m_presetDirectorySignature)
        reloadPresetNames();

    const QFileInfo info(m_settingsFilePath);
    const QDateTime lastModified = info.exists() ? info.lastModified() : QDateTime();
    const qint64 fileSize = info.exists() ? info.size() : -1;
    if (lastModified == m_settingsFileLastModified && fileSize == m_settingsFileSize)
        return;

    m_externalSettingsSyncing = true;
    m_settings.sync();

    setZebraEnabled(m_settings.value(QStringLiteral("zebraEnabled"), m_zebraEnabled).toBool());
    setZebraThreshold(static_cast<float>(m_settings.value(QStringLiteral("zebraThreshold"), m_zebraThreshold).toDouble()));
    setFocusPeakingEnabled(m_settings.value(QStringLiteral("focusPeakingEnabled"), m_focusPeakingEnabled).toBool());
    setFocusPeakingThreshold(static_cast<float>(m_settings.value(QStringLiteral("focusPeakingThreshold"), m_focusPeakingThreshold).toDouble()));
    setFocusPeakingColor(m_settings.value(QStringLiteral("focusPeakingColor"), m_focusPeakingColor).toString());
    setAnamorphicDesqueezeEnabled(m_settings.value(QStringLiteral("anamorphicDesqueezeEnabled"), m_anamorphicDesqueezeEnabled).toBool());
    setAnamorphicRatio(m_settings.value(QStringLiteral("anamorphicRatio"), m_anamorphicRatio).toString());
    setFalseColorEnabled(m_settings.value(QStringLiteral("falseColorEnabled"), m_falseColorEnabled).toBool());
    setFalseColorMode(m_settings.value(QStringLiteral("falseColorMode"), m_falseColorMode).toInt());
    setLutPreviewEnabled(m_settings.value(QStringLiteral("lutPreviewEnabled"), m_lutPreviewEnabled).toBool());
    setLutPreviewFile(m_settings.value(QStringLiteral("lutPreviewFile"), m_lutPreviewFile).toString());
    setHorizontalLevelEnabled(m_settings.value(QStringLiteral("horizontalLevelEnabled"), m_horizontalLevelEnabled).toBool());
    setHorizontalLevelColor(m_settings.value(QStringLiteral("horizontalLevelColor"), m_horizontalLevelColor).toString());
    setGuidesEnabled(m_settings.value(QStringLiteral("guidesEnabled"), m_guidesEnabled).toBool());
    setGuidesType(m_settings.value(QStringLiteral("guidesType"), m_guidesType).toString());
    setGuidesThickness(m_settings.value(QStringLiteral("guidesThickness"), m_guidesThickness).toInt());
    setCenterMarkerEnabled(m_settings.value(QStringLiteral("centerMarkerEnabled"), m_centerMarkerEnabled).toBool());
    setCenterMarkerType(m_settings.value(QStringLiteral("centerMarkerType"), m_centerMarkerType).toString());
    setGrayscaleEnabled(m_settings.value(QStringLiteral("grayscaleEnabled"), m_grayscaleEnabled).toBool());
    setSmpteEnabled(m_settings.value(QStringLiteral("smpteEnabled"), m_smpteEnabled).toBool());
    setCameraControlsOpacity(m_settings.value(QStringLiteral("cameraControlsOpacity"), m_cameraControlsOpacity).toDouble());
    setCameraControlsMode(m_settings.value(QStringLiteral("cameraControlsMode"), m_cameraControlsMode).toString());
    setExternalMonitorEnabled(m_settings.value(QStringLiteral("externalMonitorEnabled"), m_externalMonitorEnabled).toBool());
    setExternalMonitorMode(m_settings.value(QStringLiteral("externalMonitorMode"), m_externalMonitorMode).toString());
    setExternalMonitorInfoOverlay(m_settings.value(QStringLiteral("externalMonitorInfoOverlay"), m_externalMonitorInfoOverlay).toString());
    setExternalMonitorOrientation(m_settings.value(QStringLiteral("externalMonitorOrientation"), m_externalMonitorOrientation).toString());
    setDateTimeOverlayEnabled(m_settings.value(QStringLiteral("dateTimeOverlayEnabled"), m_dateTimeOverlayEnabled).toBool());
    setPhotoModeEnabled(m_settings.value(QStringLiteral("photoModeEnabled"), m_photoModeEnabled).toBool());
    setPhotoLongExposureEnabled(m_settings.value(QStringLiteral("photoLongExposureEnabled"), m_photoLongExposureEnabled).toBool());
    setPhotoBracketingEnabled(m_settings.value(QStringLiteral("photoBracketingEnabled"), m_photoBracketingEnabled).toBool());
    setPhotoBracketingEv(m_settings.value(QStringLiteral("photoBracketingEv"), m_photoBracketingEv).toDouble());
    setPhotoTimer(m_settings.value(QStringLiteral("photoTimer"), m_photoTimer).toString());
    setPhotoBurst(m_settings.value(QStringLiteral("photoBurst"), m_photoBurst).toString());
    setPhotoFormat(m_settings.value(QStringLiteral("photoFormat"), m_photoFormat).toString());
    setPhotoTimelapse(m_settings.value(QStringLiteral("photoTimelapse"), m_photoTimelapse).toString());
    setUiOrientation(m_settings.value(QStringLiteral("uiOrientation"), m_uiOrientation).toString());
    setSoundRecorderEnabled(m_settings.value(QStringLiteral("soundRecorderEnabled"), m_soundRecorderEnabled).toBool());
    setRecordAudioEnabled(m_settings.value(QStringLiteral("recordAudioEnabled"), m_recordAudioEnabled).toBool());
    setRecordGyroEnabled(m_settings.value(QStringLiteral("recordGyroEnabled"), m_recordGyroEnabled).toBool());
    setLiveAudioMonitoringEnabled(m_settings.value(QStringLiteral("liveAudioMonitoringEnabled"), m_liveAudioMonitoringEnabled).toBool());
    setAudioMeterEnabled(m_settings.value(QStringLiteral("audioMeterEnabled"), m_audioMeterEnabled).toBool());
    setAudioInputDevice(m_settings.value(QStringLiteral("audioInputDevice"), m_audioInputDevice).toString());
    setAudioOutputDevice(m_settings.value(QStringLiteral("audioOutputDevice"), m_audioOutputDevice).toString());
    setInputVolume(m_settings.value(QStringLiteral("inputVolume"), m_inputVolume).toInt());
    setAudioSampleFormat(m_settings.value("audioSampleFormat", "auto").toString());
    setAudioSampleRate(m_settings.value("audioSampleRate", 0).toInt());
    setAudioChannels(m_settings.value("audioChannels", "auto").toString());
    setHeadphoneVolume(m_settings.value(QStringLiteral("headphoneVolume"), m_headphoneVolume).toInt());
    setBatteryType(m_settings.value(QStringLiteral("batteryType"), m_batteryType).toString());
    setBatteryCapacity(m_settings.value(QStringLiteral("batteryCapacity"), m_batteryCapacity).toString());
    setCustomBatteryWh(m_settings.value(QStringLiteral("customBatteryWh"), m_customBatteryWh).toInt());
    setCustomBatteryFullVoltage(m_settings.value(QStringLiteral("customBatteryFullVoltage"), m_customBatteryFullVoltage).toDouble());
    setCustomBatteryEmptyVoltage(m_settings.value(QStringLiteral("customBatteryEmptyVoltage"), m_customBatteryEmptyVoltage).toDouble());
    setSleepMode(m_settings.value(QStringLiteral("sleepMode"), m_sleepMode).toString());
    setStandbyTimeoutSeconds(m_settings.value(QStringLiteral("standbyTimeoutSeconds"), m_standbyTimeoutSeconds).toInt());
    setStandbyUsbPowerOff(m_settings.value(QStringLiteral("standbyUsbPowerOff"), m_standbyUsbPowerOff).toBool());
    setFavoritesOrder(settingsListValue(m_settings.value(QStringLiteral("favoritesOrder"))));

    QString timecodeMode = m_settings.value(QStringLiteral("timecodeMode"), m_timecodeMode).toString().trimmed();
    if (timecodeMode.compare(QStringLiteral("Rec Run"), Qt::CaseInsensitive) == 0
        || timecodeMode.compare(QStringLiteral("rec_run"), Qt::CaseInsensitive) == 0) {
        timecodeMode = QStringLiteral("Rec Run");
    } else {
        timecodeMode = QStringLiteral("Free Run");
    }
    setTimecodeMode(timecodeMode);

    m_settings.sync();
    updateSettingsFileState();
    m_externalSettingsSyncing = false;
}

bool SettingsBridge::zebraEnabled() const
{
    return m_zebraEnabled;
}

void SettingsBridge::setZebraEnabled(bool value)
{
    if (m_zebraEnabled == value)
        return;

    m_zebraEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("zebraEnabled", value);
    m_settings.sync();
    updateSettingsFileState();
    emit zebraEnabledChanged();
}

float SettingsBridge::zebraThreshold() const
{
    return m_zebraThreshold;
}

void SettingsBridge::setZebraThreshold(float value)
{
    if (qFuzzyCompare(m_zebraThreshold, value))
        return;

    m_zebraThreshold = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("zebraThreshold", value);
    emit zebraThresholdChanged();
}

bool SettingsBridge::focusPeakingEnabled() const
{
    return m_focusPeakingEnabled;
}

void SettingsBridge::setFocusPeakingEnabled(bool value)
{
    if (m_focusPeakingEnabled == value)
        return;

    m_focusPeakingEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("focusPeakingEnabled", value);
    m_settings.sync();
    updateSettingsFileState();
    emit focusPeakingEnabledChanged();
}

float SettingsBridge::focusPeakingThreshold() const
{
    return m_focusPeakingThreshold;
}

void SettingsBridge::setFocusPeakingThreshold(float value)
{
    if (qFuzzyCompare(m_focusPeakingThreshold, value))
        return;

    m_focusPeakingThreshold = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("focusPeakingThreshold", value);
    emit focusPeakingThresholdChanged();
}

QString SettingsBridge::focusPeakingColor() const
{
    return m_focusPeakingColor;
}

void SettingsBridge::setFocusPeakingColor(const QString &value)
{
    if (m_focusPeakingColor == value)
        return;

    m_focusPeakingColor = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("focusPeakingColor", value);
    emit focusPeakingColorChanged();
}

bool SettingsBridge::grayscaleEnabled() const
{
    return m_grayscaleEnabled;
}

void SettingsBridge::setGrayscaleEnabled(bool value)
{
    if (m_grayscaleEnabled == value)
        return;

    m_grayscaleEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("grayscaleEnabled", value);
    m_settings.sync();
    updateSettingsFileState();
    emit grayscaleEnabledChanged();
}

bool SettingsBridge::smpteEnabled() const
{
    return m_smpteEnabled;
}

void SettingsBridge::setSmpteEnabled(bool value)
{
    if (m_smpteEnabled == value)
        return;

    m_smpteEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("smpteEnabled", value);
    m_settings.sync();
    updateSettingsFileState();
    emit smpteEnabledChanged();
}

bool SettingsBridge::anamorphicDesqueezeEnabled() const
{
    return m_anamorphicDesqueezeEnabled;
}

void SettingsBridge::setAnamorphicDesqueezeEnabled(bool value)
{
    if (m_anamorphicDesqueezeEnabled == value)
        return;

    m_anamorphicDesqueezeEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("anamorphicDesqueezeEnabled", value);
    emit anamorphicDesqueezeEnabledChanged();
}

QString SettingsBridge::anamorphicRatio() const
{
    return m_anamorphicRatio;
}

void SettingsBridge::setAnamorphicRatio(const QString &value)
{
    if (m_anamorphicRatio == value)
        return;

    m_anamorphicRatio = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("anamorphicRatio", value);
    emit anamorphicRatioChanged();
}

bool SettingsBridge::falseColorEnabled() const
{
    return m_falseColorEnabled;
}

void SettingsBridge::setFalseColorEnabled(bool value)
{
    if (m_falseColorEnabled == value)
        return;

    m_falseColorEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("falseColorEnabled", value);
    m_settings.sync();
    updateSettingsFileState();
    emit falseColorEnabledChanged();
}

int SettingsBridge::falseColorMode() const
{
    return m_falseColorMode;
}

void SettingsBridge::setFalseColorMode(int value)
{
    if (m_falseColorMode == value)
        return;

    m_falseColorMode = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("falseColorMode", value);
    emit falseColorModeChanged();
}

bool SettingsBridge::lutPreviewEnabled() const
{
    return m_lutPreviewEnabled;
}

void SettingsBridge::setLutPreviewEnabled(bool value)
{
    if (m_lutPreviewEnabled == value)
        return;

    m_lutPreviewEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue(QStringLiteral("lutPreviewEnabled"), value);
    m_settings.sync();
    updateSettingsFileState();
    emit lutPreviewEnabledChanged();
}

QString SettingsBridge::lutPreviewFile() const
{
    return m_lutPreviewFile;
}

void SettingsBridge::setLutPreviewFile(const QString &value)
{
    const QString path = normalizeLutPreviewPath(value);
    if (m_lutPreviewFile == path)
        return;

    m_lutPreviewFile = path;
    if (!m_externalSettingsSyncing) m_settings.setValue(QStringLiteral("lutPreviewFile"), path);
    m_settings.sync();
    updateSettingsFileState();
    emit lutPreviewFileChanged();
}

QStringList SettingsBridge::lutPreviewOptions() const
{
    return m_lutPreviewOptions;
}

void SettingsBridge::refreshLutPreviewOptions()
{
    QStringList options;
    const QStringList nameFilters = {
        QStringLiteral("*.cube"),
        QStringLiteral("*.CUBE")
    };

    for (const QString &directoryPath : lutSearchDirectories()) {
        QDir directory(directoryPath);
        if (!directory.exists())
            continue;

        const QFileInfoList files = directory.entryInfoList(
            nameFilters,
            QDir::Files | QDir::Readable,
            QDir::Name | QDir::IgnoreCase);
        for (const QFileInfo &file : files) {
            const QString path = QDir::cleanPath(file.absoluteFilePath());
            if (!options.contains(path))
                options.push_back(path);
        }
    }

    if (options != m_lutPreviewOptions) {
        m_lutPreviewOptions = options;
        emit lutPreviewOptionsChanged();
    }
}

QStringList SettingsBridge::tuningFileOptions() const
{
    return m_tuningFileOptions;
}

void SettingsBridge::refreshTuningFileOptions()
{
    QStringList options;
    const QStringList nameFilters = {
        QStringLiteral("*.json"),
        QStringLiteral("*.JSON")
    };

    for (const QString &directoryPath : tuningFileSearchDirectories()) {
        QDir directory(directoryPath);
        if (!directory.exists())
            continue;

        const QFileInfoList files = directory.entryInfoList(
            nameFilters,
            QDir::Files | QDir::Readable,
            QDir::Name | QDir::IgnoreCase);
        for (const QFileInfo &file : files) {
            const QString path = normalizeTuningFilePath(file.absoluteFilePath());
            if (!path.isEmpty() && !options.contains(path))
                options.push_back(path);
        }
    }

    if (options != m_tuningFileOptions) {
        m_tuningFileOptions = options;
        emit tuningFileOptionsChanged();
    }
}

bool SettingsBridge::horizontalLevelEnabled() const
{
    return m_horizontalLevelEnabled;
}

void SettingsBridge::setHorizontalLevelEnabled(bool value)
{
    if (m_horizontalLevelEnabled == value)
        return;

    m_horizontalLevelEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue(QStringLiteral("horizontalLevelEnabled"), value);
    m_settings.sync();
    updateSettingsFileState();
    emit horizontalLevelEnabledChanged();
}

QString SettingsBridge::horizontalLevelColor() const
{
    return m_horizontalLevelColor;
}

void SettingsBridge::setHorizontalLevelColor(const QString &value)
{
    const QString normalized = normalizeAssistColor(value, QStringLiteral("Blue"));
    if (m_horizontalLevelColor == normalized)
        return;

    m_horizontalLevelColor = normalized;
    if (!m_externalSettingsSyncing) m_settings.setValue(QStringLiteral("horizontalLevelColor"), normalized);
    emit horizontalLevelColorChanged();
}

bool SettingsBridge::guidesEnabled() const
{
    return m_guidesEnabled;
}

void SettingsBridge::setGuidesEnabled(bool value)
{
    if (m_guidesEnabled == value)
        return;

    m_guidesEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("guidesEnabled", value);
    m_settings.sync();
    updateSettingsFileState();
    emit guidesEnabledChanged();
}

QString SettingsBridge::guidesType() const
{
    return m_guidesType;
}

void SettingsBridge::setGuidesType(const QString &value)
{
    if (m_guidesType == value)
        return;

    m_guidesType = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("guidesType", value);
    emit guidesTypeChanged();
}

int SettingsBridge::guidesThickness() const
{
    return m_guidesThickness;
}

void SettingsBridge::setGuidesThickness(int value)
{
    const int clamped = qBound(1, value, 4);
    if (m_guidesThickness == clamped)
        return;

    m_guidesThickness = clamped;
    if (!m_externalSettingsSyncing) m_settings.setValue("guidesThickness", clamped);
    emit guidesThicknessChanged();
}

bool SettingsBridge::centerMarkerEnabled() const
{
    return m_centerMarkerEnabled;
}

void SettingsBridge::setCenterMarkerEnabled(bool value)
{
    if (m_centerMarkerEnabled == value)
        return;

    m_centerMarkerEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("centerMarkerEnabled", value);
    m_settings.sync();
    updateSettingsFileState();
    emit centerMarkerEnabledChanged();
}

QString SettingsBridge::centerMarkerType() const
{
    return m_centerMarkerType;
}

void SettingsBridge::setCenterMarkerType(const QString &value)
{
    if (m_centerMarkerType == value)
        return;

    m_centerMarkerType = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("centerMarkerType", value);
    emit centerMarkerTypeChanged();
}

QString SettingsBridge::uiOrientation() const
{
    return m_uiOrientation;
}

void SettingsBridge::setUiOrientation(const QString &value)
{
    QString normalized = QStringLiteral("Landscape");
    if (value == QStringLiteral("Left Side")
        || value == QStringLiteral("Right Side")
        || value == QStringLiteral("Upside Down")) {
        normalized = value;
    } else if (value == QStringLiteral("Left")) {
        normalized = QStringLiteral("Right Side");
    } else if (value == QStringLiteral("Right")) {
        normalized = QStringLiteral("Left Side");
    }

    if (m_uiOrientation == normalized)
        return;

    m_uiOrientation = normalized;
    if (!m_externalSettingsSyncing) m_settings.setValue("uiOrientation", normalized);
    m_settings.sync();
    updateSettingsFileState();
    emit uiOrientationChanged();
}

double SettingsBridge::cameraControlsOpacity() const
{
    return m_cameraControlsOpacity;
}

void SettingsBridge::setCameraControlsOpacity(double value)
{
    const double clamped = clampCameraControlsOpacity(value);
    if (qFuzzyCompare(m_cameraControlsOpacity + 1.0, clamped + 1.0))
        return;

    m_cameraControlsOpacity = clamped;
    if (!m_externalSettingsSyncing) m_settings.setValue("cameraControlsOpacity", clamped);
    m_settings.sync();
    updateSettingsFileState();
    emit cameraControlsOpacityChanged();
}

QString SettingsBridge::cameraControlsMode() const
{
    return m_cameraControlsMode;
}

void SettingsBridge::setCameraControlsMode(const QString &value)
{
    const QString normalized = normalizeCameraControlsMode(value);
    if (m_cameraControlsMode == normalized)
        return;

    m_cameraControlsMode = normalized;
    if (!m_externalSettingsSyncing) m_settings.setValue("cameraControlsMode", normalized);
    m_settings.sync();
    updateSettingsFileState();
    emit cameraControlsModeChanged();
}

bool SettingsBridge::externalMonitorEnabled() const
{
    return m_externalMonitorEnabled;
}

void SettingsBridge::setExternalMonitorEnabled(bool value)
{
    if (m_externalMonitorEnabled == value)
        return;

    m_externalMonitorEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("externalMonitorEnabled", value);
    emit externalMonitorEnabledChanged();
}

QString SettingsBridge::externalMonitorMode() const
{
    return m_externalMonitorMode;
}

void SettingsBridge::setExternalMonitorMode(const QString &value)
{
    const QString normalized = (value == QStringLiteral("Overlay Feed")
                                || value == QStringLiteral("Assist Feed"))
        ? QStringLiteral("Assist Feed")
        : QStringLiteral("Clean Feed");

    if (m_externalMonitorMode == normalized)
        return;

    m_externalMonitorMode = normalized;
    if (!m_externalSettingsSyncing) m_settings.setValue("externalMonitorMode", normalized);
    emit externalMonitorModeChanged();
}

QString SettingsBridge::externalMonitorInfoOverlay() const
{
    return m_externalMonitorInfoOverlay;
}

void SettingsBridge::setExternalMonitorInfoOverlay(const QString &value)
{
    const QString normalized = value == QStringLiteral("Off")
        ? QStringLiteral("Off")
        : QStringLiteral("On");

    if (m_externalMonitorInfoOverlay == normalized)
        return;

    m_externalMonitorInfoOverlay = normalized;
    if (!m_externalSettingsSyncing) m_settings.setValue("externalMonitorInfoOverlay", normalized);
    emit externalMonitorInfoOverlayChanged();
}

QString SettingsBridge::externalMonitorOrientation() const
{
    return m_externalMonitorOrientation;
}

void SettingsBridge::setExternalMonitorOrientation(const QString &value)
{
    QString normalized = QStringLiteral("Landscape");
    if (value == QStringLiteral("Upside Down")) {
        normalized = value;
    }

    if (m_externalMonitorOrientation == normalized)
        return;

    m_externalMonitorOrientation = normalized;
    if (!m_externalSettingsSyncing) m_settings.setValue("externalMonitorOrientation", normalized);
    emit externalMonitorOrientationChanged();
}

bool SettingsBridge::dateTimeOverlayEnabled() const
{
    return m_dateTimeOverlayEnabled;
}

void SettingsBridge::setDateTimeOverlayEnabled(bool value)
{
    if (m_dateTimeOverlayEnabled == value)
        return;

    m_dateTimeOverlayEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("dateTimeOverlayEnabled", value);
    m_settings.sync();
    updateSettingsFileState();
    emit dateTimeOverlayEnabledChanged();
}

QString SettingsBridge::timecodeMode() const
{
    return m_timecodeMode;
}

void SettingsBridge::setTimecodeMode(const QString &value)
{
    if (m_timecodeMode == value)
        return;

    m_timecodeMode = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("timecodeMode", value);
    m_settings.sync();
    updateSettingsFileState();
    emit timecodeModeChanged();
}

bool SettingsBridge::photoModeEnabled() const
{
    return m_photoModeEnabled;
}

void SettingsBridge::setPhotoModeEnabled(bool value)
{
    if (m_photoModeEnabled == value)
        return;

    m_photoModeEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("photoModeEnabled", value);
    emit photoModeEnabledChanged();
}

QString SettingsBridge::photoTimer() const
{
    return m_photoTimer;
}

bool SettingsBridge::photoLongExposureEnabled() const
{
    return m_photoLongExposureEnabled;
}

void SettingsBridge::setPhotoLongExposureEnabled(bool value)
{
    if (m_photoLongExposureEnabled == value)
        return;
    m_photoLongExposureEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("photoLongExposureEnabled", value);
    emit photoLongExposureEnabledChanged();
}

bool SettingsBridge::photoBracketingEnabled() const { return m_photoBracketingEnabled; }

void SettingsBridge::setPhotoBracketingEnabled(bool value)
{
    if (m_photoBracketingEnabled == value) return;
    m_photoBracketingEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("photoBracketingEnabled", value);
    emit photoBracketingEnabledChanged();
}

void SettingsBridge::setPhotoTimer(const QString &value)
{
    if (m_photoTimer == value)
        return;

    m_photoTimer = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("photoTimer", value);
    emit photoTimerChanged();
}

void SettingsBridge::setPhotoBracketingEv(double value)
{
    const double next = std::isfinite(value) ? std::round(std::clamp(value, 1.0 / 3.0, 3.0) * 3.0) / 3.0 : 1.0;
    if (qAbs(m_photoBracketingEv - next) < 0.0001) return;
    m_photoBracketingEv = next;
    if (!m_externalSettingsSyncing) m_settings.setValue("photoBracketingEv", next);
    emit photoBracketingEvChanged();
}

QString SettingsBridge::photoBurst() const
{
    return m_photoBurst;
}

void SettingsBridge::setPhotoBurst(const QString &value)
{
    if (m_photoBurst == value)
        return;

    m_photoBurst = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("photoBurst", value);
    emit photoBurstChanged();
}

QString SettingsBridge::photoFormat() const
{
    return m_photoFormat;
}

void SettingsBridge::setPhotoFormat(const QString &value)
{
    const QString normalized = normalizePhotoFormat(value);
    if (m_photoFormat == normalized)
        return;

    m_photoFormat = normalized;
    if (!m_externalSettingsSyncing) m_settings.setValue("photoFormat", normalized);
    emit photoFormatChanged();
}

QString SettingsBridge::photoTimelapse() const
{
    return m_photoTimelapse;
}

void SettingsBridge::setPhotoTimelapse(const QString &value)
{
    const QString normalized = normalizePhotoTimelapse(value);
    if (m_photoTimelapse == normalized)
        return;

    m_photoTimelapse = normalized;
    if (!m_externalSettingsSyncing) m_settings.setValue("photoTimelapse", normalized);
    emit photoTimelapseChanged();
}

bool SettingsBridge::recordAudioEnabled() const
{
    return m_recordAudioEnabled;
}

bool SettingsBridge::soundRecorderEnabled() const
{
    return m_soundRecorderEnabled;
}

void SettingsBridge::setSoundRecorderEnabled(bool value)
{
    if (m_soundRecorderEnabled == value)
        return;

    m_soundRecorderEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("soundRecorderEnabled", value);
    m_settings.sync();
    emit soundRecorderEnabledChanged();
}

void SettingsBridge::setRecordAudioEnabled(bool value)
{
    if (m_recordAudioEnabled == value)
        return;

    m_recordAudioEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("recordAudioEnabled", value);
    m_settings.sync();
    emit recordAudioEnabledChanged();
}

bool SettingsBridge::recordGyroEnabled() const
{
    return m_recordGyroEnabled;
}

void SettingsBridge::setRecordGyroEnabled(bool value)
{
    if (m_recordGyroEnabled == value)
        return;

    m_recordGyroEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("recordGyroEnabled", value);
    m_settings.sync();
    emit recordGyroEnabledChanged();
}

bool SettingsBridge::liveAudioMonitoringEnabled() const
{
    return m_liveAudioMonitoringEnabled;
}

void SettingsBridge::setLiveAudioMonitoringEnabled(bool value)
{
    if (m_liveAudioMonitoringEnabled == value)
        return;

    m_liveAudioMonitoringEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("liveAudioMonitoringEnabled", value);
    m_settings.sync();
    emit liveAudioMonitoringEnabledChanged();
}

bool SettingsBridge::audioMeterEnabled() const
{
    return m_audioMeterEnabled;
}

void SettingsBridge::setAudioMeterEnabled(bool value)
{
    if (m_audioMeterEnabled == value)
        return;

    m_audioMeterEnabled = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("audioMeterEnabled", value);
    m_settings.sync();
    emit audioMeterEnabledChanged();
}

QString SettingsBridge::audioInputDevice() const
{
    return m_audioInputDevice;
}

void SettingsBridge::setAudioInputDevice(const QString &value)
{
    const QString normalized = value.trimmed().compare(QStringLiteral("Auto"), Qt::CaseInsensitive) == 0
        ? QString()
        : value.trimmed();

    if (m_audioInputDevice == normalized)
        return;

    m_audioInputDevice = normalized;
    if (!m_externalSettingsSyncing) m_settings.setValue("audioInputDevice", normalized);
    m_settings.sync();
    emit audioInputDeviceChanged();
}

QString SettingsBridge::audioOutputDevice() const
{
    return m_audioOutputDevice;
}

void SettingsBridge::setAudioOutputDevice(const QString &value)
{
    const QString normalized = value.trimmed().compare(QStringLiteral("Auto"), Qt::CaseInsensitive) == 0
        ? QString()
        : value.trimmed();

    if (m_audioOutputDevice == normalized)
        return;

    m_audioOutputDevice = normalized;
    if (!m_externalSettingsSyncing) m_settings.setValue("audioOutputDevice", normalized);
    m_settings.sync();
    emit audioOutputDeviceChanged();
}

void SettingsBridge::setAudioSampleFormat(const QString &value)
{
    if (!QStringList{"auto", "s16", "s24", "s32", "float32"}.contains(value) || m_audioSampleFormat == value) return;
    m_audioSampleFormat = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("audioSampleFormat", value);
    m_settings.sync();
    emit audioCaptureOptionsChanged();
}

void SettingsBridge::setAudioSampleRate(int value)
{
    if (value < 0 || value > 192000 || value == m_audioSampleRate) return;
    m_audioSampleRate = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("audioSampleRate", value);
    m_settings.sync();
    emit audioCaptureOptionsChanged();
}

void SettingsBridge::setAudioChannels(const QString &value)
{
    if (!QRegularExpression(QStringLiteral("^(auto|all|[1-9]|[12][0-9]|3[0-2]|([1-9]|[12][0-9]|3[0-2])-([1-9]|[12][0-9]|3[0-2]))$")).match(value).hasMatch() || value == m_audioChannels) return;
    if (value.contains('-') && value.section('-', 0, 0).toInt() > value.section('-', 1, 1).toInt()) return;
    m_audioChannels = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("audioChannels", value);
    m_settings.sync();
    emit audioCaptureOptionsChanged();
}

int SettingsBridge::inputVolume() const
{
    return m_inputVolume;
}

void SettingsBridge::setInputVolume(int value)
{
    const int clamped = qBound(0, value, 100);
    if (m_inputVolume == clamped)
        return;

    m_inputVolume = clamped;
    if (!m_externalSettingsSyncing) m_settings.setValue("inputVolume", clamped);
    m_settings.sync();
    emit inputVolumeChanged();
}

int SettingsBridge::headphoneVolume() const
{
    return m_headphoneVolume;
}

void SettingsBridge::setHeadphoneVolume(int value)
{
    const int clamped = qBound(0, value, 100);
    if (m_headphoneVolume == clamped)
        return;

    m_headphoneVolume = clamped;
    if (!m_externalSettingsSyncing) m_settings.setValue("headphoneVolume", clamped);
    m_settings.sync();
    emit headphoneVolumeChanged();
}

QString SettingsBridge::batteryCapacity() const
{
    return m_batteryCapacity;
}

QString SettingsBridge::batteryType() const
{
    return m_batteryType;
}

void SettingsBridge::setBatteryType(const QString &value)
{
    const QString normalized = normalizeBatteryType(value);
    if (m_batteryType == normalized)
        return;

    m_batteryType = normalized;
    if (!m_externalSettingsSyncing) m_settings.setValue("batteryType", normalized);
    emit batteryTypeChanged();
}

void SettingsBridge::setBatteryCapacity(const QString &value)
{
    if (m_batteryCapacity == value)
        return;

    m_batteryCapacity = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("batteryCapacity", value);
    emit batteryCapacityChanged();
}

int SettingsBridge::customBatteryWh() const
{
    return m_customBatteryWh;
}

void SettingsBridge::setCustomBatteryWh(int value)
{
    const int clamped = qBound(10, value, 500);
    if (m_customBatteryWh == clamped)
        return;

    m_customBatteryWh = clamped;
    if (!m_externalSettingsSyncing) m_settings.setValue("customBatteryWh", clamped);
    emit customBatteryWhChanged();
}

double SettingsBridge::customBatteryFullVoltage() const
{
    return m_customBatteryFullVoltage;
}

void SettingsBridge::setCustomBatteryFullVoltage(double value)
{
    const double clamped = qBound(1.0, value, 60.0);
    if (qFuzzyCompare(m_customBatteryFullVoltage, clamped))
        return;

    m_customBatteryFullVoltage = clamped;
    if (m_customBatteryFullVoltage <= m_customBatteryEmptyVoltage)
        m_customBatteryEmptyVoltage = qMax(0.5, m_customBatteryFullVoltage - 0.1);
    if (!m_externalSettingsSyncing) m_settings.setValue("customBatteryFullVoltage", m_customBatteryFullVoltage);
    if (!m_externalSettingsSyncing) m_settings.setValue("customBatteryEmptyVoltage", m_customBatteryEmptyVoltage);
    emit customBatteryFullVoltageChanged();
    emit customBatteryEmptyVoltageChanged();
}

double SettingsBridge::customBatteryEmptyVoltage() const
{
    return m_customBatteryEmptyVoltage;
}

void SettingsBridge::setCustomBatteryEmptyVoltage(double value)
{
    const double clamped = qBound(0.5, value, 59.0);
    const double normalized = qMin(clamped, m_customBatteryFullVoltage - 0.1);
    if (qFuzzyCompare(m_customBatteryEmptyVoltage, normalized))
        return;

    m_customBatteryEmptyVoltage = normalized;
    if (!m_externalSettingsSyncing) m_settings.setValue("customBatteryEmptyVoltage", normalized);
    emit customBatteryEmptyVoltageChanged();
}

QString SettingsBridge::sleepMode() const
{
    return m_sleepMode;
}

void SettingsBridge::setStandbyTimeoutSeconds(int value)
{
    value = qBound(0, value, 86400);
    if (m_standbyTimeoutSeconds == value)
        return;
    m_standbyTimeoutSeconds = value;
    if (!m_externalSettingsSyncing) m_settings.setValue(QStringLiteral("standbyTimeoutSeconds"), value);
    m_settings.sync();
    emit standbyTimeoutSecondsChanged();
}

void SettingsBridge::setStandbyUsbPowerOff(bool value)
{
    if (m_standbyUsbPowerOff == value)
        return;
    m_standbyUsbPowerOff = value;
    if (!m_externalSettingsSyncing) m_settings.setValue(QStringLiteral("standbyUsbPowerOff"), value);
    m_settings.sync();
    emit standbyUsbPowerOffChanged();
}

void SettingsBridge::setSleepMode(const QString &value)
{
    if (m_sleepMode == value)
        return;

    m_sleepMode = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("sleepMode", value);
    emit sleepModeChanged();
}

QStringList SettingsBridge::monitoringOrder() const
{
    return m_monitoringOrder;
}

void SettingsBridge::setMonitoringOrder(const QStringList &value)
{
    if (m_monitoringOrder == value)
        return;

    m_monitoringOrder = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("monitoringOrder", value);
    emit monitoringOrderChanged();
}

QStringList SettingsBridge::recordOrder() const
{
    return m_recordOrder;
}

void SettingsBridge::setRecordOrder(const QStringList &value)
{
    if (m_recordOrder == value)
        return;

    m_recordOrder = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("recordOrder", value);
    emit recordOrderChanged();
}

QStringList SettingsBridge::audioOrder() const
{
    return m_audioOrder;
}

void SettingsBridge::setAudioOrder(const QStringList &value)
{
    if (m_audioOrder == value)
        return;

    m_audioOrder = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("audioOrder", value);
    emit audioOrderChanged();
}

QStringList SettingsBridge::powerOrder() const
{
    return m_powerOrder;
}

void SettingsBridge::setPowerOrder(const QStringList &value)
{
    if (m_powerOrder == value)
        return;

    m_powerOrder = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("powerOrder", value);
    emit powerOrderChanged();
}

QStringList SettingsBridge::systemOrder() const
{
    return m_systemOrder;
}

void SettingsBridge::setSystemOrder(const QStringList &value)
{
    if (m_systemOrder == value)
        return;

    m_systemOrder = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("systemOrder", value);
    emit systemOrderChanged();
}

QStringList SettingsBridge::wifiOrder() const
{
    return m_wifiOrder;
}

void SettingsBridge::setWifiOrder(const QStringList &value)
{
    if (m_wifiOrder == value)
        return;

    m_wifiOrder = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("wifiOrder", value);
    emit wifiOrderChanged();
}

QStringList SettingsBridge::infoOrder() const
{
    return m_infoOrder;
}

void SettingsBridge::setInfoOrder(const QStringList &value)
{
    if (m_infoOrder == value)
        return;

    m_infoOrder = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("infoOrder", value);
    emit infoOrderChanged();
}

QStringList SettingsBridge::sidebarOrder() const
{
    return m_sidebarOrder;
}

void SettingsBridge::setSidebarOrder(const QStringList &value)
{
    if (m_sidebarOrder == value)
        return;

    m_sidebarOrder = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("sidebarOrder", value);
    emit sidebarOrderChanged();
}

QStringList SettingsBridge::favoritesOrder() const
{
    return m_favoritesOrder;
}

void SettingsBridge::setFavoritesOrder(const QStringList &value)
{
    if (m_favoritesOrder == value)
        return;

    m_favoritesOrder = value;
    if (!m_externalSettingsSyncing) m_settings.setValue("favoritesOrder", value);
    emit favoritesOrderChanged();
}

QStringList SettingsBridge::presetNames() const
{
    return m_presetNames;
}

QString SettingsBridge::presetStatusText() const
{
    return m_presetStatusText;
}

bool SettingsBridge::presetStatusError() const
{
    return m_presetStatusError;
}

bool SettingsBridge::saveCurrentAsPreset(const QString &name)
{
    if (m_controlBridge && !m_controlBridge->connected()) {
        setPresetStatus(QStringLiteral("Connect to the camera before saving a preset."), true);
        return false;
    }
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        setPresetStatus(QStringLiteral("Preset name cannot be empty."), true);
        return false;
    }

    QString errorMessage;
    if (!writePresetFile(presetFilePathForName(trimmedName),
                         trimmedName,
                         serializeCurrentSettings(),
                         &errorMessage)) {
        setPresetStatus(errorMessage, true);
        return false;
    }

    reloadPresetNames();
    clearPresetStatus();
    return true;
}

bool SettingsBridge::loadPreset(const QString &name)
{
    QString presetName;
    QJsonObject settingsObject;
    QString errorMessage;
    if (!readPresetFile(presetFilePathForName(name), &presetName, &settingsObject, &errorMessage)) {
        setPresetStatus(errorMessage, true);
        return false;
    }

    if (!applySettingsObject(settingsObject, &errorMessage)) {
        setPresetStatus(errorMessage, true);
        return false;
    }

    m_settings.sync();
    if (m_settings.status() != QSettings::NoError) {
        setPresetStatus(QStringLiteral("Preset applied, but its settings could not be saved to disk."), true);
        return false;
    }
    setPresetStatus(QStringLiteral("Loaded preset \"%1\".").arg(presetName), false);
    return true;
}

bool SettingsBridge::deletePreset(const QString &name)
{
    const QString filePath = presetFilePathForName(name);
    QFile file(filePath);
    if (!file.exists()) {
        setPresetStatus(QStringLiteral("Preset \"%1\" was not found.").arg(name), true);
        return false;
    }

    if (!file.remove()) {
        setPresetStatus(QStringLiteral("Preset \"%1\" could not be deleted.").arg(name), true);
        return false;
    }

    reloadPresetNames();
    clearPresetStatus();
    return true;
}

bool SettingsBridge::exportPresetToPath(const QString &name, const QString &mediaMountPath)
{
    const QString trimmedMountPath = mediaMountPath.trimmed();
    if (trimmedMountPath.isEmpty()) {
        setPresetStatus(QStringLiteral("No media path is available for preset export."), true);
        return false;
    }

    const QDir mediaDir(trimmedMountPath);
    if (!mediaDir.exists()) {
        setPresetStatus(QStringLiteral("The media path \"%1\" is not available.").arg(trimmedMountPath), true);
        return false;
    }

    const QString exportDirPath = mediaDir.filePath(QStringLiteral("Apertar Presets"));
    if (!QDir().mkpath(exportDirPath)) {
        setPresetStatus(QStringLiteral("Could not create the preset export folder on media."), true);
        return false;
    }

    QString presetName;
    QJsonObject settingsObject;
    QString errorMessage;
    if (!readPresetFile(presetFilePathForName(name), &presetName, &settingsObject, &errorMessage)) {
        setPresetStatus(errorMessage, true);
        return false;
    }

    const QString exportPath = QDir(exportDirPath).filePath(
        sanitizedPresetFileStem(presetName) + QStringLiteral(".apertar-preset.json"));
    if (!writePresetFile(exportPath, presetName, settingsObject, &errorMessage)) {
        setPresetStatus(errorMessage, true);
        return false;
    }

    setPresetStatus(QStringLiteral("Exported \"%1\" to media.").arg(presetName), false);
    return true;
}

bool SettingsBridge::importPresetsFromPath(const QString &mediaMountPath)
{
    const QString trimmedMountPath = mediaMountPath.trimmed();
    if (trimmedMountPath.isEmpty()) {
        setPresetStatus(QStringLiteral("No media path is available for preset import."), true);
        return false;
    }

    const QDir mediaDir(trimmedMountPath);
    if (!mediaDir.exists()) {
        setPresetStatus(QStringLiteral("The media path \"%1\" is not available.").arg(trimmedMountPath), true);
        return false;
    }

    QStringList candidateFiles;
    const QStringList filters = {QStringLiteral("*.apertar-preset.json")};
    for (const QFileInfo &info : mediaDir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name))
        candidateFiles.push_back(info.absoluteFilePath());

    const QDir exportDir(mediaDir.filePath(QStringLiteral("Apertar Presets")));
    if (exportDir.exists()) {
        for (const QFileInfo &info : exportDir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name))
            candidateFiles.push_back(info.absoluteFilePath());
    }

    candidateFiles.removeDuplicates();
    if (candidateFiles.isEmpty()) {
        setPresetStatus(QStringLiteral("No preset files were found on the media drive."), true);
        return false;
    }

    int importedCount = 0;
    int skippedDuplicateCount = 0;
    QStringList importedNames;
    QStringList duplicateNames;
    const auto batchNameExists = [this, &importedNames](const QString &name) {
        for (const QString &existing : m_presetNames) {
            if (existing.compare(name, Qt::CaseInsensitive) == 0)
                return true;
        }
        for (const QString &imported : importedNames) {
            if (imported.compare(name, Qt::CaseInsensitive) == 0)
                return true;
        }
        return false;
    };
    for (const QString &filePath : candidateFiles) {
        QString presetName;
        QJsonObject settingsObject;
        QString errorMessage;
        if (!readPresetFile(filePath, &presetName, &settingsObject, &errorMessage))
            continue;

        QString finalName = presetName.trimmed();
        if (finalName.isEmpty())
            finalName = QStringLiteral("Imported Preset");
        if (batchNameExists(finalName)) {
            duplicateNames.push_back(finalName);
            ++skippedDuplicateCount;
            continue;
        }
        if (!writePresetFile(presetFilePathForName(finalName), finalName, settingsObject, &errorMessage))
            continue;

        importedNames.push_back(finalName);
        ++importedCount;
    }

    reloadPresetNames();

    if (importedCount == 0) {
        if (skippedDuplicateCount > 0) {
            if (skippedDuplicateCount == 1) {
                setPresetStatus(QStringLiteral("Preset \"%1\" is already in the library.")
                                    .arg(duplicateNames.constFirst()),
                                false);
            } else {
                setPresetStatus(QStringLiteral("All %1 presets already exist in the library.")
                                    .arg(skippedDuplicateCount),
                                false);
            }
            return true;
        }

        setPresetStatus(QStringLiteral("No valid preset files could be imported."), true);
        return false;
    }

    if (skippedDuplicateCount > 0) {
        setPresetStatus(QStringLiteral("Imported %1 preset%2. %3 already exist%4 in the library.")
                            .arg(importedCount)
                            .arg(importedCount == 1 ? QString() : QStringLiteral("s"))
                            .arg(skippedDuplicateCount)
                            .arg(skippedDuplicateCount == 1 ? QStringLiteral("s") : QString()),
                        false);
    } else {
        setPresetStatus(
            importedCount == 1
                ? QStringLiteral("Imported preset \"%1\".").arg(importedNames.constFirst())
                : QStringLiteral("Imported %1 presets from media.").arg(importedCount),
            false);
    }
    return true;
}

void SettingsBridge::clearPresetStatus()
{
    setPresetStatus(QString(), false);
}

QString SettingsBridge::presetsDirectoryPath() const
{
    const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString appDataPresetsPath = appDataPath.isEmpty()
        ? QString()
        : QDir(appDataPath).filePath(QStringLiteral("presets"));
    const QString legacyPresetsPath = QDir(QDir::homePath() + QStringLiteral("/.config/CinePi/UI"))
        .filePath(QStringLiteral("presets"));

    const QDir appDataDir(appDataPresetsPath);
    if (!appDataPresetsPath.isEmpty() && appDataDir.exists())
        return appDataPresetsPath;

    const QDir legacyDir(legacyPresetsPath);
    if (legacyDir.exists())
        return legacyPresetsPath;

    if (!appDataPresetsPath.isEmpty())
        return appDataPresetsPath;

    return legacyPresetsPath;
}

QStringList SettingsBridge::currentPresetDirectorySignature() const
{
    const QDir dir(presetsDirectoryPath());
    QStringList signature;
    if (!dir.exists())
        return signature;

    const QFileInfoList files = dir.entryInfoList(
        {QStringLiteral("*.apertar-preset.json")},
        QDir::Files | QDir::Readable,
        QDir::Name | QDir::IgnoreCase);

    signature.reserve(files.size());
    for (const QFileInfo &info : files) {
        signature.push_back(
            info.fileName()
            + QLatin1Char('|')
            + QString::number(info.lastModified().toMSecsSinceEpoch())
            + QLatin1Char('|')
            + QString::number(info.size()));
    }
    return signature;
}

QString SettingsBridge::sanitizedPresetFileStem(const QString &name) const
{
    QString stem = name.simplified();
    if (stem.isEmpty())
        stem = QStringLiteral("Preset");

    static const QString invalidCharacters = QStringLiteral("\\/:*?\"<>|");
    for (const QChar invalid : invalidCharacters)
        stem.replace(invalid, QLatin1Char('_'));

    stem.replace(QLatin1Char('\n'), QLatin1Char('_'));
    stem.replace(QLatin1Char('\r'), QLatin1Char('_'));
    stem.replace(QStringLiteral("  "), QStringLiteral(" "));
    return stem;
}

QString SettingsBridge::presetFilePathForName(const QString &name) const
{
    return QDir(presetsDirectoryPath()).filePath(
        sanitizedPresetFileStem(name) + QStringLiteral(".apertar-preset.json"));
}

QString SettingsBridge::uniquePresetName(const QString &baseName) const
{
    QString candidate = baseName.trimmed();
    if (candidate.isEmpty())
        candidate = QStringLiteral("Imported Preset");

    const auto nameExists = [this](const QString &name) {
        for (const QString &existing : m_presetNames) {
            if (existing.compare(name, Qt::CaseInsensitive) == 0)
                return true;
        }
        return false;
    };

    if (!nameExists(candidate))
        return candidate;

    const QString seed = candidate;
    int suffix = 2;
    while (nameExists(QStringLiteral("%1 (%2)").arg(seed).arg(suffix)))
        ++suffix;
    return QStringLiteral("%1 (%2)").arg(seed).arg(suffix);
}

QJsonObject SettingsBridge::serializeCurrentSettings() const
{
    QJsonObject settingsObject;
    settingsObject.insert(QStringLiteral("zebraEnabled"), m_zebraEnabled);
    settingsObject.insert(QStringLiteral("zebraThreshold"), m_zebraThreshold);
    settingsObject.insert(QStringLiteral("focusPeakingEnabled"), m_focusPeakingEnabled);
    settingsObject.insert(QStringLiteral("focusPeakingThreshold"), m_focusPeakingThreshold);
    settingsObject.insert(QStringLiteral("focusPeakingColor"), m_focusPeakingColor);
    settingsObject.insert(QStringLiteral("grayscaleEnabled"), m_grayscaleEnabled);
    settingsObject.insert(QStringLiteral("smpteEnabled"), m_smpteEnabled);
    settingsObject.insert(QStringLiteral("anamorphicDesqueezeEnabled"), m_anamorphicDesqueezeEnabled);
    settingsObject.insert(QStringLiteral("anamorphicRatio"), m_anamorphicRatio);
    settingsObject.insert(QStringLiteral("falseColorEnabled"), m_falseColorEnabled);
    settingsObject.insert(QStringLiteral("falseColorMode"), m_falseColorMode);
    settingsObject.insert(QStringLiteral("lutPreviewEnabled"), m_lutPreviewEnabled);
    settingsObject.insert(QStringLiteral("lutPreviewFile"), m_lutPreviewFile);
    settingsObject.insert(QStringLiteral("horizontalLevelEnabled"), m_horizontalLevelEnabled);
    settingsObject.insert(QStringLiteral("horizontalLevelColor"), m_horizontalLevelColor);
    settingsObject.insert(QStringLiteral("guidesEnabled"), m_guidesEnabled);
    settingsObject.insert(QStringLiteral("guidesType"), m_guidesType);
    settingsObject.insert(QStringLiteral("guidesThickness"), m_guidesThickness);
    settingsObject.insert(QStringLiteral("centerMarkerEnabled"), m_centerMarkerEnabled);
    settingsObject.insert(QStringLiteral("centerMarkerType"), m_centerMarkerType);
    settingsObject.insert(QStringLiteral("uiOrientation"), m_uiOrientation);
    settingsObject.insert(QStringLiteral("cameraControlsOpacity"), m_cameraControlsOpacity);
    settingsObject.insert(QStringLiteral("cameraControlsMode"), m_cameraControlsMode);
    settingsObject.insert(QStringLiteral("externalMonitorEnabled"), m_externalMonitorEnabled);
    settingsObject.insert(QStringLiteral("externalMonitorMode"), m_externalMonitorMode);
    settingsObject.insert(QStringLiteral("externalMonitorInfoOverlay"), m_externalMonitorInfoOverlay);
    settingsObject.insert(QStringLiteral("externalMonitorOrientation"), m_externalMonitorOrientation);
    settingsObject.insert(QStringLiteral("dateTimeOverlayEnabled"), m_dateTimeOverlayEnabled);
    settingsObject.insert(QStringLiteral("timecodeMode"), m_timecodeMode);
    settingsObject.insert(QStringLiteral("photoModeEnabled"), m_photoModeEnabled);
    settingsObject.insert(QStringLiteral("photoLongExposureEnabled"), m_photoLongExposureEnabled);
    settingsObject.insert(QStringLiteral("photoBracketingEnabled"), m_photoBracketingEnabled);
    settingsObject.insert(QStringLiteral("photoBracketingEv"), m_photoBracketingEv);
    settingsObject.insert(QStringLiteral("photoTimer"), m_photoTimer);
    settingsObject.insert(QStringLiteral("photoBurst"), m_photoBurst);
    settingsObject.insert(QStringLiteral("photoFormat"), m_photoFormat);
    settingsObject.insert(QStringLiteral("photoTimelapse"), m_photoTimelapse);
    settingsObject.insert(QStringLiteral("soundRecorderEnabled"), m_soundRecorderEnabled);
    settingsObject.insert(QStringLiteral("recordAudioEnabled"), m_recordAudioEnabled);
    settingsObject.insert(QStringLiteral("recordGyroEnabled"), m_recordGyroEnabled);
    settingsObject.insert(QStringLiteral("liveAudioMonitoringEnabled"), m_liveAudioMonitoringEnabled);
    settingsObject.insert(QStringLiteral("audioMeterEnabled"), m_audioMeterEnabled);
    settingsObject.insert(QStringLiteral("audioInputDevice"), m_audioInputDevice);
    settingsObject.insert(QStringLiteral("audioOutputDevice"), m_audioOutputDevice);
    settingsObject.insert(QStringLiteral("audioSampleFormat"), m_audioSampleFormat);
    settingsObject.insert(QStringLiteral("audioSampleRate"), m_audioSampleRate);
    settingsObject.insert(QStringLiteral("audioChannels"), m_audioChannels);
    settingsObject.insert(QStringLiteral("inputVolume"), m_inputVolume);
    settingsObject.insert(QStringLiteral("headphoneVolume"), m_headphoneVolume);
    settingsObject.insert(QStringLiteral("batteryType"), m_batteryType);
    settingsObject.insert(QStringLiteral("batteryCapacity"), m_batteryCapacity);
    settingsObject.insert(QStringLiteral("customBatteryWh"), m_customBatteryWh);
    settingsObject.insert(QStringLiteral("customBatteryFullVoltage"), m_customBatteryFullVoltage);
    settingsObject.insert(QStringLiteral("customBatteryEmptyVoltage"), m_customBatteryEmptyVoltage);
    settingsObject.insert(QStringLiteral("sleepMode"), m_sleepMode);
    settingsObject.insert(QStringLiteral("standbyTimeoutSeconds"), m_standbyTimeoutSeconds);
    settingsObject.insert(QStringLiteral("standbyUsbPowerOff"), m_standbyUsbPowerOff);
    if (m_controlBridge) {
        const auto capture = m_controlBridge->cameraPresetSettings();
        for (auto it = capture.begin(); it != capture.end(); ++it)
            settingsObject.insert(it.key(), it.value());
        settingsObject.insert(QStringLiteral("cameraShutterMode"),
                              m_photoModeEnabled ? QStringLiteral("speed") : QStringLiteral("angle"));
    }
    settingsObject.insert(QStringLiteral("monitoringOrder"), QJsonArray::fromStringList(m_monitoringOrder));
    settingsObject.insert(QStringLiteral("recordOrder"), QJsonArray::fromStringList(m_recordOrder));
    settingsObject.insert(QStringLiteral("audioOrder"), QJsonArray::fromStringList(m_audioOrder));
    settingsObject.insert(QStringLiteral("powerOrder"), QJsonArray::fromStringList(m_powerOrder));
    settingsObject.insert(QStringLiteral("systemOrder"), QJsonArray::fromStringList(m_systemOrder));
    settingsObject.insert(QStringLiteral("wifiOrder"), QJsonArray::fromStringList(m_wifiOrder));
    settingsObject.insert(QStringLiteral("infoOrder"), QJsonArray::fromStringList(m_infoOrder));
    settingsObject.insert(QStringLiteral("sidebarOrder"), QJsonArray::fromStringList(m_sidebarOrder));
    settingsObject.insert(QStringLiteral("favoritesOrder"), QJsonArray::fromStringList(m_favoritesOrder));
    return settingsObject;
}

bool SettingsBridge::applySettingsObject(const QJsonObject &settingsObject, QString *errorMessage)
{
    if (settingsObject.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("The preset file did not contain any settings.");
        return false;
    }

    if (m_controlBridge && !m_controlBridge->applyCameraPreset(settingsObject)) {
        if (errorMessage) *errorMessage = m_controlBridge->lastError();
        return false;
    }
    setZebraEnabled(boolValue(settingsObject, "zebraEnabled", m_zebraEnabled));
    setZebraThreshold(static_cast<float>(doubleValue(settingsObject, "zebraThreshold", m_zebraThreshold)));
    setFocusPeakingEnabled(boolValue(settingsObject, "focusPeakingEnabled", m_focusPeakingEnabled));
    setFocusPeakingThreshold(static_cast<float>(doubleValue(settingsObject, "focusPeakingThreshold", m_focusPeakingThreshold)));
    setFocusPeakingColor(stringValue(settingsObject, "focusPeakingColor", m_focusPeakingColor));
    setGrayscaleEnabled(boolValue(settingsObject, "grayscaleEnabled", m_grayscaleEnabled));
    setSmpteEnabled(boolValue(settingsObject, "smpteEnabled", m_smpteEnabled));
    setAnamorphicDesqueezeEnabled(boolValue(settingsObject, "anamorphicDesqueezeEnabled", m_anamorphicDesqueezeEnabled));
    setAnamorphicRatio(stringValue(settingsObject, "anamorphicRatio", m_anamorphicRatio));
    setFalseColorEnabled(boolValue(settingsObject, "falseColorEnabled", m_falseColorEnabled));
    setFalseColorMode(intValue(settingsObject, "falseColorMode", m_falseColorMode));
    setLutPreviewEnabled(boolValue(settingsObject, "lutPreviewEnabled", m_lutPreviewEnabled));
    setLutPreviewFile(stringValue(settingsObject, "lutPreviewFile", m_lutPreviewFile));
    setHorizontalLevelEnabled(boolValue(settingsObject, "horizontalLevelEnabled", m_horizontalLevelEnabled));
    setHorizontalLevelColor(stringValue(settingsObject, "horizontalLevelColor", m_horizontalLevelColor));
    setGuidesEnabled(boolValue(settingsObject, "guidesEnabled", m_guidesEnabled));
    setGuidesType(stringValue(settingsObject, "guidesType", m_guidesType));
    setGuidesThickness(intValue(settingsObject, "guidesThickness", m_guidesThickness));
    setCenterMarkerEnabled(boolValue(settingsObject, "centerMarkerEnabled", m_centerMarkerEnabled));
    setCenterMarkerType(stringValue(settingsObject, "centerMarkerType", m_centerMarkerType));
    setUiOrientation(stringValue(settingsObject, "uiOrientation", m_uiOrientation));
    setCameraControlsOpacity(doubleValue(settingsObject, "cameraControlsOpacity", m_cameraControlsOpacity));
    setCameraControlsMode(stringValue(settingsObject, "cameraControlsMode", m_cameraControlsMode));
    setExternalMonitorEnabled(boolValue(settingsObject, "externalMonitorEnabled", m_externalMonitorEnabled));
    setExternalMonitorMode(stringValue(settingsObject, "externalMonitorMode", m_externalMonitorMode));
    setExternalMonitorInfoOverlay(stringValue(settingsObject, "externalMonitorInfoOverlay", m_externalMonitorInfoOverlay));
    setExternalMonitorOrientation(stringValue(settingsObject, "externalMonitorOrientation", m_externalMonitorOrientation));
    setDateTimeOverlayEnabled(boolValue(settingsObject, "dateTimeOverlayEnabled", m_dateTimeOverlayEnabled));
    setTimecodeMode(stringValue(settingsObject, "timecodeMode", m_timecodeMode));
    setPhotoModeEnabled(boolValue(settingsObject, "photoModeEnabled", m_photoModeEnabled));
    setPhotoLongExposureEnabled(boolValue(settingsObject, "photoLongExposureEnabled", m_photoLongExposureEnabled));
    setPhotoBracketingEnabled(boolValue(settingsObject, "photoBracketingEnabled", m_photoBracketingEnabled));
    setPhotoBracketingEv(doubleValue(settingsObject, "photoBracketingEv", m_photoBracketingEv));
    setPhotoTimer(stringValue(settingsObject, "photoTimer", m_photoTimer));
    setPhotoBurst(stringValue(settingsObject, "photoBurst", m_photoBurst));
    setPhotoFormat(stringValue(settingsObject, "photoFormat", m_photoFormat));
    setPhotoTimelapse(stringValue(settingsObject, "photoTimelapse", m_photoTimelapse));
    setSoundRecorderEnabled(boolValue(settingsObject, "soundRecorderEnabled", m_soundRecorderEnabled));
    setRecordAudioEnabled(boolValue(settingsObject, "recordAudioEnabled", m_recordAudioEnabled));
    setRecordGyroEnabled(boolValue(settingsObject, "recordGyroEnabled", m_recordGyroEnabled));
    setLiveAudioMonitoringEnabled(boolValue(settingsObject, "liveAudioMonitoringEnabled", m_liveAudioMonitoringEnabled));
    setAudioMeterEnabled(boolValue(settingsObject, "audioMeterEnabled", m_audioMeterEnabled));
    setAudioInputDevice(stringValue(settingsObject, "audioInputDevice", m_audioInputDevice));
    setAudioSampleFormat(stringValue(settingsObject, "audioSampleFormat", m_audioSampleFormat));
    setAudioSampleRate(intValue(settingsObject, "audioSampleRate", m_audioSampleRate));
    setAudioChannels(stringValue(settingsObject, "audioChannels", m_audioChannels));
    setAudioOutputDevice(stringValue(settingsObject, "audioOutputDevice", m_audioOutputDevice));
    setInputVolume(intValue(settingsObject, "inputVolume", m_inputVolume));
    setHeadphoneVolume(intValue(settingsObject, "headphoneVolume", m_headphoneVolume));
    setBatteryType(stringValue(settingsObject, "batteryType", m_batteryType));
    setBatteryCapacity(stringValue(settingsObject, "batteryCapacity", m_batteryCapacity));
    setCustomBatteryWh(intValue(settingsObject, "customBatteryWh", m_customBatteryWh));
    setCustomBatteryFullVoltage(doubleValue(settingsObject, "customBatteryFullVoltage", m_customBatteryFullVoltage));
    setCustomBatteryEmptyVoltage(doubleValue(settingsObject, "customBatteryEmptyVoltage", m_customBatteryEmptyVoltage));
    setSleepMode(stringValue(settingsObject, "sleepMode", m_sleepMode));
    setStandbyTimeoutSeconds(settingsObject.value(QStringLiteral("standbyTimeoutSeconds")).toInt(m_standbyTimeoutSeconds));
    setStandbyUsbPowerOff(settingsObject.value(QStringLiteral("standbyUsbPowerOff")).toBool(m_standbyUsbPowerOff));
    setMonitoringOrder(stringListValue(settingsObject, "monitoringOrder", m_monitoringOrder));
    setRecordOrder(stringListValue(settingsObject, "recordOrder", m_recordOrder));
    setAudioOrder(stringListValue(settingsObject, "audioOrder", m_audioOrder));
    setPowerOrder(stringListValue(settingsObject, "powerOrder", m_powerOrder));
    setSystemOrder(stringListValue(settingsObject, "systemOrder", m_systemOrder));
    setWifiOrder(stringListValue(settingsObject, "wifiOrder", m_wifiOrder));
    setInfoOrder(stringListValue(settingsObject, "infoOrder", m_infoOrder));
    setSidebarOrder(stringListValue(settingsObject, "sidebarOrder", m_sidebarOrder));
    setFavoritesOrder(stringListValue(settingsObject, "favoritesOrder", m_favoritesOrder));


    return true;
}

bool SettingsBridge::writePresetFile(const QString &filePath,
                                     const QString &presetName,
                                     const QJsonObject &settingsObject,
                                     QString *errorMessage) const
{
    const QFileInfo fileInfo(filePath);
    if (!QDir().mkpath(fileInfo.dir().absolutePath())) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Could not create the preset folder.");
        return false;
    }

    QJsonObject rootObject;
    rootObject.insert(QStringLiteral("type"), QStringLiteral("apertar-preset"));
    rootObject.insert(QStringLiteral("version"), 1);
    rootObject.insert(QStringLiteral("name"), presetName.trimmed());
    rootObject.insert(QStringLiteral("createdAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    rootObject.insert(QStringLiteral("settings"), settingsObject);

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Could not open the preset file for writing.");
        return false;
    }

    const QByteArray payload = QJsonDocument(rootObject).toJson(QJsonDocument::Indented);
    if (file.write(payload) != payload.size()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Could not write the preset file.");
        return false;
    }

    if (!file.commit()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Could not finalize the preset file.");
        return false;
    }

    return true;
}

bool SettingsBridge::readPresetFile(const QString &filePath,
                                    QString *presetName,
                                    QJsonObject *settingsObject,
                                    QString *errorMessage) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Could not open the preset file.");
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (document.isNull() || !document.isObject()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Preset file is invalid JSON (%1).").arg(parseError.errorString());
        return false;
    }

    const QJsonObject rootObject = document.object();
    const QJsonObject payload = rootObject.value(QStringLiteral("settings")).isObject()
        ? rootObject.value(QStringLiteral("settings")).toObject()
        : rootObject;

    const QString resolvedName = stringValue(rootObject, "name", QFileInfo(filePath).completeBaseName());
    if (presetName)
        *presetName = resolvedName.trimmed().isEmpty() ? QStringLiteral("Preset") : resolvedName.trimmed();
    if (settingsObject)
        *settingsObject = payload;
    return true;
}

void SettingsBridge::reloadPresetNames()
{
    const QDir dir(presetsDirectoryPath());
    QStringList names;
    QStringList signature;
    if (dir.exists()) {
        const QFileInfoList files = dir.entryInfoList(
            {QStringLiteral("*.apertar-preset.json")},
            QDir::Files | QDir::Readable,
            QDir::Name | QDir::IgnoreCase);

        signature.reserve(files.size());
        for (const QFileInfo &info : files) {
            signature.push_back(
                info.fileName()
                + QLatin1Char('|')
                + QString::number(info.lastModified().toMSecsSinceEpoch())
                + QLatin1Char('|')
                + QString::number(info.size()));
            QString presetName;
            QJsonObject settingsObject;
            QString errorMessage;
            if (readPresetFile(info.absoluteFilePath(), &presetName, &settingsObject, &errorMessage))
                names.push_back(presetName);
        }
    }

    std::sort(names.begin(), names.end(), [](const QString &lhs, const QString &rhs) {
        return lhs.localeAwareCompare(rhs) < 0;
    });

    if (m_presetNames == names && m_presetDirectorySignature == signature)
        return;

    m_presetNames = names;
    m_presetDirectorySignature = signature;
    emit presetNamesChanged();
}

void SettingsBridge::setPresetStatus(const QString &text, bool error)
{
    if (m_presetStatusText == text && m_presetStatusError == error)
        return;

    m_presetStatusText = text;
    m_presetStatusError = error;
    emit presetStatusChanged();
}
