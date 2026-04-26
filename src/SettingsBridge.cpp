#include "SettingsBridge.hpp"

SettingsBridge::SettingsBridge(QObject *parent)
    : QObject(parent),
      m_settings("CinePi", "UI")
{
    m_zebraEnabled = m_settings.value("zebraEnabled", false).toBool();
    m_zebraThreshold = m_settings.value("zebraThreshold", 0.70).toFloat();

    m_focusPeakingEnabled = m_settings.value("focusPeakingEnabled", false).toBool();
    m_focusPeakingThreshold = m_settings.value("focusPeakingThreshold", 0.04).toFloat();
    m_focusPeakingColor = m_settings.value("focusPeakingColor", "Red").toString();

    m_grayscaleEnabled = m_settings.value("grayscaleEnabled", false).toBool();
    m_anamorphicDesqueezeEnabled = m_settings.value("anamorphicDesqueezeEnabled", false).toBool();
    m_anamorphicRatio = m_settings.value("anamorphicRatio", "1.33x").toString();

    m_falseColorEnabled = m_settings.value("falseColorEnabled", false).toBool();
    m_falseColorMode = m_settings.value("falseColorMode", 0).toInt();

    m_guidesEnabled = m_settings.value("guidesEnabled", true).toBool();
    m_guidesType = m_settings.value("guidesType", "Thirds").toString();

    m_centerMarkerEnabled = m_settings.value("centerMarkerEnabled", true).toBool();
    m_centerMarkerType = m_settings.value("centerMarkerType", "Circle/Dot").toString();
    m_timecodeMode = m_settings.value("timecodeMode", "Free Run").toString();
    m_photoModeEnabled = m_settings.value("photoModeEnabled", false).toBool();
    m_photoTimer = m_settings.value("photoTimer", "Off").toString();
    m_photoBurst = m_settings.value("photoBurst", "Single").toString();
    m_recordAudioEnabled = m_settings.value("recordAudioEnabled", false).toBool();
    m_audioMeterEnabled = m_settings.value("audioMeterEnabled", false).toBool();
    m_inputVolume = m_settings.value("inputVolume", 60).toInt();
    m_headphoneVolume = m_settings.value("headphoneVolume", 55).toInt();
    m_batteryCapacity = m_settings.value("batteryCapacity", "150Wh").toString();
    m_customBatteryWh = m_settings.value("customBatteryWh", 150).toInt();
    m_sleepMode = m_settings.value("sleepMode", "Off").toString();
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
    m_settings.setValue("zebraEnabled", value);
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
    m_settings.setValue("zebraThreshold", value);
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
    m_settings.setValue("focusPeakingEnabled", value);
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
    m_settings.setValue("focusPeakingThreshold", value);
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
    m_settings.setValue("focusPeakingColor", value);
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
    m_settings.setValue("grayscaleEnabled", value);
    emit grayscaleEnabledChanged();
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
    m_settings.setValue("anamorphicDesqueezeEnabled", value);
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
    m_settings.setValue("anamorphicRatio", value);
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
    m_settings.setValue("falseColorEnabled", value);
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
    m_settings.setValue("falseColorMode", value);
    emit falseColorModeChanged();
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
    m_settings.setValue("guidesEnabled", value);
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
    m_settings.setValue("guidesType", value);
    emit guidesTypeChanged();
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
    m_settings.setValue("centerMarkerEnabled", value);
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
    m_settings.setValue("centerMarkerType", value);
    emit centerMarkerTypeChanged();
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
    m_settings.setValue("timecodeMode", value);
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
    m_settings.setValue("photoModeEnabled", value);
    emit photoModeEnabledChanged();
}

QString SettingsBridge::photoTimer() const
{
    return m_photoTimer;
}

void SettingsBridge::setPhotoTimer(const QString &value)
{
    if (m_photoTimer == value)
        return;

    m_photoTimer = value;
    m_settings.setValue("photoTimer", value);
    emit photoTimerChanged();
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
    m_settings.setValue("photoBurst", value);
    emit photoBurstChanged();
}

bool SettingsBridge::recordAudioEnabled() const
{
    return m_recordAudioEnabled;
}

void SettingsBridge::setRecordAudioEnabled(bool value)
{
    if (m_recordAudioEnabled == value)
        return;

    m_recordAudioEnabled = value;
    m_settings.setValue("recordAudioEnabled", value);
    emit recordAudioEnabledChanged();
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
    m_settings.setValue("audioMeterEnabled", value);
    emit audioMeterEnabledChanged();
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
    m_settings.setValue("inputVolume", clamped);
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
    m_settings.setValue("headphoneVolume", clamped);
    emit headphoneVolumeChanged();
}

QString SettingsBridge::batteryCapacity() const
{
    return m_batteryCapacity;
}

void SettingsBridge::setBatteryCapacity(const QString &value)
{
    if (m_batteryCapacity == value)
        return;

    m_batteryCapacity = value;
    m_settings.setValue("batteryCapacity", value);
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
    m_settings.setValue("customBatteryWh", clamped);
    emit customBatteryWhChanged();
}

QString SettingsBridge::sleepMode() const
{
    return m_sleepMode;
}

void SettingsBridge::setSleepMode(const QString &value)
{
    if (m_sleepMode == value)
        return;

    m_sleepMode = value;
    m_settings.setValue("sleepMode", value);
    emit sleepModeChanged();
}
