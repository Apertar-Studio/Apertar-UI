#pragma once

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTimer>

class SettingsBridge;

class AudioMixerBridge : public QObject
{
    Q_OBJECT

public:
    explicit AudioMixerBridge(QObject *parent = nullptr);

    void setSettingsBridge(SettingsBridge *settingsBridge);

private:
    enum class MixerDirection {
        Input,
        Output
    };

    static QString mixerCardForDevice(const QString &deviceId);
    static QStringList preferredControls(const QStringList &controls, MixerDirection direction);

    void scheduleInputApply();
    void scheduleOutputApply();
    void applyInputMixer();
    void applyOutputMixer();
    QStringList mixerControlsForCard(const QString &cardSpec);
    bool applyMixerVolume(const QString &cardSpec,
                          MixerDirection direction,
                          int level);

    QPointer<SettingsBridge> m_settingsBridge;
    QTimer m_inputApplyTimer;
    QTimer m_outputApplyTimer;
    QHash<QString, QStringList> m_cardControlsCache;
};
