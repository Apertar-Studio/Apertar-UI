#pragma once

#include <QObject>
#include <QSettings>

class SettingsBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool zebraEnabled READ zebraEnabled WRITE setZebraEnabled NOTIFY zebraEnabledChanged)
    Q_PROPERTY(float zebraThreshold READ zebraThreshold WRITE setZebraThreshold NOTIFY zebraThresholdChanged)

    Q_PROPERTY(bool focusPeakingEnabled READ focusPeakingEnabled WRITE setFocusPeakingEnabled NOTIFY focusPeakingEnabledChanged)
    Q_PROPERTY(float focusPeakingThreshold READ focusPeakingThreshold WRITE setFocusPeakingThreshold NOTIFY focusPeakingThresholdChanged)

    Q_PROPERTY(QString focusPeakingColor READ focusPeakingColor WRITE setFocusPeakingColor NOTIFY focusPeakingColorChanged)

    Q_PROPERTY(bool grayscaleEnabled READ grayscaleEnabled WRITE setGrayscaleEnabled NOTIFY grayscaleEnabledChanged)
    Q_PROPERTY(bool anamorphicDesqueezeEnabled READ anamorphicDesqueezeEnabled WRITE setAnamorphicDesqueezeEnabled NOTIFY anamorphicDesqueezeEnabledChanged)
    Q_PROPERTY(QString anamorphicRatio READ anamorphicRatio WRITE setAnamorphicRatio NOTIFY anamorphicRatioChanged)

    Q_PROPERTY(bool falseColorEnabled READ falseColorEnabled WRITE setFalseColorEnabled NOTIFY falseColorEnabledChanged)
    Q_PROPERTY(int falseColorMode READ falseColorMode WRITE setFalseColorMode NOTIFY falseColorModeChanged)

    Q_PROPERTY(bool guidesEnabled READ guidesEnabled WRITE setGuidesEnabled NOTIFY guidesEnabledChanged)
    Q_PROPERTY(QString guidesType READ guidesType WRITE setGuidesType NOTIFY guidesTypeChanged)
    Q_PROPERTY(bool centerMarkerEnabled READ centerMarkerEnabled WRITE setCenterMarkerEnabled NOTIFY centerMarkerEnabledChanged)
    Q_PROPERTY(QString centerMarkerType READ centerMarkerType WRITE setCenterMarkerType NOTIFY centerMarkerTypeChanged)
    Q_PROPERTY(QString timecodeMode READ timecodeMode WRITE setTimecodeMode NOTIFY timecodeModeChanged)
    Q_PROPERTY(bool photoModeEnabled READ photoModeEnabled WRITE setPhotoModeEnabled NOTIFY photoModeEnabledChanged)
    Q_PROPERTY(QString photoTimer READ photoTimer WRITE setPhotoTimer NOTIFY photoTimerChanged)
    Q_PROPERTY(QString photoBurst READ photoBurst WRITE setPhotoBurst NOTIFY photoBurstChanged)
    Q_PROPERTY(bool recordAudioEnabled READ recordAudioEnabled WRITE setRecordAudioEnabled NOTIFY recordAudioEnabledChanged)
    Q_PROPERTY(bool audioMeterEnabled READ audioMeterEnabled WRITE setAudioMeterEnabled NOTIFY audioMeterEnabledChanged)
    Q_PROPERTY(int inputVolume READ inputVolume WRITE setInputVolume NOTIFY inputVolumeChanged)
    Q_PROPERTY(int headphoneVolume READ headphoneVolume WRITE setHeadphoneVolume NOTIFY headphoneVolumeChanged)
    Q_PROPERTY(QString batteryCapacity READ batteryCapacity WRITE setBatteryCapacity NOTIFY batteryCapacityChanged)
    Q_PROPERTY(int customBatteryWh READ customBatteryWh WRITE setCustomBatteryWh NOTIFY customBatteryWhChanged)
    Q_PROPERTY(QString sleepMode READ sleepMode WRITE setSleepMode NOTIFY sleepModeChanged)

public:
    explicit SettingsBridge(QObject *parent = nullptr);

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

    bool anamorphicDesqueezeEnabled() const;
    void setAnamorphicDesqueezeEnabled(bool value);

    QString anamorphicRatio() const;
    void setAnamorphicRatio(const QString &value);

    bool falseColorEnabled() const;
    void setFalseColorEnabled(bool value);

    int falseColorMode() const;
    void setFalseColorMode(int value);

    bool guidesEnabled() const;
    void setGuidesEnabled(bool value);

    QString guidesType() const;
    void setGuidesType(const QString &value);

    bool centerMarkerEnabled() const;
    void setCenterMarkerEnabled(bool value);

    QString centerMarkerType() const;
    void setCenterMarkerType(const QString &value);

    QString timecodeMode() const;
    void setTimecodeMode(const QString &value);

    bool photoModeEnabled() const;
    void setPhotoModeEnabled(bool value);

    QString photoTimer() const;
    void setPhotoTimer(const QString &value);

    QString photoBurst() const;
    void setPhotoBurst(const QString &value);

    bool recordAudioEnabled() const;
    void setRecordAudioEnabled(bool value);

    bool audioMeterEnabled() const;
    void setAudioMeterEnabled(bool value);

    int inputVolume() const;
    void setInputVolume(int value);

    int headphoneVolume() const;
    void setHeadphoneVolume(int value);

    QString batteryCapacity() const;
    void setBatteryCapacity(const QString &value);

    int customBatteryWh() const;
    void setCustomBatteryWh(int value);

    QString sleepMode() const;
    void setSleepMode(const QString &value);

signals:
    void zebraEnabledChanged();
    void zebraThresholdChanged();

    void focusPeakingEnabledChanged();
    void focusPeakingThresholdChanged();

    void focusPeakingColorChanged();

    void grayscaleEnabledChanged();
    void anamorphicDesqueezeEnabledChanged();
    void anamorphicRatioChanged();

    void falseColorEnabledChanged();
    void falseColorModeChanged();

    void guidesEnabledChanged();
    void guidesTypeChanged();

    void centerMarkerEnabledChanged();
    void centerMarkerTypeChanged();
    void timecodeModeChanged();
    void photoModeEnabledChanged();
    void photoTimerChanged();
    void photoBurstChanged();
    void recordAudioEnabledChanged();
    void audioMeterEnabledChanged();
    void inputVolumeChanged();
    void headphoneVolumeChanged();
    void batteryCapacityChanged();
    void customBatteryWhChanged();
    void sleepModeChanged();

private:
    QSettings m_settings;

    bool m_zebraEnabled = false;
    float m_zebraThreshold = 0.70f;

    bool m_focusPeakingEnabled = false;
    float m_focusPeakingThreshold = 0.04f;

    QString m_focusPeakingColor = "Red";

    bool m_grayscaleEnabled = false;
    bool m_anamorphicDesqueezeEnabled = false;
    QString m_anamorphicRatio = "1.33x";

    bool m_falseColorEnabled = false;
    int m_falseColorMode = 0;

    bool m_guidesEnabled = true;
    QString m_guidesType = "Thirds";

    bool m_centerMarkerEnabled = true;
    QString m_centerMarkerType = "Circle/Dot";
    QString m_timecodeMode = "Free Run";
    bool m_photoModeEnabled = false;
    QString m_photoTimer = "Off";
    QString m_photoBurst = "Single";
    bool m_recordAudioEnabled = false;
    bool m_audioMeterEnabled = false;
    int m_inputVolume = 60;
    int m_headphoneVolume = 55;
    QString m_batteryCapacity = "150Wh";
    int m_customBatteryWh = 150;
    QString m_sleepMode = "Off";
};
