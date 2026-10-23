#include "AudioMeterBridge.hpp"

#include "ApertarControlBridge.hpp"
#include "SettingsBridge.hpp"

#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace {

constexpr int kDecayIntervalMs = 70;
constexpr int kDecayStep = 7;
constexpr int kMeterOnlySampleRate = 16000;
constexpr int kMonitorSampleRate = 48000;
constexpr int kMonitorRestartDelayMs = 1500;

}

AudioMeterBridge::AudioMeterBridge(QObject *parent)
    : QObject(parent)
{
    m_monitorProcess.setProcessChannelMode(QProcess::SeparateChannels);
    m_playbackProcess.setProcessChannelMode(QProcess::SeparateChannels);

    connect(&m_monitorProcess, &QProcess::readyReadStandardOutput, this, &AudioMeterBridge::handleMonitorOutput);
    connect(&m_monitorProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int, QProcess::ExitStatus) {
                m_activeDeviceId.clear();
                m_monitorCaptureActive = false;
                stopPlaybackProcess();
                setInputLevel(0);
                updateInputDeviceAvailable();
                if (m_controlBridge && m_controlBridge->recording())
                    return;

                if (!m_monitoringSuspended
                    && (meterEnabled() || monitorEnabled())
                    && !selectedInputDevice().isEmpty()
                    && !m_restartTimer.isActive()) {
                    m_restartTimer.start();
                }
            });
    connect(&m_playbackProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int, QProcess::ExitStatus) {
                m_activeOutputDeviceId.clear();
                updateInputDeviceAvailable();
                if (!m_monitoringSuspended
                    && (!m_controlBridge || !m_controlBridge->recording())
                    && monitorEnabled()) {
                    QTimer::singleShot(200, this, &AudioMeterBridge::updateMonitoring);
                }
            });

    m_decayTimer.setInterval(kDecayIntervalMs);
    connect(&m_decayTimer, &QTimer::timeout, this, &AudioMeterBridge::handleDecayTick);

    m_restartTimer.setSingleShot(true);
    m_restartTimer.setInterval(kMonitorRestartDelayMs);
    connect(&m_restartTimer, &QTimer::timeout, this, &AudioMeterBridge::updateMonitoring);
}

int AudioMeterBridge::inputLevel() const
{
    return m_inputLevel;
}

bool AudioMeterBridge::inputDeviceAvailable() const
{
    return m_inputDeviceAvailable;
}

void AudioMeterBridge::setSettingsBridge(SettingsBridge *settingsBridge)
{
    if (m_settingsBridge == settingsBridge)
        return;

    m_settingsBridge = settingsBridge;
    if (m_settingsBridge) {
        connect(m_settingsBridge, &SettingsBridge::recordAudioEnabledChanged, this, &AudioMeterBridge::updateMonitoring);
        connect(m_settingsBridge, &SettingsBridge::audioMeterEnabledChanged, this, &AudioMeterBridge::updateMonitoring);
        connect(m_settingsBridge, &SettingsBridge::photoModeEnabledChanged, this, &AudioMeterBridge::updateMonitoring);
        connect(m_settingsBridge, &SettingsBridge::audioInputDeviceChanged, this, &AudioMeterBridge::updateMonitoring);
        connect(m_settingsBridge, &SettingsBridge::audioOutputDeviceChanged, this, &AudioMeterBridge::updateMonitoring);
        connect(m_settingsBridge, &SettingsBridge::audioCaptureOptionsChanged, this, [this]() {
            stopMonitorProcess(true);
            updateMonitoring();
        });
        connect(m_settingsBridge, &SettingsBridge::headphoneVolumeChanged, this, &AudioMeterBridge::updateMonitoring);
        connect(m_settingsBridge, &SettingsBridge::inputVolumeChanged, this, [this]() {
            if (m_settingsBridge && m_settingsBridge->inputVolume() <= 0)
                setInputLevel(0);
        });
    }

    updateInputDeviceAvailable();
    updateMonitoring();
}

void AudioMeterBridge::setControlBridge(ApertarControlBridge *controlBridge)
{
    if (m_controlBridge == controlBridge)
        return;

    m_controlBridge = controlBridge;
    if (m_controlBridge) {
        connect(m_controlBridge, &ApertarControlBridge::recordingChanged, this, [this]() {
            if (m_controlBridge && m_controlBridge->recording())
                m_monitoringSuspended = false;
            updateInputDeviceAvailable();
            updateMonitoring();
        });
        connect(m_controlBridge, &ApertarControlBridge::audioInputLevelChanged, this, [this]() {
            if (m_controlBridge && m_controlBridge->recording())
                pushMeasuredLevel(m_controlBridge->audioInputLevel());
        });
    }

    updateInputDeviceAvailable();
    updateMonitoring();
}

void AudioMeterBridge::suspendMonitoring()
{
    m_monitoringSuspended = true;
    m_restartTimer.stop();
    stopMonitorProcess(true);
    updateInputDeviceAvailable();
}

void AudioMeterBridge::resumeMonitoring()
{
    m_monitoringSuspended = false;
    updateInputDeviceAvailable();
    updateMonitoring();
}

void AudioMeterBridge::releaseInputForRecording()
{
    m_restartTimer.stop();
    m_monitoringSuspended = false;
    stopMonitorProcess(true);
}

QString AudioMeterBridge::normalizedMonitorDevice(const QString &deviceId)
{
    const QString normalized = deviceId.trimmed();
    if (normalized.isEmpty())
        return QString();

    if (normalized.startsWith(QStringLiteral("hw:"), Qt::CaseInsensitive))
        return QStringLiteral("plughw:%1").arg(normalized.mid(3));

    return normalized;
}

bool AudioMeterBridge::isHdmiOutputDevice(const QString &deviceId)
{
    const QString normalized = deviceId.trimmed().toLower();
    return normalized.startsWith(QStringLiteral("hdmi:"))
        || normalized.contains(QStringLiteral("vc4hdmi"))
        || normalized.contains(QStringLiteral("vc4-hdmi"));
}

double AudioMeterBridge::inputGainFactor(int inputVolume)
{
    const int clamped = qBound(0, inputVolume, 100);
    if (clamped <= 0)
        return 0.0;

    constexpr double kUnityReference = 60.0;
    return std::clamp(static_cast<double>(clamped) / kUnityReference, 0.0, 2.0);
}

int AudioMeterBridge::meterLevelFromPcm16(const QByteArray &pcmBytes, double gain)
{
    if (pcmBytes.size() < 2)
        return 0;

    int peak = 0;
    const char *data = pcmBytes.constData();
    for (int offset = 0; offset + 1 < pcmBytes.size(); offset += 2) {
        const uint16_t sampleBits =
            static_cast<uint16_t>(static_cast<unsigned char>(data[offset])) |
            (static_cast<uint16_t>(static_cast<unsigned char>(data[offset + 1])) << 8);
        const int16_t sample = static_cast<int16_t>(sampleBits);
        const int magnitude = sample == std::numeric_limits<int16_t>::min()
            ? std::numeric_limits<int16_t>::max()
            : std::abs(static_cast<int>(sample));
        peak = std::max(peak, magnitude);
    }

    if (peak <= 0)
        return 0;

    const double normalized = std::clamp((static_cast<double>(peak) * std::max(gain, 0.0)) / 32767.0, 0.0, 1.0);
    const double db = 20.0 * std::log10(std::max(normalized, 1.0e-6));
    const double scaled = std::clamp((db + 50.0) / 50.0, 0.0, 1.0);
    return static_cast<int>(std::lround(scaled * 100.0));
}

void AudioMeterBridge::applyGainToPcm16(QByteArray &pcmBytes, double gain)
{
    if (pcmBytes.size() < 2)
        return;

    if (gain <= 0.0) {
        pcmBytes.fill('\0');
        return;
    }

    if (std::fabs(gain - 1.0) < 0.0001)
        return;

    char *data = pcmBytes.data();
    for (int offset = 0; offset + 1 < pcmBytes.size(); offset += 2) {
        const uint16_t sampleBits =
            static_cast<uint16_t>(static_cast<unsigned char>(data[offset])) |
            (static_cast<uint16_t>(static_cast<unsigned char>(data[offset + 1])) << 8);
        const int16_t sample = static_cast<int16_t>(sampleBits);
        const int scaled = static_cast<int>(std::lround(static_cast<double>(sample) * gain));
        const int16_t clamped = static_cast<int16_t>(
            std::clamp(scaled,
                       static_cast<int>(std::numeric_limits<int16_t>::min()),
                       static_cast<int>(std::numeric_limits<int16_t>::max())));
        const uint16_t encoded = static_cast<uint16_t>(clamped);
        data[offset] = static_cast<char>(encoded & 0xff);
        data[offset + 1] = static_cast<char>((encoded >> 8) & 0xff);
    }
}

QByteArray AudioMeterBridge::duplicateMonoToStereo(const QByteArray &pcmBytes)
{
    if (pcmBytes.size() < 2)
        return pcmBytes;

    QByteArray stereo;
    stereo.resize(pcmBytes.size() * 2);
    const char *src = pcmBytes.constData();
    char *dst = stereo.data();

    for (int offset = 0; offset + 1 < pcmBytes.size(); offset += 2) {
        dst[0] = src[offset];
        dst[1] = src[offset + 1];
        dst[2] = src[offset];
        dst[3] = src[offset + 1];
        dst += 4;
    }

    return stereo;
}

bool AudioMeterBridge::meterEnabled() const
{
    return m_settingsBridge
        && m_settingsBridge->recordAudioEnabled()
        && m_settingsBridge->audioMeterEnabled()
        && !m_settingsBridge->photoModeEnabled();
}

bool AudioMeterBridge::monitorEnabled() const
{
    return m_settingsBridge
        && m_settingsBridge->recordAudioEnabled()
        && m_settingsBridge->liveAudioMonitoringEnabled()
        && !m_settingsBridge->photoModeEnabled()
        && !selectedInputDevice().isEmpty()
        && !selectedOutputDevice().isEmpty()
        && m_settingsBridge->headphoneVolume() > 0;
}

QString AudioMeterBridge::selectedInputDevice() const
{
    return m_settingsBridge ? normalizedMonitorDevice(m_settingsBridge->audioInputDevice()) : QString();
}

QString AudioMeterBridge::selectedOutputDevice() const
{
    return m_settingsBridge ? normalizedMonitorDevice(m_settingsBridge->audioOutputDevice()) : QString();
}

void AudioMeterBridge::updateMonitoring()
{
    if (m_capabilityProbeActive) { stopMonitorProcess(true); return; }
    if (!(meterEnabled() || monitorEnabled())) {
        m_restartTimer.stop();
        stopMonitorProcess(true);
        updateInputDeviceAvailable();
        return;
    }

    if (m_controlBridge && m_controlBridge->recording()) {
        m_restartTimer.stop();
        stopMonitorProcess(!meterEnabled());
        pushMeasuredLevel(m_controlBridge->audioInputLevel());
        updateInputDeviceAvailable();
        return;
    }

    if (m_monitoringSuspended) {
        m_restartTimer.stop();
        stopMonitorProcess(true);
        updateInputDeviceAvailable();
        return;
    }

    const QString deviceId = selectedInputDevice();
    if (deviceId.isEmpty()) {
        m_restartTimer.stop();
        stopMonitorProcess(true);
        updateInputDeviceAvailable();
        return;
    }

    const bool wantPlaybackMonitor = monitorEnabled();
    const int desiredSampleRate = wantPlaybackMonitor ? kMonitorSampleRate : kMeterOnlySampleRate;

    if (m_monitorProcess.state() == QProcess::Running
        && m_activeDeviceId == deviceId
        && m_activeMonitorPlayback == wantPlaybackMonitor
        && m_activeSampleRate == desiredSampleRate)
    {
        if (wantPlaybackMonitor)
            startPlaybackProcess(selectedOutputDevice());
        else
            stopPlaybackProcess();
        updateInputDeviceAvailable();
        return;
    }

    startMonitorProcess(deviceId);
    updateInputDeviceAvailable();
}

void AudioMeterBridge::startMonitorProcess(const QString &deviceId)
{
    m_restartTimer.stop();
    stopMonitorProcess(false);

    const bool wantPlaybackMonitor = monitorEnabled();
    const int sampleRate = wantPlaybackMonitor ? kMonitorSampleRate : kMeterOnlySampleRate;

    m_activeDeviceId = deviceId;
    m_activeMonitorPlayback = wantPlaybackMonitor;
    m_activeSampleRate = sampleRate;
    const QString selection = m_settingsBridge ? m_settingsBridge->audioChannels() : "auto";
    m_firstMonitorChannel = qMax(0, selection.section('-', 0, 0).toInt() - 1);
    m_lastMonitorChannel = selection.contains('-') ? selection.section('-', 1, 1).toInt() - 1 : m_firstMonitorChannel;
    m_lastMonitorChannel = qBound(m_firstMonitorChannel, m_lastMonitorChannel, 31);
    m_activeCaptureChannels = m_lastMonitorChannel + 1;
    m_pendingCaptureBytes.clear();
    m_monitorCaptureActive = false;
    m_monitorProcess.start(QStringLiteral("arecord"), {
        QStringLiteral("-q"),
        QStringLiteral("-D"), deviceId,
        QStringLiteral("-f"), QStringLiteral("S16_LE"),
        QStringLiteral("-c"), QString::number(m_activeCaptureChannels),
        QStringLiteral("-r"), QString::number(sampleRate),
        QStringLiteral("-t"), QStringLiteral("raw")
    });

    if (wantPlaybackMonitor)
        startPlaybackProcess(selectedOutputDevice());
    else
        stopPlaybackProcess();

    updateInputDeviceAvailable();
}

void AudioMeterBridge::startPlaybackProcess(const QString &deviceId)
{
    if (deviceId.isEmpty()) {
        stopPlaybackProcess();
        return;
    }

    if (m_playbackProcess.state() == QProcess::Running && m_activeOutputDeviceId == deviceId)
        return;

    stopPlaybackProcess();
    m_activeOutputDeviceId = deviceId;
    const bool hdmiOutput = isHdmiOutputDevice(deviceId);
    m_playbackProcess.start(QStringLiteral("aplay"), {
        QStringLiteral("-q"),
        QStringLiteral("-D"), deviceId,
        QStringLiteral("-f"), QStringLiteral("S16_LE"),
        QStringLiteral("-c"), hdmiOutput ? QStringLiteral("2") : QStringLiteral("1"),
        QStringLiteral("-r"), QStringLiteral("48000"),
        QStringLiteral("-t"), QStringLiteral("raw")
    });
}

void AudioMeterBridge::stopMonitorProcess(bool clearLevel)
{
    m_restartTimer.stop();
    if (m_monitorProcess.state() != QProcess::NotRunning) {
        m_monitorProcess.blockSignals(true);
        m_monitorProcess.terminate();
        if (!m_monitorProcess.waitForFinished(150))
            m_monitorProcess.kill();
        m_monitorProcess.waitForFinished(150);
        m_monitorProcess.blockSignals(false);
    }

    m_activeDeviceId.clear();
    m_activeSampleRate = 0;
    m_activeMonitorPlayback = false;
    m_monitorCaptureActive = false;
    stopPlaybackProcess();
    if (clearLevel)
        setInputLevel(0);
    updateInputDeviceAvailable();
}

void AudioMeterBridge::stopPlaybackProcess()
{
    if (m_playbackProcess.state() != QProcess::NotRunning) {
        m_playbackProcess.blockSignals(true);
        m_playbackProcess.closeWriteChannel();
        m_playbackProcess.terminate();
        if (!m_playbackProcess.waitForFinished(150))
            m_playbackProcess.kill();
        m_playbackProcess.waitForFinished(150);
        m_playbackProcess.blockSignals(false);
    }

    m_activeOutputDeviceId.clear();
}

void AudioMeterBridge::handleMonitorOutput()
{
    m_pendingCaptureBytes += m_monitorProcess.readAllStandardOutput();
    const int frameBytes = m_activeCaptureChannels * 2;
    const int frames = m_pendingCaptureBytes.size() / frameBytes;
    QByteArray pcmBytes(frames * 2, '\0');
    for (int frame = 0; frame < frames; ++frame) {
        int sum = 0;
        for (int ch = m_firstMonitorChannel; ch <= m_lastMonitorChannel; ++ch) {
            const auto *p = reinterpret_cast<const unsigned char *>(m_pendingCaptureBytes.constData() + frame * frameBytes + ch * 2);
            sum += int16_t(uint16_t(p[0]) | (uint16_t(p[1]) << 8));
        }
        const uint16_t sample = uint16_t(int16_t(sum / (m_lastMonitorChannel - m_firstMonitorChannel + 1)));
        pcmBytes[frame * 2] = char(sample); pcmBytes[frame * 2 + 1] = char(sample >> 8);
    }
    m_pendingCaptureBytes.remove(0, frames * frameBytes);
    if (!pcmBytes.isEmpty() && !m_monitorCaptureActive) {
        m_monitorCaptureActive = true;
        updateInputDeviceAvailable();
    }
    const int inputVolume = m_settingsBridge ? m_settingsBridge->inputVolume() : 60;
    const double gain = inputGainFactor(inputVolume);
    applyGainToPcm16(pcmBytes, gain);
    pushMeasuredLevel(meterLevelFromPcm16(pcmBytes, 1.0));

    if (m_playbackProcess.state() == QProcess::Running && monitorEnabled()) {
        const QByteArray playbackBytes = isHdmiOutputDevice(m_activeOutputDeviceId)
            ? duplicateMonoToStereo(pcmBytes)
            : pcmBytes;
        m_playbackProcess.write(playbackBytes);
    }
}

void AudioMeterBridge::handleDecayTick()
{
    if (m_inputLevel <= 0) {
        m_decayTimer.stop();
        return;
    }

    setInputLevel(std::max(0, m_inputLevel - kDecayStep));
}

void AudioMeterBridge::setInputLevel(int value)
{
    const int clamped = qBound(0, value, 100);
    if (m_inputLevel == clamped)
        return;

    m_inputLevel = clamped;
    if (m_inputLevel > 0 && !m_decayTimer.isActive())
        m_decayTimer.start();
    else if (m_inputLevel == 0)
        m_decayTimer.stop();
    emit inputLevelChanged();
}

void AudioMeterBridge::updateInputDeviceAvailable()
{
    const bool available =
        !selectedInputDevice().isEmpty()
        && ((m_controlBridge && m_controlBridge->recording())
            || m_monitorCaptureActive);

    if (m_inputDeviceAvailable == available)
        return;

    m_inputDeviceAvailable = available;
    emit inputDeviceAvailableChanged();
}

void AudioMeterBridge::pushMeasuredLevel(int value)
{
    const int clamped = qBound(0, value, 100);
    if (clamped >= m_inputLevel) {
        setInputLevel(clamped);
        return;
    }

    setInputLevel((m_inputLevel * 3 + clamped) / 4);
}
