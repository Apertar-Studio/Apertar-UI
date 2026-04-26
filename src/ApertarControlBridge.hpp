#pragma once

#include <QObject>
#include <QJsonObject>
#include <QSettings>
#include <QString>
#include <QTimer>

class ApertarControlBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString fps READ fps NOTIFY fpsChanged)
    Q_PROPERTY(QString iso READ iso NOTIFY isoChanged)
    Q_PROPERTY(QString shutterAngle READ shutterAngle NOTIFY shutterAngleChanged)
    Q_PROPERTY(QString shutterSpeed READ shutterSpeed NOTIFY shutterSpeedChanged)
    Q_PROPERTY(QString whiteBalance READ whiteBalance NOTIFY whiteBalanceChanged)
    Q_PROPERTY(QString resolution READ resolution NOTIFY resolutionChanged)
    Q_PROPERTY(bool recording READ recording NOTIFY recordingChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit ApertarControlBridge(QObject *parent = nullptr);

    QString fps() const;
    QString iso() const;
    QString shutterAngle() const;
    QString shutterSpeed() const;
    QString whiteBalance() const;
    QString resolution() const;
    bool recording() const;
    bool connected() const;
    QString lastError() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool applyFps(const QString &displayValue);
    Q_INVOKABLE bool applyIso(const QString &displayValue);
    Q_INVOKABLE bool applyShutterAngle(const QString &displayValue);
    Q_INVOKABLE bool applyShutterSpeed(const QString &displayValue);
    Q_INVOKABLE bool applyWhiteBalance(const QString &displayValue);
    Q_INVOKABLE bool applyResolution(const QString &displayValue);
    Q_INVOKABLE bool capturePhoto();
    Q_INVOKABLE bool setRecording(bool recording);

signals:
    void fpsChanged();
    void isoChanged();
    void shutterAngleChanged();
    void shutterSpeedChanged();
    void whiteBalanceChanged();
    void resolutionChanged();
    void recordingChanged();
    void connectedChanged();
    void lastErrorChanged();

private:
    void refreshFromCore();
    void loadPersistedCameraSettings();
    void saveCurrentCameraSettings(int shutterUsOverride = -1);
    bool syncPersistedCameraSettingsToCore();
    bool sendCoreCommand(const QString &commandName, const QJsonObject &arguments = {}, QJsonObject *replyObject = nullptr);
    void applyCoreMessage(const QJsonObject &message);
    void applyCoreState(const QJsonObject &state);
    void applyShutterFromMicroseconds(double shutterUs, double fps);
    int shutterUsForAngle(double angle, double fps) const;
    int shutterUsForSpeed(double denominator) const;
    double currentFpsValue() const;

    QString formatFpsValue(double value) const;
    QString formatIsoValue(double value) const;
    QString formatShutterAngleValue(double value) const;
    QString formatShutterSpeedValue(double value) const;
    QString formatResolutionValue(int width, int height) const;

    double parseFpsValue(const QString &displayValue) const;
    double parseIsoValue(const QString &displayValue) const;
    double parseShutterAngleValue(const QString &displayValue) const;
    double parseShutterSpeedValue(const QString &displayValue) const;
    double parseWhiteBalanceValue(const QString &displayValue) const;
    bool parseResolutionValue(const QString &displayValue, int *width, int *height) const;

    QString compactNumber(double value, int decimals = 3) const;
    void setFps(const QString &value);
    void setIso(const QString &value);
    void setShutterAngle(const QString &value);
    void setShutterSpeed(const QString &value);
    void setWhiteBalance(const QString &value);
    void setResolution(const QString &value);
    void setRecordingState(bool value);
    void setConnected(bool value);
    void setLastError(const QString &value);

    QSettings m_settings;
    QString m_fps = QStringLiteral("24.000");
    QString m_iso = QStringLiteral("800");
    QString m_shutterAngle = QStringLiteral("180°");
    QString m_shutterSpeed = QStringLiteral("1/48");
    QString m_whiteBalance = QStringLiteral("5600K");
    QString m_resolution = QStringLiteral("1928x1090");
    bool m_isoAuto = false;
    bool m_shutterAuto = false;
    bool m_recording = false;
    bool m_connected = false;
    QString m_lastError;
    QTimer m_retryTimer;
    int m_nextCommandId = 1;
    bool m_pendingInitialSettingsSync = false;
    bool m_syncingPersistedSettings = false;
};
