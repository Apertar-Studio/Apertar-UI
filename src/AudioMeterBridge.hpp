#pragma once

#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QString>
#include <QTimer>

class SettingsBridge;
class ApertarControlBridge;

class AudioMeterBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int inputLevel READ inputLevel NOTIFY inputLevelChanged)
    Q_PROPERTY(bool inputDeviceAvailable READ inputDeviceAvailable NOTIFY inputDeviceAvailableChanged)

public:
    explicit AudioMeterBridge(QObject *parent = nullptr);

    int inputLevel() const;
    bool inputDeviceAvailable() const;

    void setSettingsBridge(SettingsBridge *settingsBridge);
    void setControlBridge(ApertarControlBridge *controlBridge);

    Q_INVOKABLE void suspendMonitoring();
    Q_INVOKABLE void resumeMonitoring();
    void releaseInputForRecording();
    void setCapabilityProbeActive(bool active) { m_capabilityProbeActive = active; updateMonitoring(); }

signals:
    void inputLevelChanged();
    void inputDeviceAvailableChanged();

private:
    static QString normalizedMonitorDevice(const QString &deviceId);
    static bool isHdmiOutputDevice(const QString &deviceId);
    static double inputGainFactor(int inputVolume);
    static int meterLevelFromPcm16(const QByteArray &pcmBytes, double gain);
    static void applyGainToPcm16(QByteArray &pcmBytes, double gain);
    static QByteArray duplicateMonoToStereo(const QByteArray &pcmBytes);

    bool meterEnabled() const;
    bool monitorEnabled() const;
    QString selectedInputDevice() const;
    QString selectedOutputDevice() const;
    void updateMonitoring();
    void startMonitorProcess(const QString &deviceId);
    void startPlaybackProcess(const QString &deviceId);
    void stopMonitorProcess(bool clearLevel);
    void stopPlaybackProcess();
    void handleMonitorOutput();
    void handleDecayTick();
    void setInputLevel(int value);
    void updateInputDeviceAvailable();
    void pushMeasuredLevel(int value);

    QPointer<SettingsBridge> m_settingsBridge;
    QPointer<ApertarControlBridge> m_controlBridge;
    QProcess m_monitorProcess;
    QProcess m_playbackProcess;
    QTimer m_decayTimer;
    QTimer m_restartTimer;
    QString m_activeDeviceId;
    QString m_activeOutputDeviceId;
    int m_activeSampleRate = 0;
    int m_activeCaptureChannels = 1;
    int m_firstMonitorChannel = 0;
    int m_lastMonitorChannel = 0;
    QByteArray m_pendingCaptureBytes;
    bool m_activeMonitorPlayback = false;
    bool m_monitorCaptureActive = false;
    bool m_monitoringSuspended = true;
    bool m_capabilityProbeActive = false;
    int m_inputLevel = 0;
    bool m_inputDeviceAvailable = false;
};
