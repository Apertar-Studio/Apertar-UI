#pragma once

#include <QObject>
#include <QJsonObject>
#include <QLocalSocket>
#include <QSettings>
#include <QString>
#include <QTimer>
#include <QElapsedTimer>
#include <QVariantList>

class ApertarControlBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(QString iso READ iso NOTIFY isoChanged)
    Q_PROPERTY(QString shutterAngle READ shutterAngle NOTIFY shutterAngleChanged)
    Q_PROPERTY(QString shutterSpeed READ shutterSpeed NOTIFY shutterSpeedChanged)
    Q_PROPERTY(QString whiteBalance READ whiteBalance NOTIFY whiteBalanceChanged)
    Q_PROPERTY(bool whiteBalanceAvailable READ whiteBalanceAvailable NOTIFY whiteBalanceAvailableChanged)
    Q_PROPERTY(double exposureCompensation READ exposureCompensation NOTIFY exposureCompensationChanged)
    Q_PROPERTY(bool exposureCompensationAvailable READ exposureCompensationAvailable NOTIFY exposureCompensationChanged)
    Q_PROPERTY(bool autoExposureEnabled READ autoExposureEnabled NOTIFY exposureCompensationChanged)
    Q_PROPERTY(QString exposureMetering READ exposureMetering NOTIFY exposureMeteringChanged)
    Q_PROPERTY(QVariantList exposureMeteringOptions READ exposureMeteringOptions NOTIFY exposureMeteringChanged)
    Q_PROPERTY(QString resolution READ resolution NOTIFY resolutionChanged)
    Q_PROPERTY(QString recordingFormat READ recordingFormat NOTIFY recordingFormatChanged)
    Q_PROPERTY(QString proresProfile READ proresProfile NOTIFY proresProfileChanged)
    Q_PROPERTY(QString proresColorProfile READ proresColorProfile NOTIFY proresColorProfileChanged)
    Q_PROPERTY(QString cdngPreviewColorProfile READ cdngPreviewColorProfile NOTIFY cdngPreviewColorProfileChanged)
    Q_PROPERTY(QVariantMap previewExposureReference READ previewExposureReference NOTIFY previewExposureReferenceChanged)
    Q_PROPERTY(QString cdngCompression READ cdngCompression NOTIFY cdngCompressionChanged)
    Q_PROPERTY(bool rawCompressionSupported READ rawCompressionSupported NOTIFY recordingCapabilitiesChanged)
    Q_PROPERTY(bool rawRecordingSupported READ rawRecordingSupported NOTIFY recordingCapabilitiesChanged)
    Q_PROPERTY(bool proresSupported READ proresSupported NOTIFY recordingCapabilitiesChanged)
    Q_PROPERTY(bool h264HardwareEncoding READ h264HardwareEncoding NOTIFY recordingCapabilitiesChanged)
    Q_PROPERTY(int h264MaxWidth READ h264MaxWidth NOTIFY recordingCapabilitiesChanged)
    Q_PROPERTY(double h264MaxFps READ h264MaxFps NOTIFY recordingCapabilitiesChanged)
    Q_PROPERTY(QString tuningFile READ tuningFile NOTIFY tuningFileChanged)
    Q_PROPERTY(QString rawBitDepth READ rawBitDepth NOTIFY rawBitDepthChanged)
    Q_PROPERTY(QString h264Resolution READ h264Resolution NOTIFY h264ResolutionChanged)
    Q_PROPERTY(QString sensorName READ sensorName NOTIFY sensorNameChanged)
    Q_PROPERTY(QString autofocusMode READ autofocusMode NOTIFY autofocusModeChanged)
    Q_PROPERTY(QString autofocusRange READ autofocusRange NOTIFY autofocusRangeChanged)
    Q_PROPERTY(QString autofocusSpeed READ autofocusSpeed NOTIFY autofocusSpeedChanged)
    Q_PROPERTY(QString autofocusWindow READ autofocusWindow NOTIFY autofocusWindowChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(bool recordingHasDroppedFrames READ recordingHasDroppedFrames NOTIFY recordingHealthChanged)
    Q_PROPERTY(bool standby READ standby NOTIFY standbyChanged)
    Q_PROPERTY(bool timelapseActive READ timelapseActive NOTIFY timelapseActiveChanged)
    Q_PROPERTY(qulonglong timelapseShotCount READ timelapseShotCount NOTIFY timelapseShotCountChanged)
    Q_PROPERTY(bool stillExposureActive READ stillExposureActive NOTIFY stillExposureChanged)
    Q_PROPERTY(bool mediaWritesPending READ mediaWritesPending NOTIFY mediaWritesPendingChanged)
    Q_PROPERTY(double stillExposureRemainingSeconds READ stillExposureRemainingSeconds NOTIFY stillExposureChanged)
    Q_PROPERTY(int audioInputLevel READ audioInputLevel NOTIFY audioInputLevelChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit ApertarControlBridge(QObject *parent = nullptr);

    QString fps() const;
    QString iso() const;
    QString shutterAngle() const;
    QString shutterSpeed() const;
    QString whiteBalance() const;
    bool whiteBalanceAvailable() const { return m_whiteBalanceAvailable; }
    double exposureCompensation() const { return m_exposureCompensation; }
    bool exposureCompensationAvailable() const { return m_exposureCompensationAvailable; }
    bool autoExposureEnabled() const { return m_isoAuto || m_shutterAuto; }
    QString exposureMetering() const { return m_exposureMetering; }
    QVariantList exposureMeteringOptions() const { return m_exposureMeteringOptions; }
    QString resolution() const;
    QString recordingFormat() const;
    QString proresProfile() const { return m_proresProfile; }
    QString proresColorProfile() const { return m_proresColorProfile; }
    QString cdngPreviewColorProfile() const { return m_cdngPreviewColorProfile; }
    QVariantMap previewExposureReference() const { return m_previewExposureReference; }
    QString cdngCompression() const { return m_cdngCompression; }
    bool rawCompressionSupported() const { return m_rawCompressionSupported; }
    bool rawRecordingSupported() const { return m_rawRecordingSupported; }
    bool proresSupported() const { return m_proresSupported; }
    bool h264HardwareEncoding() const { return m_h264HardwareEncoding; }
    int h264MaxWidth() const { return m_h264MaxWidth; }
    double h264MaxFps() const { return m_h264MaxFps; }
    QString tuningFile() const { return m_tuningFile; }
    QString rawBitDepth() const;
    QString h264Resolution() const;
    QString sensorName() const;
    QString autofocusMode() const;
    QString autofocusRange() const;
    QString autofocusSpeed() const;
    QString autofocusWindow() const;
    bool recording() const;
    bool recordingHasDroppedFrames() const { return m_recordingHasDroppedFrames; }
    bool standby() const { return m_standby; }
    Q_INVOKABLE bool enterStandby();
    Q_INVOKABLE bool wakeFromStandby();
    Q_INVOKABLE void notifyUserActivity();
    bool timelapseActive() const;
    qulonglong timelapseShotCount() const { return m_timelapseShotCount; }
    bool stillExposureActive() const { return m_stillExposureActive; }
    bool mediaWritesPending() const { return m_mediaWritesPending; }
    double stillExposureRemainingSeconds() const;
    int audioInputLevel() const;
    bool connected() const;
    QString lastError() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool applyFps(const QString &displayValue);
    Q_INVOKABLE bool applyIso(const QString &displayValue);
    Q_INVOKABLE bool applyExposureCompensation(double ev);
    Q_INVOKABLE bool applyExposureMetering(const QString &mode);
    Q_INVOKABLE bool applyShutterAngle(const QString &displayValue);
    Q_INVOKABLE bool applyShutterSpeed(const QString &displayValue);
    Q_INVOKABLE bool applyWhiteBalance(const QString &displayValue);
    Q_INVOKABLE bool applyResolution(const QString &displayValue);
    Q_INVOKABLE bool applyRecordingFormat(const QString &displayValue);
    Q_INVOKABLE bool applyProresProfile(const QString &value);
    Q_INVOKABLE bool applyProresColorProfile(const QString &value);
    Q_INVOKABLE bool applyCdngPreviewColorProfile(const QString &value);
    Q_INVOKABLE bool applyCdngCompression(const QString &value);
    Q_INVOKABLE bool applyTuningFile(const QString &path);
    Q_INVOKABLE bool applyRawBitDepth(const QString &displayValue);
    Q_INVOKABLE bool applyH264Resolution(const QString &displayValue);
    Q_INVOKABLE bool applyTimecodeMode(const QString &displayValue);
    Q_INVOKABLE bool applyAutofocusMode(const QString &displayValue);
    Q_INVOKABLE bool applyAutofocusRange(const QString &displayValue);
    Q_INVOKABLE bool applyAutofocusSpeed(const QString &displayValue);
    Q_INVOKABLE bool applyAutofocusWindow(const QString &displayValue);
    Q_INVOKABLE bool applyPhotoTimelapse(const QString &displayValue, const QString &format);
    Q_INVOKABLE bool applyPhotoModeEnabled(bool enabled);
    Q_INVOKABLE bool applyCaptureMode(const QString &mode);
    Q_INVOKABLE bool applyPhotoLongExposureEnabled(bool enabled);
    Q_INVOKABLE bool applyPhotoBracketingEnabled(bool enabled);
    Q_INVOKABLE bool applyPhotoBracketingEv(double ev);
    Q_INVOKABLE bool setTimelapseActive(bool active, const QString &displayValue, const QString &format);
    Q_INVOKABLE bool setRecording(bool recording);
    Q_INVOKABLE bool capturePhoto(const QString &format, int count = 1);
    Q_INVOKABLE bool calibrateGyro();
    QJsonObject cameraPresetSettings() const;
    bool applyCameraPreset(const QJsonObject &settings);

signals:
    void fpsChanged();
    void isoChanged();
    void shutterAngleChanged();
    void shutterSpeedChanged();
    void whiteBalanceChanged();
    void whiteBalanceAvailableChanged();
    void exposureCompensationChanged();
    void exposureMeteringChanged();
    void resolutionChanged();
    void recordingFormatChanged();
    void proresProfileChanged();
    void proresColorProfileChanged();
    void cdngPreviewColorProfileChanged();
    void previewExposureReferenceChanged();
    void cdngCompressionChanged();
    void recordingCapabilitiesChanged();
    void tuningFileChanged();
    void rawBitDepthChanged();
    void h264ResolutionChanged();
    void sensorNameChanged();
    void autofocusModeChanged();
    void autofocusRangeChanged();
    void autofocusSpeedChanged();
    void autofocusWindowChanged();
    void recordingChanged();
    void recordingHealthChanged();
    void standbyChanged();
    void timelapseActiveChanged();
    void mediaWritesPendingChanged();
    void timelapseShotCountChanged();
    void stillExposureChanged();
    void audioInputLevelChanged();
    void connectedChanged();
    void lastErrorChanged();
    void recordingFailed(const QString &error);
    void gyroCalibrationFinished(bool ok, const QString &message);
    void sharedPhotoStateChanged(bool photoModeEnabled,
                                 const QString &photoFormat,
                                 const QString &photoTimelapse,
                                 bool longExposureEnabled,
                                 bool bracketingEnabled,
                                 double bracketingEv);
    void timelapseFrameCaptured();
    void photoCaptured();
    void photoCaptureFailed(const QString &error);
    void externalRecordingPreparationRequested();
    void hardwareActionTriggered(const QString &action);

private:
    void refreshFromCore();
    void updateRefreshTimer();
    void loadPersistedCameraSettings();
    void saveCurrentCameraSettings(int shutterUsOverride = -1);
    bool sendCoreCommand(const QString &commandName,
                         const QJsonObject &arguments = {},
                         QJsonObject *replyObject = nullptr);
    void applyCoreMessage(const QJsonObject &message);
    void applyCoreState(const QJsonObject &state);
    void ensureEventSocketConnected();
    void processEventSocketMessages();
    void applyShutterFromMicroseconds(double shutterUs, double fps);
    int shutterUsForAngle(double angle, double fps) const;
    int shutterUsForSpeed(double denominator) const;
    double currentFpsValue() const;

    QString formatFpsValue(double value) const;
    QString formatIsoValue(double value) const;
    QString formatShutterAngleValue(double value) const;
    QString formatShutterSpeedValue(double value) const;
    QString formatResolutionValue(int width, int height) const;
    QString formatH264ResolutionValue(int width, int height) const;
    QString formatRecordingFormatValue(const QString &value) const;
    QString parseLossyDngRatioValue(const QString &displayValue) const;
    QString formatRawBitDepthValue(int value) const;
    QString formatAutofocusModeValue(const QString &value) const;
    QString formatAutofocusRangeValue(const QString &value) const;
    QString formatAutofocusSpeedValue(const QString &value) const;
    QString formatAutofocusWindowValue(double x, double y, double width, double height) const;

    double parseFpsValue(const QString &displayValue) const;
    double parseIsoValue(const QString &displayValue) const;
    double parseShutterAngleValue(const QString &displayValue) const;
    double parseShutterSpeedValue(const QString &displayValue) const;
    double parseWhiteBalanceValue(const QString &displayValue) const;
    bool parseResolutionValue(const QString &displayValue, int *width, int *height) const;
    bool parseH264ResolutionValue(const QString &displayValue, int *width, int *height) const;
    QString parseRecordingFormatValue(const QString &displayValue) const;
    int parseRawBitDepthValue(const QString &displayValue) const;
    QString parseTimecodeModeValue(const QString &displayValue) const;
    QString parseAutofocusModeValue(const QString &displayValue) const;
    QString parseAutofocusRangeValue(const QString &displayValue) const;
    QString parseAutofocusSpeedValue(const QString &displayValue) const;
    bool autofocusWindowPreset(const QString &displayValue,
                               double *x,
                               double *y,
                               double *width,
                               double *height) const;
    QString audioDeviceIdFromSelection(const QString &displayValue) const;
    QString compactNumber(double value, int decimals = 3) const;
    void setFps(const QString &value);
    void setIso(const QString &value);
    void setShutterAngle(const QString &value);
    void setShutterSpeed(const QString &value);
    void setWhiteBalance(const QString &value);
    void setResolution(const QString &value);
    void setRecordingFormatValue(const QString &value);
    void setProresProfileValue(const QString &value);
    void setProresColorProfileValue(const QString &value);
    void setCdngPreviewColorProfileValue(const QString &value);
    void setTuningFileValue(const QString &value);
    void setRawBitDepthValue(const QString &value);
    void setH264ResolutionValue(const QString &value);
    void setSensorNameValue(const QString &value);
    void setAutofocusModeValue(const QString &value);
    void setAutofocusRangeValue(const QString &value);
    void setAutofocusSpeedValue(const QString &value);
    void setAutofocusWindowValue(const QString &value);
    void setRecordingState(bool value);
    void setTimelapseActiveState(bool value);
    void setTimelapseShotCount(qulonglong value);
    void setAudioInputLevel(int value);
    void setConnected(bool value);
    void setLastError(const QString &value);

    QSettings m_settings;
    bool m_rawCompressionSupported = false;
    bool m_rawRecordingSupported = false;
    bool m_proresSupported = false;
    bool m_h264HardwareEncoding = false;
    int m_h264MaxWidth = 2560;
    double m_h264MaxFps = 0;
    QLocalSocket m_eventSocket;
    QString m_fps = QStringLiteral("24.000");
    QString m_iso = QStringLiteral("800");
    QString m_shutterAngle = QStringLiteral("180°");
    QString m_shutterSpeed = QStringLiteral("1/48");
    QString m_whiteBalance = QStringLiteral("5600K");
    QString m_resolution = QStringLiteral("1928x1090");
    QString m_recordingFormat = QStringLiteral("cDNG");
    QString m_cdngCompression = QStringLiteral("none");
    QString m_lossyDngRatio = QStringLiteral("3to1");
    QString m_proresProfile = QStringLiteral("422");
    QString m_proresColorProfile = QStringLiteral("standard");
    QString m_cdngPreviewColorProfile = QStringLiteral("standard");
    QVariantMap m_previewExposureReference;
    QString m_tuningFile;
    QString m_rawBitDepth = QStringLiteral("12-bit");
    QString m_h264Resolution = QStringLiteral("Match Capture");
    QString m_sensorName = QStringLiteral("Unknown");
    QString m_autofocusMode = QStringLiteral("Continuous");
    QString m_autofocusRange = QStringLiteral("Normal");
    QString m_autofocusSpeed = QStringLiteral("Normal");
    QString m_autofocusWindow = QStringLiteral("Center");
    double m_afX = 0.25, m_afY = 0.25, m_afWidth = 0.5, m_afHeight = 0.5;
    bool m_isoAuto = false;
    bool m_shutterAuto = false;
    bool m_whiteBalanceAuto = false;
    bool m_whiteBalanceAvailable = false;
    double m_exposureCompensation = 0.0;
    bool m_exposureCompensationAvailable = false;
    QString m_exposureMetering = QStringLiteral("centre-weighted");
    QVariantList m_exposureMeteringOptions;
    bool m_recording = false;
    bool m_recordingHasDroppedFrames = false;
    bool m_standby = false;
    bool m_timelapseActive = false;
    qulonglong m_timelapseShotCount = 0;
    bool m_stillExposureActive = false;
    bool m_mediaWritesPending = false;
    qint64 m_mediaWriteRevision = -1;
    double m_stillExposureRemainingSeconds = 0;
    QElapsedTimer m_stillExposureTimer;
    int m_audioInputLevel = 0;
    bool m_connected = false;
    QString m_lastError;
    QString m_lastRecordingError;
    QString m_lastPhotoWriteError;
    QTimer m_retryTimer;
    int m_nextCommandId = 1;
    int m_pendingStateRequestId = -1;
};
