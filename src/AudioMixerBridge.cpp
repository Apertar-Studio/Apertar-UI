#include "AudioMixerBridge.hpp"

#include "SettingsBridge.hpp"

#include <QProcess>
#include <QRegularExpression>

#include <algorithm>

namespace {

constexpr int kApplyDebounceMs = 120;

struct RankedControl {
    QString name;
    int score = 0;
};

int inputControlScore(const QString &name)
{
    const QString lower = name.toLower();
    if (lower == QStringLiteral("capture"))
        return 100;
    if (lower.contains(QStringLiteral("capture")))
        return 90;
    if (lower == QStringLiteral("mic"))
        return 85;
    if (lower.contains(QStringLiteral("mic")))
        return 80;
    if (lower.contains(QStringLiteral("input")))
        return 70;
    if (lower == QStringLiteral("digital"))
        return 60;
    if (lower.contains(QStringLiteral("digital")))
        return 50;
    if (lower.contains(QStringLiteral("line")))
        return 40;
    return -1;
}

int outputControlScore(const QString &name)
{
    const QString lower = name.toLower();
    if (lower == QStringLiteral("headphone"))
        return 100;
    if (lower.contains(QStringLiteral("headphone")))
        return 95;
    if (lower == QStringLiteral("speaker"))
        return 90;
    if (lower.contains(QStringLiteral("speaker")))
        return 85;
    if (lower == QStringLiteral("pcm"))
        return 80;
    if (lower.contains(QStringLiteral("pcm")))
        return 75;
    if (lower.contains(QStringLiteral("playback")))
        return 70;
    if (lower == QStringLiteral("master"))
        return 65;
    if (lower.contains(QStringLiteral("master")))
        return 60;
    if (lower == QStringLiteral("digital"))
        return 55;
    if (lower.contains(QStringLiteral("digital")))
        return 50;
    return -1;
}

}

AudioMixerBridge::AudioMixerBridge(QObject *parent)
    : QObject(parent)
{
    m_inputApplyTimer.setSingleShot(true);
    m_inputApplyTimer.setInterval(kApplyDebounceMs);
    connect(&m_inputApplyTimer, &QTimer::timeout, this, &AudioMixerBridge::applyInputMixer);

    m_outputApplyTimer.setSingleShot(true);
    m_outputApplyTimer.setInterval(kApplyDebounceMs);
    connect(&m_outputApplyTimer, &QTimer::timeout, this, &AudioMixerBridge::applyOutputMixer);
}

void AudioMixerBridge::setSettingsBridge(SettingsBridge *settingsBridge)
{
    if (m_settingsBridge == settingsBridge)
        return;

    m_settingsBridge = settingsBridge;
    if (m_settingsBridge) {
        connect(m_settingsBridge, &SettingsBridge::audioOutputDeviceChanged, this, &AudioMixerBridge::scheduleOutputApply);
        connect(m_settingsBridge, &SettingsBridge::headphoneVolumeChanged, this, &AudioMixerBridge::scheduleOutputApply);
    }

    scheduleOutputApply();
}

QString AudioMixerBridge::mixerCardForDevice(const QString &deviceId)
{
    static const QRegularExpression pattern(QStringLiteral(R"(^(?:plug)?hw:(\d+)(?:,\d+)?$)"),
                                            QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = pattern.match(deviceId.trimmed());
    if (!match.hasMatch())
        return QString();

    return QStringLiteral("hw:%1").arg(match.captured(1));
}

QStringList AudioMixerBridge::preferredControls(const QStringList &controls, MixerDirection direction)
{
    QList<RankedControl> ranked;
    ranked.reserve(controls.size());

    for (const QString &control : controls) {
        const int score = direction == MixerDirection::Input
            ? inputControlScore(control)
            : outputControlScore(control);
        if (score < 0)
            continue;

        ranked.push_back({ control, score });
    }

    std::sort(ranked.begin(), ranked.end(), [](const RankedControl &left, const RankedControl &right) {
        if (left.score != right.score)
            return left.score > right.score;
        return left.name < right.name;
    });

    QStringList result;
    result.reserve(ranked.size());
    for (const RankedControl &entry : ranked)
        result.push_back(entry.name);
    return result;
}

void AudioMixerBridge::scheduleInputApply()
{
    m_inputApplyTimer.start();
}

void AudioMixerBridge::scheduleOutputApply()
{
    m_outputApplyTimer.start();
}

void AudioMixerBridge::applyInputMixer()
{
    // Input gain is applied in the audio capture path so it always affects
    // recording and metering consistently, even on USB devices with no usable
    // ALSA capture mixer control.
}

void AudioMixerBridge::applyOutputMixer()
{
    if (!m_settingsBridge)
        return;

    const QString cardSpec = mixerCardForDevice(m_settingsBridge->audioOutputDevice());
    if (cardSpec.isEmpty())
        return;

    applyMixerVolume(cardSpec, MixerDirection::Output, m_settingsBridge->headphoneVolume());
}

QStringList AudioMixerBridge::mixerControlsForCard(const QString &cardSpec)
{
    const auto cached = m_cardControlsCache.constFind(cardSpec);
    if (cached != m_cardControlsCache.constEnd())
        return cached.value();

    QProcess process;
    process.start(QStringLiteral("amixer"), {
        QStringLiteral("-D"), cardSpec,
        QStringLiteral("scontrols")
    });
    if (!process.waitForStarted(1200)) {
        m_cardControlsCache.insert(cardSpec, {});
        return {};
    }

    process.waitForFinished(2000);
    const QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    static const QRegularExpression pattern(QStringLiteral(R"(Simple mixer control '([^']+)')"));

    QStringList controls;
    QRegularExpressionMatchIterator it = pattern.globalMatch(output);
    while (it.hasNext())
        controls.push_back(it.next().captured(1).trimmed());

    m_cardControlsCache.insert(cardSpec, controls);
    return controls;
}

bool AudioMixerBridge::applyMixerVolume(const QString &cardSpec,
                                        MixerDirection direction,
                                        int level)
{
    const QStringList controls = preferredControls(mixerControlsForCard(cardSpec), direction);
    if (controls.isEmpty())
        return false;

    const QString percent = QStringLiteral("%1%").arg(qBound(0, level, 100));
    const QString stateArg = direction == MixerDirection::Input
        ? QStringLiteral("cap")
        : (level <= 0 ? QStringLiteral("mute") : QStringLiteral("unmute"));

    for (const QString &control : controls) {
        QProcess process;
        process.start(QStringLiteral("amixer"), {
            QStringLiteral("-D"), cardSpec,
            QStringLiteral("sset"),
            control,
            percent,
            stateArg
        });
        if (!process.waitForStarted(1200))
            continue;
        process.waitForFinished(2000);
        if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0)
            return true;
    }

    return false;
}
