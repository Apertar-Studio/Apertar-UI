#pragma once

#include <QDateTime>
#include <QFileSystemWatcher>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QSettings>
#include <QTimer>

class ApertarControlBridge;

class SettingsBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool zebraEnabled READ zebraEnabled WRITE setZebraEnabled NOTIFY zebraEnabledChanged)
    Q_PROPERTY(float zebraThreshold READ zebraThreshold WRITE setZebraThreshold NOTIFY zebraThresholdChanged)

    Q_PROPERTY(bool focusPeakingEnabled READ focusPeakingEnabled WRITE setFocusPeakingEnabled NOTIFY focusPeakingEnabledChanged)
    Q_PROPERTY(float focusPeakingThreshold READ focusPeakingThreshold WRITE setFocusPeakingThreshold NOTIFY focusPeakingThresholdChanged)

    Q_PROPERTY(QString focusPeakingColor READ focusPeakingColor WRITE setFocusPeakingColor NOTIFY focusPeakingColorChanged)

    Q_PROPERTY(bool grayscaleEnabled READ grayscaleEnabled WRITE setGrayscaleEnabled NOTIFY grayscaleEnabledChanged)
    Q_PROPERTY(bool smpteEnabled READ smpteEnabled WRITE setSmpteEnabled NOTIFY smpteEnabledChanged)
    Q_PROPERTY(bool anamorphicDesqueezeEnabled READ anamorphicDesqueezeEnabled WRITE setAnamorphicDesqueezeEnabled NOTIFY anamorphicDesqueezeEnabledChanged)
    Q_PROPERTY(QString anamorphicRatio READ anamorphicRatio WRITE setAnamorphicRatio NOTIFY anamorphicRatioChanged)

    Q_PROPERTY(bool falseColorEnabled READ falseColorEnabled WRITE setFalseColorEnabled NOTIFY falseColorEnabledChanged)
    Q_PROPERTY(int falseColorMode READ falseColorMode WRITE setFalseColorMode NOTIFY falseColorModeChanged)
    Q_PROPERTY(bool lutPreviewEnabled READ lutPreviewEnabled WRITE setLutPreviewEnabled NOTIFY lutPreviewEnabledChanged)
    Q_PROPERTY(QString lutPreviewFile READ lutPreviewFile WRITE setLutPreviewFile NOTIFY lutPreviewFileChanged)
    Q_PROPERTY(QStringList lutPreviewOptions READ lutPreviewOptions NOTIFY lutPreviewOptionsChanged)
    Q_PROPERTY(QStringList tuningFileOptions READ tuningFileOptions NOTIFY tuningFileOptionsChanged)
    Q_PROPERTY(bool horizontalLevelEnabled READ horizontalLevelEnabled WRITE setHorizontalLevelEnabled NOTIFY horizontalLevelEnabledChanged)
    Q_PROPERTY(QString horizontalLevelColor READ horizontalLevelColor WRITE setHorizontalLevelColor NOTIFY horizontalLevelColorChanged)

    Q_PROPERTY(bool guidesEnabled READ guidesEnabled WRITE setGuidesEnabled NOTIFY guidesEnabledChanged)
    Q_PROPERTY(QString guidesType READ guidesType WRITE setGuidesType NOTIFY guidesTypeChanged)
    Q_PROPERTY(int guidesThickness READ guidesThickness WRITE setGuidesThickness NOTIFY guidesThicknessChanged)
    Q_PROPERTY(bool centerMarkerEnabled READ centerMarkerEnabled WRITE setCenterMarkerEnabled NOTIFY centerMarkerEnabledChanged)
    Q_PROPERTY(QString centerMarkerType READ centerMarkerType WRITE setCenterMarkerType NOTIFY centerMarkerTypeChanged)
    Q_PROPERTY(QString uiOrientation READ uiOrientation WRITE setUiOrientation NOTIFY uiOrientationChanged)
    Q_PROPERTY(double cameraControlsOpacity READ cameraControlsOpacity WRITE setCameraControlsOpacity NOTIFY cameraControlsOpacityChanged)
    Q_PROPERTY(QString cameraControlsMode READ cameraControlsMode WRITE setCameraControlsMode NOTIFY cameraControlsModeChanged)
    Q_PROPERTY(bool externalMonitorEnabled READ externalMonitorEnabled WRITE setExternalMonitorEnabled NOTIFY externalMonitorEnabledChanged)
    Q_PROPERTY(QString externalMonitorMode READ externalMonitorMode WRITE setExternalMonitorMode NOTIFY externalMonitorModeChanged)
    Q_PROPERTY(QString externalMonitorInfoOverlay READ externalMonitorInfoOverlay WRITE setExternalMonitorInfoOverlay NOTIFY externalMonitorInfoOverlayChanged)
    Q_PROPERTY(QString externalMonitorOrientation READ externalMonitorOrientation WRITE setExternalMonitorOrientation NOTIFY externalMonitorOrientationChanged)
    Q_PROPERTY(bool dateTimeOverlayEnabled READ dateTimeOverlayEnabled WRITE setDateTimeOverlayEnabled NOTIFY dateTimeOverlayEnabledChanged)
    Q_PROPERTY(QString timecodeMode READ timecodeMode WRITE setTimecodeMode NOTIFY timecodeModeChanged)
    Q_PROPERTY(bool photoModeEnabled READ photoModeEnabled WRITE setPhotoModeEnabled NOTIFY photoModeEnabledChanged)
    Q_PROPERTY(bool photoLongExposureEnabled READ photoLongExposureEnabled WRITE setPhotoLongExposureEnabled NOTIFY photoLongExposureEnabledChanged)
    Q_PROPERTY(bool photoBracketingEnabled READ photoBracketingEnabled WRITE setPhotoBracketingEnabled NOTIFY photoBracketingEnabledChanged)
    Q_PROPERTY(double photoBracketingEv READ photoBracketingEv WRITE setPhotoBracketingEv NOTIFY photoBracketingEvChanged)
    Q_PROPERTY(QString photoTimer READ photoTimer WRITE setPhotoTimer NOTIFY photoTimerChanged)
    Q_PROPERTY(QString photoBurst READ photoBurst WRITE setPhotoBurst NOTIFY photoBurstChanged)
    Q_PROPERTY(QString photoFormat READ photoFormat WRITE setPhotoFormat NOTIFY photoFormatChanged)
    Q_PROPERTY(QString photoTimelapse READ photoTimelapse WRITE setPhotoTimelapse NOTIFY photoTimelapseChanged)
    Q_PROPERTY(bool soundRecorderEnabled READ soundRecorderEnabled WRITE setSoundRecorderEnabled NOTIFY soundRecorderEnabledChanged)
    Q_PROPERTY(bool recordAudioEnabled READ recordAudioEnabled WRITE setRecordAudioEnabled NOTIFY recordAudioEnabledChanged)
    Q_PROPERTY(bool recordGyroEnabled READ recordGyroEnabled WRITE setRecordGyroEnabled NOTIFY recordGyroEnabledChanged)
    Q_PROPERTY(bool liveAudioMonitoringEnabled READ liveAudioMonitoringEnabled WRITE setLiveAudioMonitoringEnabled NOTIFY liveAudioMonitoringEnabledChanged)
    Q_PROPERTY(bool audioMeterEnabled READ audioMeterEnabled WRITE setAudioMeterEnabled NOTIFY audioMeterEnabledChanged)
    Q_PROPERTY(QString audioInputDevice READ audioInputDevice WRITE setAudioInputDevice NOTIFY audioInputDeviceChanged)
    Q_PROPERTY(QString audioOutputDevice READ audioOutputDevice WRITE setAudioOutputDevice NOTIFY audioOutputDeviceChanged)
    Q_PROPERTY(QString audioSampleFormat READ audioSampleFormat WRITE setAudioSampleFormat NOTIFY audioCaptureOptionsChanged)
    Q_PROPERTY(int audioSampleRate READ audioSampleRate WRITE setAudioSampleRate NOTIFY audioCaptureOptionsChanged)
    Q_PROPERTY(QString audioChannels READ audioChannels WRITE setAudioChannels NOTIFY audioCaptureOptionsChanged)
    Q_PROPERTY(int inputVolume READ inputVolume WRITE setInputVolume NOTIFY inputVolumeChanged)
    Q_PROPERTY(int headphoneVolume READ headphoneVolume WRITE setHeadphoneVolume NOTIFY headphoneVolumeChanged)
    Q_PROPERTY(QString batteryType READ batteryType WRITE setBatteryType NOTIFY batteryTypeChanged)
    Q_PROPERTY(QString batteryCapacity READ batteryCapacity WRITE setBatteryCapacity NOTIFY batteryCapacityChanged)
    Q_PROPERTY(int customBatteryWh READ customBatteryWh WRITE setCustomBatteryWh NOTIFY customBatteryWhChanged)
    Q_PROPERTY(double customBatteryFullVoltage READ customBatteryFullVoltage WRITE setCustomBatteryFullVoltage NOTIFY customBatteryFullVoltageChanged)
    Q_PROPERTY(double customBatteryEmptyVoltage READ customBatteryEmptyVoltage WRITE setCustomBatteryEmptyVoltage NOTIFY customBatteryEmptyVoltageChanged)
    Q_PROPERTY(QString sleepMode READ sleepMode WRITE setSleepMode NOTIFY sleepModeChanged)
    Q_PROPERTY(int standbyTimeoutSeconds READ standbyTimeoutSeconds WRITE setStandbyTimeoutSeconds NOTIFY standbyTimeoutSecondsChanged)
    Q_PROPERTY(bool standbyUsbPowerOff READ standbyUsbPowerOff WRITE setStandbyUsbPowerOff NOTIFY standbyUsbPowerOffChanged)
    Q_PROPERTY(QStringList monitoringOrder READ monitoringOrder WRITE setMonitoringOrder NOTIFY monitoringOrderChanged)
    Q_PROPERTY(QStringList recordOrder READ recordOrder WRITE setRecordOrder NOTIFY recordOrderChanged)
    Q_PROPERTY(QStringList audioOrder READ audioOrder WRITE setAudioOrder NOTIFY audioOrderChanged)
    Q_PROPERTY(QStringList powerOrder READ powerOrder WRITE setPowerOrder NOTIFY powerOrderChanged)
    Q_PROPERTY(QStringList systemOrder READ systemOrder WRITE setSystemOrder NOTIFY systemOrderChanged)
    Q_PROPERTY(QStringList wifiOrder READ wifiOrder WRITE setWifiOrder NOTIFY wifiOrderChanged)
    Q_PROPERTY(QStringList infoOrder READ infoOrder WRITE setInfoOrder NOTIFY infoOrderChanged)
    Q_PROPERTY(QStringList sidebarOrder READ sidebarOrder WRITE setSidebarOrder NOTIFY sidebarOrderChanged)
    Q_PROPERTY(QStringList favoritesOrder READ favoritesOrder WRITE setFavoritesOrder NOTIFY favoritesOrderChanged)
    Q_PROPERTY(QStringList presetNames READ presetNames NOTIFY presetNamesChanged)
    Q_PROPERTY(QString presetStatusText READ presetStatusText NOTIFY presetStatusChanged)
    Q_PROPERTY(bool presetStatusError READ presetStatusError NOTIFY presetStatusChanged)

public:
    explicit SettingsBridge(QObject *parent = nullptr);
    void setControlBridge(ApertarControlBridge *controlBridge);
    void setDefaultCameraControlsOpacity(double value);

    bool zebraEnabled() const;
    void setZebraEnabled(bool value);

    float zebraThreshold() const;
    void setZebraThreshold(float value);

    bool focusPeakingEnabled() const;
    void setFocusPeakingEnabled(bool value);

    float focusPeakingThreshold() const;
    void setFocusPeakingThreshold(float value);

    QString focusPeakingColor() const;
    void setFocusPeakingColor(const QString &value);

    bool grayscaleEnabled() const;
    void setGrayscaleEnabled(bool value);

    bool smpteEnabled() const;
    void setSmpteEnabled(bool value);

    bool anamorphicDesqueezeEnabled() const;
    void setAnamorphicDesqueezeEnabled(bool value);

    QString anamorphicRatio() const;
    void setAnamorphicRatio(const QString &value);

    bool falseColorEnabled() const;
    void setFalseColorEnabled(bool value);

    int falseColorMode() const;
    void setFalseColorMode(int value);

    bool lutPreviewEnabled() const;
    void setLutPreviewEnabled(bool value);

    QString lutPreviewFile() const;
    void setLutPreviewFile(const QString &value);

    QStringList lutPreviewOptions() const;
    Q_INVOKABLE void refreshLutPreviewOptions();

    QStringList tuningFileOptions() const;
    Q_INVOKABLE void refreshTuningFileOptions();

    bool horizontalLevelEnabled() const;
    void setHorizontalLevelEnabled(bool value);

    QString horizontalLevelColor() const;
    void setHorizontalLevelColor(const QString &value);

    bool guidesEnabled() const;
    void setGuidesEnabled(bool value);

    QString guidesType() const;
    void setGuidesType(const QString &value);

    int guidesThickness() const;
    void setGuidesThickness(int value);

    bool centerMarkerEnabled() const;
    void setCenterMarkerEnabled(bool value);

    QString centerMarkerType() const;
    void setCenterMarkerType(const QString &value);

    QString uiOrientation() const;
    void setUiOrientation(const QString &value);

    double cameraControlsOpacity() const;
    void setCameraControlsOpacity(double value);

    QString cameraControlsMode() const;
    void setCameraControlsMode(const QString &value);

    bool externalMonitorEnabled() const;
    void setExternalMonitorEnabled(bool value);

    QString externalMonitorMode() const;
    void setExternalMonitorMode(const QString &value);

    QString externalMonitorInfoOverlay() const;
    void setExternalMonitorInfoOverlay(const QString &value);

    QString externalMonitorOrientation() const;
    void setExternalMonitorOrientation(const QString &value);

    bool dateTimeOverlayEnabled() const;
    void setDateTimeOverlayEnabled(bool value);

    QString timecodeMode() const;
    void setTimecodeMode(const QString &value);

    bool photoModeEnabled() const;
    void setPhotoModeEnabled(bool value);
    bool photoLongExposureEnabled() const;
    void setPhotoLongExposureEnabled(bool value);
    bool photoBracketingEnabled() const;
    void setPhotoBracketingEnabled(bool value);
    double photoBracketingEv() const { return m_photoBracketingEv; }
    void setPhotoBracketingEv(double value);

    QString photoTimer() const;
    void setPhotoTimer(const QString &value);

    QString photoBurst() const;
    void setPhotoBurst(const QString &value);

    QString photoFormat() const;
    void setPhotoFormat(const QString &value);

    QString photoTimelapse() const;
    void setPhotoTimelapse(const QString &value);

    bool soundRecorderEnabled() const;
    void setSoundRecorderEnabled(bool value);

    bool recordAudioEnabled() const;
    void setRecordAudioEnabled(bool value);

    bool recordGyroEnabled() const;
    void setRecordGyroEnabled(bool value);

    bool liveAudioMonitoringEnabled() const;
    void setLiveAudioMonitoringEnabled(bool value);

    bool audioMeterEnabled() const;
    void setAudioMeterEnabled(bool value);

    QString audioInputDevice() const;
    void setAudioInputDevice(const QString &value);

    QString audioOutputDevice() const;
    void setAudioOutputDevice(const QString &value);
    QString audioSampleFormat() const { return m_audioSampleFormat; }
    int audioSampleRate() const { return m_audioSampleRate; }
    QString audioChannels() const { return m_audioChannels; }
    void setAudioSampleFormat(const QString &value);
    void setAudioSampleRate(int value);
    void setAudioChannels(const QString &value);

    int inputVolume() const;
    void setInputVolume(int value);

    int headphoneVolume() const;
    void setHeadphoneVolume(int value);

    QString batteryType() const;
    void setBatteryType(const QString &value);

    QString batteryCapacity() const;
    void setBatteryCapacity(const QString &value);

    int customBatteryWh() const;
    void setCustomBatteryWh(int value);

    double customBatteryFullVoltage() const;
    void setCustomBatteryFullVoltage(double value);

    double customBatteryEmptyVoltage() const;
    void setCustomBatteryEmptyVoltage(double value);

    QString sleepMode() const;
    void setSleepMode(const QString &value);
    int standbyTimeoutSeconds() const { return m_standbyTimeoutSeconds; }
    void setStandbyTimeoutSeconds(int value);
    bool standbyUsbPowerOff() const { return m_standbyUsbPowerOff; }
    void setStandbyUsbPowerOff(bool value);

    QStringList monitoringOrder() const;
    void setMonitoringOrder(const QStringList &value);

    QStringList recordOrder() const;
    void setRecordOrder(const QStringList &value);

    QStringList audioOrder() const;
    void setAudioOrder(const QStringList &value);

    QStringList powerOrder() const;
    void setPowerOrder(const QStringList &value);

    QStringList systemOrder() const;
    void setSystemOrder(const QStringList &value);

    QStringList wifiOrder() const;
    void setWifiOrder(const QStringList &value);

    QStringList infoOrder() const;
    void setInfoOrder(const QStringList &value);

    QStringList sidebarOrder() const;
    void setSidebarOrder(const QStringList &value);

    QStringList favoritesOrder() const;
    void setFavoritesOrder(const QStringList &value);

    QStringList presetNames() const;
    QString presetStatusText() const;
    bool presetStatusError() const;

    Q_INVOKABLE bool saveCurrentAsPreset(const QString &name);
    Q_INVOKABLE bool loadPreset(const QString &name);
    Q_INVOKABLE bool deletePreset(const QString &name);
    Q_INVOKABLE bool exportPresetToPath(const QString &name, const QString &mediaMountPath);
    Q_INVOKABLE bool importPresetsFromPath(const QString &mediaMountPath);
    Q_INVOKABLE void clearPresetStatus();

signals:
    void zebraEnabledChanged();
    void zebraThresholdChanged();

    void focusPeakingEnabledChanged();
    void focusPeakingThresholdChanged();

    void focusPeakingColorChanged();

    void grayscaleEnabledChanged();
    void smpteEnabledChanged();
    void anamorphicDesqueezeEnabledChanged();
    void anamorphicRatioChanged();

    void falseColorEnabledChanged();
    void falseColorModeChanged();
    void lutPreviewEnabledChanged();
    void lutPreviewFileChanged();
    void lutPreviewOptionsChanged();
    void tuningFileOptionsChanged();
    void horizontalLevelEnabledChanged();
    void horizontalLevelColorChanged();

    void guidesEnabledChanged();
    void guidesTypeChanged();
    void guidesThicknessChanged();

    void centerMarkerEnabledChanged();
    void centerMarkerTypeChanged();
    void uiOrientationChanged();
    void cameraControlsOpacityChanged();
    void cameraControlsModeChanged();
    void externalMonitorEnabledChanged();
    void externalMonitorModeChanged();
    void externalMonitorInfoOverlayChanged();
    void externalMonitorOrientationChanged();
    void dateTimeOverlayEnabledChanged();
    void timecodeModeChanged();
    void photoModeEnabledChanged();
    void photoLongExposureEnabledChanged();
    void photoBracketingEnabledChanged();
    void photoBracketingEvChanged();
    void photoTimerChanged();
    void photoBurstChanged();
    void photoFormatChanged();
    void photoTimelapseChanged();
    void soundRecorderEnabledChanged();
    void recordAudioEnabledChanged();
    void recordGyroEnabledChanged();
    void liveAudioMonitoringEnabledChanged();
    void audioMeterEnabledChanged();
    void audioInputDeviceChanged();
    void audioOutputDeviceChanged();
    void audioCaptureOptionsChanged();
    void inputVolumeChanged();
    void headphoneVolumeChanged();
    void batteryTypeChanged();
    void batteryCapacityChanged();
    void customBatteryWhChanged();
    void customBatteryFullVoltageChanged();
    void customBatteryEmptyVoltageChanged();
    void sleepModeChanged();
    void standbyTimeoutSecondsChanged();
    void standbyUsbPowerOffChanged();
    void monitoringOrderChanged();
    void recordOrderChanged();
    void audioOrderChanged();
    void powerOrderChanged();
    void systemOrderChanged();
    void wifiOrderChanged();
    void infoOrderChanged();
    void sidebarOrderChanged();
    void favoritesOrderChanged();
    void presetNamesChanged();
    void presetStatusChanged();

private:
    QString presetsDirectoryPath() const;
    QStringList currentPresetDirectorySignature() const;
    QString sanitizedPresetFileStem(const QString &name) const;
    QString presetFilePathForName(const QString &name) const;
    QString uniquePresetName(const QString &baseName) const;
    QJsonObject serializeCurrentSettings() const;
    bool applySettingsObject(const QJsonObject &settingsObject, QString *errorMessage);
    bool writePresetFile(const QString &filePath,
                         const QString &presetName,
                         const QJsonObject &settingsObject,
                         QString *errorMessage) const;
    bool readPresetFile(const QString &filePath,
                        QString *presetName,
                        QJsonObject *settingsObject,
                        QString *errorMessage) const;
    void reloadPresetNames();
    void setPresetStatus(const QString &text, bool error);
    void pollExternalSettings();
    void updateSettingsFileState();
    void ensureExternalSettingsWatchPaths();

    QSettings m_settings;
    QPointer<ApertarControlBridge> m_controlBridge;
    QFileSystemWatcher m_externalSettingsWatcher;
    QTimer m_externalSettingsPollTimer;
    QString m_settingsFilePath;
    QDateTime m_settingsFileLastModified;
    qint64 m_settingsFileSize = -1;
    bool m_settingsSnapshotInitialized = false;
    bool m_externalSettingsSyncing = false;

    bool m_zebraEnabled = false;
    float m_zebraThreshold = 0.70f;

    bool m_focusPeakingEnabled = false;
    float m_focusPeakingThreshold = 0.04f;

    QString m_focusPeakingColor = "Red";

    bool m_grayscaleEnabled = false;
    bool m_smpteEnabled = false;
    bool m_anamorphicDesqueezeEnabled = false;
    QString m_anamorphicRatio = "1.33x";

    bool m_falseColorEnabled = false;
    int m_falseColorMode = 0;
    bool m_lutPreviewEnabled = false;
    QString m_lutPreviewFile;
    QStringList m_lutPreviewOptions;
    QStringList m_tuningFileOptions;
    bool m_horizontalLevelEnabled = false;
    QString m_horizontalLevelColor = "Blue";

    bool m_guidesEnabled = true;
    QString m_guidesType = "Thirds";
    int m_guidesThickness = 1;

    bool m_centerMarkerEnabled = true;
    QString m_centerMarkerType = "Circle/Dot";
    QString m_uiOrientation = "Landscape";
    double m_cameraControlsOpacity = 1.0;
    QString m_cameraControlsMode = "Light";
    bool m_externalMonitorEnabled = false;
    QString m_externalMonitorMode = "Clean Feed";
    QString m_externalMonitorInfoOverlay = "On";
    QString m_externalMonitorOrientation = "Landscape";
    bool m_dateTimeOverlayEnabled = false;
    QString m_timecodeMode = "Free Run";
    bool m_photoModeEnabled = false;
    bool m_photoLongExposureEnabled = false;
    bool m_photoBracketingEnabled = false;
    double m_photoBracketingEv = 1.0;
    QString m_photoTimer = "Off";
    QString m_photoBurst = "Single";
    QString m_photoFormat = "DNG";
    QString m_photoTimelapse = "Off";
    bool m_soundRecorderEnabled = false;
    bool m_recordAudioEnabled = false;
    bool m_recordGyroEnabled = false;
    bool m_liveAudioMonitoringEnabled = true;
    bool m_audioMeterEnabled = false;
    QString m_audioInputDevice;
    QString m_audioOutputDevice;
    QString m_audioSampleFormat = "auto";
    int m_audioSampleRate = 0;
    QString m_audioChannels = "auto";
    int m_inputVolume = 60;
    int m_headphoneVolume = 55;
    QString m_batteryType = "V-Mount";
    QString m_batteryCapacity = "150Wh";
    int m_customBatteryWh = 150;
    double m_customBatteryFullVoltage = 16.8;
    double m_customBatteryEmptyVoltage = 13.2;
    QString m_sleepMode = "Off";
    int m_standbyTimeoutSeconds = 0;
    bool m_standbyUsbPowerOff = false;
    QStringList m_monitoringOrder;
    QStringList m_recordOrder;
    QStringList m_audioOrder;
    QStringList m_powerOrder;
    QStringList m_systemOrder;
    QStringList m_wifiOrder;
    QStringList m_infoOrder;
    QStringList m_sidebarOrder;
    QStringList m_favoritesOrder;
    QStringList m_presetNames;
    QStringList m_presetDirectorySignature;
    QString m_presetStatusText;
    bool m_presetStatusError = false;
};
