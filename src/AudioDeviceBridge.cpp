#include "AudioDeviceBridge.hpp"
#include "SettingsBridge.hpp"

#include <algorithm>

#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
constexpr int kAudioRefreshIntervalMs = 2000;
constexpr int kAudioBackgroundRefreshIntervalMs = 30000;
const QString kNoAudioInputDeviceText = QStringLiteral("No audio input device detected");
const QString kNoAudioOutputDeviceText = QStringLiteral("None");
const QString kNoneAudioOutputText = QStringLiteral("None");

struct ParsedAudioDevice {
    QString id;
    QString label;
    QString displayLabel;
    bool usb = false;
    bool hdmi = false;
};

QString readTrimmedFirstLine(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    return QString::fromLocal8Bit(file.readLine()).trimmed();
}

QString runAudioDeviceListTool(const QString &tool)
{
    QProcess process;
    process.setProgram(tool);
    process.setArguments({ QStringLiteral("-l") });
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start();
    if (!process.waitForStarted(500))
        return QString();
    if (!process.waitForFinished(2000))
        return QString();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return QString();
    return QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
}

QString preferredHardwareAudioLabel(const QString &cardLabel,
                                    const QString &deviceLabel,
                                    const QString &cardId)
{
    const QString trimmedCardLabel = cardLabel.trimmed();
    const QString trimmedDeviceLabel = deviceLabel.trimmed();
    const QString trimmedCardId = cardId.trimmed();

    auto isGeneric = [](const QString &value) {
        const QString lower = value.trimmed().toLower();
        return lower.isEmpty()
            || lower == QStringLiteral("usb audio")
            || lower == QStringLiteral("audio")
            || lower.contains(QStringLiteral("playback"))
            || lower.contains(QStringLiteral("capture"));
    };

    if (!trimmedCardLabel.isEmpty() && !isGeneric(trimmedCardLabel))
        return trimmedCardLabel;
    if (!trimmedDeviceLabel.isEmpty())
        return trimmedDeviceLabel;
    if (!trimmedCardLabel.isEmpty())
        return trimmedCardLabel;
    return trimmedCardId;
}

QList<ParsedAudioDevice> parseAlsaHardwareDeviceList(const QString &text, bool outputMode)
{
    static const QRegularExpression pattern(
        QStringLiteral(R"(^\s*card\s+(\d+):\s*([^\s\[]+)\s*\[(.*?)\],\s*device\s+(\d+):\s*(.*?)\s*\[(.*?)\]\s*$)"),
        QRegularExpression::MultilineOption);

    QList<ParsedAudioDevice> devices;
    QRegularExpressionMatchIterator it = pattern.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString cardNumber = match.captured(1).trimmed();
        const QString cardId = match.captured(2).trimmed();
        const QString cardLabel = match.captured(3).trimmed();
        const QString deviceNumber = match.captured(4).trimmed();
        const QString devicePrimaryLabel = match.captured(5).trimmed();
        const QString deviceLabel = match.captured(6).trimmed();
        const QString combined = QStringList{ cardId, cardLabel, devicePrimaryLabel, deviceLabel }
                                     .join(QLatin1Char(' '))
                                     .trimmed();

        ParsedAudioDevice device;
        device.usb = combined.contains(QStringLiteral("usb"), Qt::CaseInsensitive);
        device.hdmi = combined.contains(QStringLiteral("hdmi"), Qt::CaseInsensitive)
                      || combined.contains(QStringLiteral("vc4"), Qt::CaseInsensitive);
        device.label = preferredHardwareAudioLabel(cardLabel, deviceLabel, cardId);
        if (device.label.isEmpty())
            device.label = preferredHardwareAudioLabel(cardLabel, devicePrimaryLabel, cardId);

        if (outputMode && device.hdmi && !cardId.isEmpty())
            device.id = QStringLiteral("hdmi:CARD=%1,DEV=%2").arg(cardId, deviceNumber);
        else
            device.id = QStringLiteral("hw:%1,%2").arg(cardNumber, deviceNumber);

        devices.push_back(device);
    }

    return devices;
}

QList<ParsedAudioDevice> parseAlsaDeviceList(const QString &text, bool outputMode)
{
    static const QRegularExpression pattern(
        QStringLiteral(R"(^\s*(\d+)-(\d+):\s*(.*?)\s*:\s*(.*?)\s*:\s*(playback|capture)\s+\d+\s*$)"),
        QRegularExpression::MultilineOption);

    QList<ParsedAudioDevice> devices;
    QHash<QString, QString> cardIds;
    QRegularExpressionMatchIterator it = pattern.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString direction = match.captured(5).trimmed().toLower();
        const bool isPlayback = direction == QStringLiteral("playback");
        if (isPlayback != outputMode)
            continue;

        const QString cardNumber = match.captured(1).trimmed();
        const QString deviceNumber = match.captured(2).trimmed();
        const QString primaryName = match.captured(3).trimmed();
        const QString secondaryName = match.captured(4).trimmed();
        QString cardId = cardIds.value(cardNumber);
        if (cardId.isEmpty()) {
            cardId = readTrimmedFirstLine(QStringLiteral("/proc/asound/card%1/id").arg(cardNumber));
            cardIds.insert(cardNumber, cardId);
        }
        const QString combined = QStringList{cardId, primaryName, secondaryName}.join(QLatin1Char(' ')).trimmed();

        ParsedAudioDevice device;
        device.usb = combined.contains(QStringLiteral("usb"), Qt::CaseInsensitive);
        device.hdmi = combined.contains(QStringLiteral("hdmi"), Qt::CaseInsensitive)
                      || combined.contains(QStringLiteral("vc4"), Qt::CaseInsensitive);
        device.label = !secondaryName.isEmpty()
            ? secondaryName
            : (!primaryName.isEmpty() ? primaryName : cardId);

        if (outputMode && device.hdmi && !cardId.isEmpty())
            device.id = QStringLiteral("hdmi:CARD=%1,DEV=%2").arg(cardId, deviceNumber);
        else
            device.id = QStringLiteral("hw:%1,%2").arg(cardNumber, deviceNumber);

        devices.push_back(device);
    }

    return devices;
}

QString friendlyOutputLabel(const ParsedAudioDevice &device)
{
    if (!device.hdmi)
        return device.label;

    const QString combined = (device.id + QLatin1Char(' ') + device.label).toLower();
    if (combined.contains(QStringLiteral("hdmi1")) || combined.contains(QStringLiteral("hdmi-1")))
        return QStringLiteral("HDMI 2");
    if (combined.contains(QStringLiteral("hdmi0")) || combined.contains(QStringLiteral("hdmi-0")))
        return QStringLiteral("HDMI 1");
    return QStringLiteral("HDMI Output");
}

AudioDeviceBridge::DeviceSnapshot formatDeviceOptions(const QList<ParsedAudioDevice> &devices, bool outputMode)
{
    AudioDeviceBridge::DeviceSnapshot snapshot;
    if (outputMode) {
        snapshot.labels.push_back(kNoneAudioOutputText);
        snapshot.labelToId.insert(kNoneAudioOutputText, QString());
        snapshot.idToLabel.insert(QString(), kNoneAudioOutputText);
    }

    QList<ParsedAudioDevice> filtered;
    QSet<QString> seenIds;
    for (const ParsedAudioDevice &device : devices) {
        const bool keep = device.usb;
        if (!keep)
            continue;
        if (seenIds.contains(device.id))
            continue;
        seenIds.insert(device.id);
        ParsedAudioDevice formatted = device;
        if (outputMode)
            formatted.label = friendlyOutputLabel(device);
        filtered.push_back(formatted);
    }

    QHash<QString, int> labelCounts;
    for (const ParsedAudioDevice &device : filtered)
        labelCounts[device.label] += 1;

    QHash<QString, int> labelIndex;
    for (ParsedAudioDevice &device : filtered) {
        const int total = labelCounts.value(device.label);
        const int index = ++labelIndex[device.label];
        device.displayLabel = total > 1
            ? QStringLiteral("%1 (%2)").arg(device.label).arg(index)
            : device.label;

        snapshot.labels.push_back(device.displayLabel);
        snapshot.labelToId.insert(device.displayLabel, device.id);
        snapshot.idToLabel.insert(device.id, device.displayLabel);
    }

    return snapshot;
}

QList<ParsedAudioDevice> mergeOutputDevices(const QList<ParsedAudioDevice> &baseDevices,
                                            const QList<ParsedAudioDevice> &namedDevices)
{
    QList<ParsedAudioDevice> merged = baseDevices;
    QSet<QString> seenIds;
    QSet<QString> seenHdmiKeys;

    for (const ParsedAudioDevice &device : merged) {
        seenIds.insert(device.id);
        if (device.hdmi)
            seenHdmiKeys.insert(friendlyOutputLabel(device).toLower());
    }

    for (const ParsedAudioDevice &device : namedDevices) {
        if (seenIds.contains(device.id))
            continue;
        if (device.hdmi && seenHdmiKeys.contains(friendlyOutputLabel(device).toLower()))
            continue;

        merged.push_back(device);
        seenIds.insert(device.id);
        if (device.hdmi)
            seenHdmiKeys.insert(friendlyOutputLabel(device).toLower());
    }

    return merged;
}

} // namespace

AudioDeviceBridge::AudioDeviceBridge(QObject *parent)
    : QObject(parent)
{
    connect(&m_refreshTimer, &QTimer::timeout, this, &AudioDeviceBridge::refresh);
    m_hotplugRefreshTimer.setSingleShot(true);
    m_hotplugRefreshTimer.setInterval(150);
    connect(&m_hotplugRefreshTimer, &QTimer::timeout, this, &AudioDeviceBridge::refresh);
    connect(&m_deviceWatcher, &QFileSystemWatcher::directoryChanged,
            this, [this]() { m_hotplugRefreshTimer.start(); });
    for (const QString &path : {QStringLiteral("/dev/snd"), QStringLiteral("/proc/asound")}) {
        if (QFileInfo(path).isDir())
            m_deviceWatcher.addPath(path);
    }
    applyPollingMode();
    m_refreshTimer.start();
    QTimer::singleShot(2000, this, &AudioDeviceBridge::refresh);
}

QStringList AudioDeviceBridge::inputDeviceOptions() const
{
    return m_inputDeviceOptions;
}

QStringList AudioDeviceBridge::outputDeviceOptions() const
{
    return m_outputDeviceOptions;
}

bool AudioDeviceBridge::hasInputDevices() const
{
    return !m_inputDeviceOptions.isEmpty();
}

bool AudioDeviceBridge::hasOutputDevices() const
{
    return !m_outputDeviceOptions.isEmpty();
}

void AudioDeviceBridge::setSettingsBridge(SettingsBridge *settingsBridge)
{
    if (m_settingsBridge == settingsBridge)
        return;

    m_settingsBridge = settingsBridge;
    if (settingsBridge) {
        connect(settingsBridge, &SettingsBridge::audioCaptureOptionsChanged, this, &AudioDeviceBridge::captureOptionsChanged);
        connect(settingsBridge, &SettingsBridge::audioInputDeviceChanged, this, [this]() {
            m_captureModes = {};
            emit captureOptionsChanged();
        });
    }
}

namespace {
const QStringList audioFormats{"auto", "s16", "s24", "s32", "float32"};
const QStringList audioFormatLabels{"Auto", "16-bit PCM", "24-bit PCM", "32-bit PCM", "32-bit float (native)"};
QString formatLabel(const QString &format) { return audioFormatLabels.value(audioFormats.indexOf(format), "Auto"); }
QString rateLabel(int rate) { return rate ? QString::number(rate / 1000.0, 'g', 5) + " kHz" : "Auto"; }
QString channelLabel(const QString &value) {
    if (value == "all") return "All inputs";
    if (value == "auto") return "Auto";
    return (value.contains('-') ? "Inputs " : "Input ") + value;
}
}

void AudioDeviceBridge::refreshCaptureCapabilities()
{
    if (m_capabilitiesBusy || !m_settingsBridge) return;
    m_capabilityDevice = normalizeInputSelection(m_settingsBridge->audioInputDevice());
    m_captureModes = {};
    m_capabilityError.clear();
    if (m_capabilityDevice.isEmpty()) { emit captureOptionsChanged(); return; }
    m_capabilitiesBusy = true;
    emit captureOptionsChanged();
    emit capabilityProbeActiveChanged(true);
    auto *socket = new QLocalSocket(this);
    m_capabilitySocket = socket;
    connect(socket, &QLocalSocket::connected, this, [this, socket]() {
        socket->write(QJsonDocument(QJsonObject{{"cmd", "audio_capabilities"}, {"id", 1},
            {"device", m_capabilityDevice}}).toJson(QJsonDocument::Compact) + '\n');
    });
    connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
        if (!socket->canReadLine()) return;
        const auto reply = QJsonDocument::fromJson(socket->readLine()).object();
        if (reply.value("ok").toBool() && m_settingsBridge &&
            m_capabilityDevice == normalizeInputSelection(m_settingsBridge->audioInputDevice()))
            m_captureModes = reply.value("modes").toArray();
        finishCapabilityProbe(reply.value("ok").toBool() ? QString() : reply.value("error").toString());
    });
    connect(socket, &QLocalSocket::errorOccurred, this, [this, socket]() {
        if (m_capabilitySocket == socket) finishCapabilityProbe(socket->errorString());
    });
    QTimer::singleShot(4000, socket, [this, socket]() {
        if (m_capabilitySocket == socket) finishCapabilityProbe("Audio capability check timed out. Stop recording and retry.");
    });
    socket->connectToServer(qEnvironmentVariable("APERTAR_CONTROL_SOCKET", "/tmp/apertar-core.sock"));
}

void AudioDeviceBridge::finishCapabilityProbe(const QString &error)
{
    m_capabilityError = error;
    if (m_capabilitySocket) { m_capabilitySocket->disconnect(this); m_capabilitySocket->deleteLater(); m_capabilitySocket = nullptr; }
    m_capabilitiesBusy = false;
    emit captureOptionsChanged();
    emit capabilityProbeActiveChanged(false);
    if (m_settingsBridge && m_capabilityDevice != normalizeInputSelection(m_settingsBridge->audioInputDevice()))
        QTimer::singleShot(0, this, &AudioDeviceBridge::refreshCaptureCapabilities);
}

QStringList AudioDeviceBridge::sampleFormatOptions() const
{
    QStringList labels{"Auto"};
    for (int i = 1; i < audioFormats.size(); ++i) for (const auto &value : m_captureModes)
        if (value.toObject().value("format").toString() == audioFormats[i]) { labels << audioFormatLabels[i]; break; }
    return labels;
}

QStringList AudioDeviceBridge::sampleRateOptions() const
{
    QSet<int> rates;
    const QString format = m_settingsBridge ? m_settingsBridge->audioSampleFormat() : "auto";
    for (const auto &value : m_captureModes) {
        const auto mode = value.toObject();
        if (format == "auto" || format == mode.value("format").toString()) rates.insert(mode.value("rate").toInt());
    }
    auto sorted = rates.values(); std::sort(sorted.begin(), sorted.end());
    QStringList labels{"Auto"};
    for (int rate : sorted) labels << rateLabel(rate);
    return labels;
}

int AudioDeviceBridge::maximumChannels() const
{
    int maximum = 0;
    for (const auto &value : m_captureModes) {
        const auto mode = value.toObject();
        if (m_settingsBridge && (m_settingsBridge->audioSampleFormat() == "auto" || mode.value("format").toString() == m_settingsBridge->audioSampleFormat())
            && (!m_settingsBridge->audioSampleRate() || mode.value("rate").toInt() == m_settingsBridge->audioSampleRate()))
            maximum = std::max(maximum, mode.value("channels").toInt());
    }
    return maximum;
}

QStringList AudioDeviceBridge::channelOptions() const
{
    QStringList labels{"Auto"};
    const int maximum = maximumChannels();
    for (int ch = 1; ch <= maximum; ++ch) labels << channelLabel(QString::number(ch));
    for (int ch = 1; ch < maximum; ch += 2) labels << channelLabel(QString::number(ch) + "-" + QString::number(ch + 1));
    if (maximum > 2) labels << "All inputs";
    return labels;
}
QString AudioDeviceBridge::sampleFormatLabel() const { return formatLabel(m_settingsBridge ? m_settingsBridge->audioSampleFormat() : "auto"); }
QString AudioDeviceBridge::sampleRateLabel() const { return rateLabel(m_settingsBridge ? m_settingsBridge->audioSampleRate() : 0); }
QString AudioDeviceBridge::channelsLabel() const { return channelLabel(m_settingsBridge ? m_settingsBridge->audioChannels() : "auto"); }
QString AudioDeviceBridge::captureDescription() const
{
    if (m_capabilitiesBusy) return "Checking native input capabilities...";
    if (!m_capabilityError.isEmpty()) return m_capabilityError;
    if (m_captureModes.isEmpty()) return "Refresh to check available input formats.";
    if (m_settingsBridge && m_settingsBridge->audioSampleRate() && m_settingsBridge->audioSampleFormat() != "auto") {
        const auto selected = m_settingsBridge->audioChannels();
        const int channels = selected == "all" ? maximumChannels() : selected == "auto" ? qMin(2, maximumChannels())
            : selected.contains('-') ? selected.section('-', 1, 1).toInt() - selected.section('-', 0, 0).toInt() + 1 : 1;
        const int bits = m_settingsBridge->audioSampleFormat() == "s16" ? 16 : m_settingsBridge->audioSampleFormat() == "s24" ? 24 : 32;
        return QStringLiteral("PCM bitrate: %1 kbit/s. Native device support only.").arg(m_settingsBridge->audioSampleRate() * channels * bits / 1000.0);
    }
    return "Auto prefers native 48 kHz / 16-bit, up to two channels. Float requires device support.";
}
void AudioDeviceBridge::selectSampleFormat(const QString &label)
{
    if (!m_settingsBridge || !sampleFormatOptions().contains(label)) return;
    m_settingsBridge->setAudioSampleFormat(audioFormats.value(audioFormatLabels.indexOf(label), "auto"));
    if (!sampleRateOptions().contains(sampleRateLabel())) m_settingsBridge->setAudioSampleRate(0);
    if (!channelOptions().contains(channelsLabel())) m_settingsBridge->setAudioChannels("auto");
}
void AudioDeviceBridge::selectSampleRate(const QString &label)
{
    if (!m_settingsBridge || !sampleRateOptions().contains(label)) return;
    m_settingsBridge->setAudioSampleRate(label == "Auto" ? 0 : qRound(label.section(' ', 0, 0).toDouble() * 1000));
    if (!channelOptions().contains(channelsLabel())) m_settingsBridge->setAudioChannels("auto");
}
void AudioDeviceBridge::selectChannels(const QString &label)
{
    if (!m_settingsBridge || !channelOptions().contains(label)) return;
    m_settingsBridge->setAudioChannels(label == "Auto" ? "auto" : label == "All inputs" ? "all" : label.section(' ', 1));
}

void AudioDeviceBridge::setPollingEnabled(bool enabled)
{
    if (m_pollingEnabled == enabled)
        return;

    m_pollingEnabled = enabled;
    applyPollingMode();
    if (m_pollingEnabled)
        refresh();
}

void AudioDeviceBridge::refresh()
{
    setInputDevices(detectDevices(QStringLiteral("arecord")));
    setOutputDevices(detectDevices(QStringLiteral("aplay")));
}

QString AudioDeviceBridge::resolvedInputDeviceLabel(const QString &selection) const
{
    return resolvedDeviceLabel(m_inputDeviceOptions, m_inputIdToLabel, selection, kNoAudioInputDeviceText);
}

QString AudioDeviceBridge::resolvedOutputDeviceLabel(const QString &selection) const
{
    return resolvedDeviceLabel(m_outputDeviceOptions, m_outputIdToLabel, selection, kNoAudioOutputDeviceText);
}

QString AudioDeviceBridge::normalizeInputSelection(const QString &selection) const
{
    return normalizedSelection(m_inputDeviceOptions, m_inputLabelToId, m_inputIdToLabel, selection);
}

QString AudioDeviceBridge::normalizeOutputSelection(const QString &selection) const
{
    const QString normalized = selection.trimmed();
    if (normalized.isEmpty())
        return QString();
    if (normalized.compare(kNoneAudioOutputText, Qt::CaseInsensitive) == 0)
        return QString();
    return normalizedSelection(m_outputDeviceOptions, m_outputLabelToId, m_outputIdToLabel, selection);
}

QString AudioDeviceBridge::inputDeviceIdForLabel(const QString &label) const
{
    return deviceIdForLabel(m_inputLabelToId, label);
}

QString AudioDeviceBridge::outputDeviceIdForLabel(const QString &label) const
{
    if (label.trimmed().compare(kNoneAudioOutputText, Qt::CaseInsensitive) == 0)
        return QString();
    return deviceIdForLabel(m_outputLabelToId, label);
}

AudioDeviceBridge::DeviceSnapshot AudioDeviceBridge::detectDevices(const QString &tool) const
{
    const bool outputMode = tool == QStringLiteral("aplay");
    const QString hardwareList = runAudioDeviceListTool(tool);
    if (!hardwareList.isEmpty()) {
        const QList<ParsedAudioDevice> devices = parseAlsaHardwareDeviceList(hardwareList, outputMode);
        if (!devices.isEmpty())
            return formatDeviceOptions(devices, outputMode);
    }

    QFile pcmFile(QStringLiteral("/proc/asound/pcm"));
    if (!pcmFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return DeviceSnapshot{};

    const QString output = QString::fromLocal8Bit(pcmFile.readAll());
    const QList<ParsedAudioDevice> devices = parseAlsaDeviceList(output, outputMode);
    return formatDeviceOptions(devices, outputMode);
}

void AudioDeviceBridge::setInputDevices(const DeviceSnapshot &snapshot)
{
    const QHash<QString, QString> previousIdToLabel = m_inputIdToLabel;
    const QHash<QString, QString> nextIdToLabel = snapshot.idToLabel.isEmpty()
        ? m_inputIdToLabel
        : snapshot.idToLabel;
    if (m_inputDeviceOptions == snapshot.labels
        && m_inputLabelToId == snapshot.labelToId
        && m_inputIdToLabel == nextIdToLabel) {
        return;
    }

    m_inputDeviceOptions = snapshot.labels;
    m_inputLabelToId = snapshot.labelToId;
    m_inputIdToLabel = nextIdToLabel;
    if (m_settingsBridge) {
        const QString remapped = remapSelection(m_settingsBridge->audioInputDevice(),
                                                snapshot,
                                                previousIdToLabel,
                                                false);
        if (remapped != m_settingsBridge->audioInputDevice())
            m_settingsBridge->setAudioInputDevice(remapped);
    }
    emit inputDeviceOptionsChanged();
}

void AudioDeviceBridge::setOutputDevices(const DeviceSnapshot &snapshot)
{
    const QHash<QString, QString> previousIdToLabel = m_outputIdToLabel;
    const QHash<QString, QString> nextIdToLabel = snapshot.idToLabel.isEmpty()
        ? m_outputIdToLabel
        : snapshot.idToLabel;
    if (m_outputDeviceOptions == snapshot.labels
        && m_outputLabelToId == snapshot.labelToId
        && m_outputIdToLabel == nextIdToLabel) {
        return;
    }

    m_outputDeviceOptions = snapshot.labels;
    m_outputLabelToId = snapshot.labelToId;
    m_outputIdToLabel = nextIdToLabel;
    if (m_settingsBridge) {
        const QString remapped = remapSelection(m_settingsBridge->audioOutputDevice(),
                                                snapshot,
                                                previousIdToLabel,
                                                true);
        if (remapped != m_settingsBridge->audioOutputDevice())
            m_settingsBridge->setAudioOutputDevice(remapped);
    }
    emit outputDeviceOptionsChanged();
}

QString AudioDeviceBridge::resolvedDeviceLabel(const QStringList &options,
                                              const QHash<QString, QString> &idToLabel,
                                              const QString &selection,
                                              const QString &placeholder) const
{
    const QString normalized = selection.trimmed();
    if (options.isEmpty())
        return placeholder;
    if (!normalized.isEmpty()) {
        const auto byId = idToLabel.constFind(normalized);
        if (byId != idToLabel.constEnd())
            return byId.value();
        if (options.contains(normalized))
            return normalized;
    }
    return options.first();
}

QString AudioDeviceBridge::normalizedSelection(const QStringList &options,
                                              const QHash<QString, QString> &labelToId,
                                              const QHash<QString, QString> &idToLabel,
                                              const QString &selection) const
{
    const QString normalized = selection.trimmed();
    if (options.isEmpty())
        return QString();
    if (!normalized.isEmpty()) {
        if (idToLabel.contains(normalized))
            return normalized;
        const auto byLabel = labelToId.constFind(normalized);
        if (byLabel != labelToId.constEnd())
            return byLabel.value();
    }
    return deviceIdForLabel(labelToId, options.first());
}

QString AudioDeviceBridge::deviceIdForLabel(const QHash<QString, QString> &labelToId, const QString &label) const
{
    return labelToId.value(label.trimmed());
}

QString AudioDeviceBridge::remapSelection(const QString &selection,
                                          const DeviceSnapshot &snapshot,
                                          const QHash<QString, QString> &previousIdToLabel,
                                          bool outputMode) const
{
    const QString normalized = selection.trimmed();
    if (normalized.isEmpty())
        return QString();

    if (outputMode && normalized.compare(kNoneAudioOutputText, Qt::CaseInsensitive) == 0)
        return QString();

    if (snapshot.labels.isEmpty())
        return normalized;

    if (snapshot.idToLabel.contains(normalized))
        return normalized;

    if (snapshot.labelToId.contains(normalized))
        return snapshot.labelToId.value(normalized);

    const auto previousLabelIt = previousIdToLabel.constFind(normalized);
    if (previousLabelIt != previousIdToLabel.constEnd()) {
        const QString previousLabel = previousLabelIt.value();
        if (snapshot.labelToId.contains(previousLabel))
            return snapshot.labelToId.value(previousLabel);
    }

    return QString();
}

void AudioDeviceBridge::applyPollingMode()
{
    m_refreshTimer.setInterval(m_pollingEnabled
                               ? kAudioRefreshIntervalMs
                               : kAudioBackgroundRefreshIntervalMs);
}
