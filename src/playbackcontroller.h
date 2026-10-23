#pragma once

#include <QHash>
#include <QObject>
#include <QElapsedTimer>
#include <QFile>
#include <QProcess>
#include <QThreadPool>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include "VideoPlaybackApi.h"

class QImage;

class SettingsBridge;

class PlaybackController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentClipName READ currentClipName NOTIFY clipChanged)
    Q_PROPERTY(QString currentClipPath READ currentClipPath NOTIFY clipChanged)
    Q_PROPERTY(int frameCount READ frameCount NOTIFY clipChanged)
    Q_PROPERTY(int currentFrameIndex READ currentFrameIndex NOTIFY currentFrameChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(double fps READ fps WRITE setFps NOTIFY fpsChanged)
    Q_PROPERTY(QString currentFramePath READ currentFramePath NOTIFY currentFrameChanged)
    Q_PROPERTY(QString frameSource READ frameSource NOTIFY currentFrameChanged)
    Q_PROPERTY(QString fastFrameSource READ fastFrameSource NOTIFY currentFrameChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QVariantList histogramBins READ histogramBins NOTIFY histogramChanged)
    Q_PROPERTY(QVariantMap clipMetadata READ clipMetadata NOTIFY clipMetadataChanged)
    Q_PROPERTY(bool videoClip READ videoClip NOTIFY clipChanged)

public:
    explicit PlaybackController(QObject *parent = nullptr);
    ~PlaybackController() override;

    QString currentClipName() const;
    QString currentClipPath() const;
    int frameCount() const;
    int currentFrameIndex() const;
    bool playing() const;
    double fps() const;
    QString currentFramePath() const;
    QString frameSource() const;
    QString fastFrameSource() const;
    QString statusText() const;
    QVariantList histogramBins() const;
    QVariantMap clipMetadata() const;
    quint64 currentFrameToken() const;
    void setSettingsBridge(SettingsBridge *settingsBridge);
    bool videoClip() const { return !m_videoPath.isEmpty(); }
    void acceptVideoFrame(const QString &path, int index, const QImage &image);
    void videoDecodeFailed(const QString &path, const QString &error);

    Q_INVOKABLE void loadClip(const QString &clipPath);
    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void nextFrame();
    Q_INVOKABLE void previousFrame();
    Q_INVOKABLE void seekToFrame(int frameIndex);

public slots:
    void setFps(double fps);

signals:
    void clipChanged();
    void currentFrameChanged();
    void playingChanged();
    void fpsChanged();
    void statusTextChanged();
    void histogramChanged();
    void clipMetadataChanged();

private:
    struct AudioClipInfo {
        QString filePath;
        qint64 dataOffset = 0;
        qint64 dataSize = 0;
        qint64 durationMs = 0;
        qint64 bytesPerSecond = 0;
        int sampleRate = 0;
        int channels = 0;
        int bitsPerSample = 0;
        bool floatingPoint = false;

        bool valid() const
        {
            return !filePath.isEmpty()
                && dataOffset >= 0
                && dataSize > 0
                && durationMs > 0
                && bytesPerSecond > 0
                && sampleRate > 0
                && channels > 0
                && (bitsPerSample == 16 || bitsPerSample == 24 || bitsPerSample == 32);
        }

        int bytesPerFrame() const
        {
            return channels * (bitsPerSample / 8);
        }
    };

    void reloadFrameList(const QString &clipPath);
    void updateTimerInterval();
    void setPlaying(bool playing);
    void setStatusText(const QString &statusText);
    void setClipMetadata(const QVariantMap &clipMetadata);
    void updateClipMetadata();
    void updateHistogram();
    void resetHistogram();
    void clearHistogramCache();
    void insertHistogramCache(const QString &framePath, const QVariantList &bins);
    void handlePlaybackTick();
    void handleAudioFeedTick();
    void updateFrameFromPlaybackClock();
    void discoverClipAudio();
    AudioClipInfo parseAudioClip(const QString &filePath) const;
    bool startAudioPlaybackForFrame(int frameIndex);
    void stopAudioPlayback();
    QString playbackOutputDevice() const;
    QString normalizedPlaybackDevice(const QString &deviceId) const;
    qint64 audioOffsetMsForFrame(int frameIndex) const;
    qint64 audioBytesForMs(qint64 milliseconds) const;
    qint64 writeAudioChunk(const QByteArray &chunk);
    bool audioPlaybackActive() const;

    QString m_currentClipName;
    QString m_currentClipPath;
    QStringList m_frameFiles;
    QString m_videoPath;
    ApertarVideoInfo m_videoInfo {};
    QElapsedTimer m_videoClock;
    QElapsedTimer m_videoHistogramClock;
    int m_videoStartFrame = 0;
    int m_currentFrameIndex = -1;
    bool m_playing = false;
    double m_fps = 24.0;
    QString m_statusText;
    QVariantList m_histogramBins;
    QVariantMap m_clipMetadata;
    QHash<QString, QVariantList> m_histogramCache;
    QStringList m_histogramCacheOrder;
    QThreadPool m_histogramThreadPool;
    bool m_histogramRequestInFlight = false;
    QString m_histogramRequestedPath;
    quint64 m_frameToken = 0;
    QTimer m_playbackTimer;
    QTimer m_audioFeedTimer;
    SettingsBridge *m_settingsBridge = nullptr;
    AudioClipInfo m_audioClip;
    QFile m_audioFile;
    QProcess m_audioProcess;
    QElapsedTimer m_audioClock;
    QByteArray m_audioPendingBuffer;
    qint64 m_audioStartOffsetMs = 0;
    qint64 m_audioAbsoluteBytesWritten = 0;
};
