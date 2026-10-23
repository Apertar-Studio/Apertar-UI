#pragma once

#include <QObject>
#include <QPointer>
#include <QHash>
#include <QFileSystemWatcher>
#include <QStringList>
#include <QTimer>
#include <QJsonArray>
#include <QLocalSocket>

class SettingsBridge;

class AudioDeviceBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList inputDeviceOptions READ inputDeviceOptions NOTIFY inputDeviceOptionsChanged)
    Q_PROPERTY(QStringList outputDeviceOptions READ outputDeviceOptions NOTIFY outputDeviceOptionsChanged)
    Q_PROPERTY(bool hasInputDevices READ hasInputDevices NOTIFY inputDeviceOptionsChanged)
    Q_PROPERTY(bool hasOutputDevices READ hasOutputDevices NOTIFY outputDeviceOptionsChanged)
    Q_PROPERTY(QStringList sampleFormatOptions READ sampleFormatOptions NOTIFY captureOptionsChanged)
    Q_PROPERTY(QStringList sampleRateOptions READ sampleRateOptions NOTIFY captureOptionsChanged)
    Q_PROPERTY(QStringList channelOptions READ channelOptions NOTIFY captureOptionsChanged)
    Q_PROPERTY(QString sampleFormatLabel READ sampleFormatLabel NOTIFY captureOptionsChanged)
    Q_PROPERTY(QString sampleRateLabel READ sampleRateLabel NOTIFY captureOptionsChanged)
    Q_PROPERTY(QString channelsLabel READ channelsLabel NOTIFY captureOptionsChanged)
    Q_PROPERTY(QString captureDescription READ captureDescription NOTIFY captureOptionsChanged)
    Q_PROPERTY(bool capabilitiesBusy READ capabilitiesBusy NOTIFY captureOptionsChanged)

public:
    struct DeviceSnapshot {
        QStringList labels;
        QHash<QString, QString> labelToId;
        QHash<QString, QString> idToLabel;
    };

    explicit AudioDeviceBridge(QObject *parent = nullptr);

    QStringList inputDeviceOptions() const;
    QStringList outputDeviceOptions() const;
    bool hasInputDevices() const;
    bool hasOutputDevices() const;

    void setSettingsBridge(SettingsBridge *settingsBridge);
    Q_INVOKABLE void setPollingEnabled(bool enabled);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void refreshCaptureCapabilities();
    Q_INVOKABLE void selectSampleFormat(const QString &label);
    Q_INVOKABLE void selectSampleRate(const QString &label);
    Q_INVOKABLE void selectChannels(const QString &label);
    QStringList sampleFormatOptions() const;
    QStringList sampleRateOptions() const;
    QStringList channelOptions() const;
    QString sampleFormatLabel() const;
    QString sampleRateLabel() const;
    QString channelsLabel() const;
    QString captureDescription() const;
    bool capabilitiesBusy() const { return m_capabilitiesBusy; }
    Q_INVOKABLE QString resolvedInputDeviceLabel(const QString &selection) const;
    Q_INVOKABLE QString resolvedOutputDeviceLabel(const QString &selection) const;
    Q_INVOKABLE QString normalizeInputSelection(const QString &selection) const;
    Q_INVOKABLE QString normalizeOutputSelection(const QString &selection) const;
    Q_INVOKABLE QString inputDeviceIdForLabel(const QString &label) const;
    Q_INVOKABLE QString outputDeviceIdForLabel(const QString &label) const;

signals:
    void inputDeviceOptionsChanged();
    void outputDeviceOptionsChanged();
    void captureOptionsChanged();
    void capabilityProbeActiveChanged(bool active);

private:
    DeviceSnapshot detectDevices(const QString &tool) const;
    void setInputDevices(const DeviceSnapshot &snapshot);
    void setOutputDevices(const DeviceSnapshot &snapshot);
    QString resolvedDeviceLabel(const QStringList &options,
                                const QHash<QString, QString> &idToLabel,
                                const QString &selection,
                                const QString &placeholder) const;
    QString normalizedSelection(const QStringList &options,
                                const QHash<QString, QString> &labelToId,
                                const QHash<QString, QString> &idToLabel,
                                const QString &selection) const;
    QString deviceIdForLabel(const QHash<QString, QString> &labelToId, const QString &label) const;
    QString remapSelection(const QString &selection,
                           const DeviceSnapshot &snapshot,
                           const QHash<QString, QString> &previousIdToLabel,
                           bool outputMode) const;
    void applyPollingMode();
    int maximumChannels() const;
    void finishCapabilityProbe(const QString &error);

    QJsonArray m_captureModes;
    QPointer<QLocalSocket> m_capabilitySocket;
    QString m_capabilityError;
    QString m_capabilityDevice;
    bool m_capabilitiesBusy = false;

    QPointer<SettingsBridge> m_settingsBridge;
    QFileSystemWatcher m_deviceWatcher;
    QTimer m_refreshTimer;
    QTimer m_hotplugRefreshTimer;
    bool m_pollingEnabled = false;
    QStringList m_inputDeviceOptions;
    QStringList m_outputDeviceOptions;
    QHash<QString, QString> m_inputLabelToId;
    QHash<QString, QString> m_outputLabelToId;
    QHash<QString, QString> m_inputIdToLabel;
    QHash<QString, QString> m_outputIdToLabel;
};
