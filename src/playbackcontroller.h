#pragma once

#include <QHash>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class PlaybackController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentClipName READ currentClipName NOTIFY clipChanged)
    Q_PROPERTY(QString currentClipPath READ currentClipPath NOTIFY clipChanged)
    Q_PROPERTY(int frameCount READ frameCount NOTIFY clipChanged)
    Q_PROPERTY(int currentFrameIndex READ currentFrameIndex NOTIFY currentFrameChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(double fps READ fps WRITE setFps NOTIFY fpsChanged)
    Q_PROPERTY(QString frameSource READ frameSource NOTIFY currentFrameChanged)
    Q_PROPERTY(QString fastFrameSource READ fastFrameSource NOTIFY currentFrameChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QVariantList histogramBins READ histogramBins NOTIFY histogramChanged)
    Q_PROPERTY(QVariantMap clipMetadata READ clipMetadata NOTIFY clipMetadataChanged)

public:
    explicit PlaybackController(QObject *parent = nullptr);

    QString currentClipName() const;
    QString currentClipPath() const;
    int frameCount() const;
    int currentFrameIndex() const;
    bool playing() const;
    double fps() const;
    QString frameSource() const;
    QString fastFrameSource() const;
    QString statusText() const;
    QVariantList histogramBins() const;
    QVariantMap clipMetadata() const;
    quint64 currentFrameToken() const;

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
    void reloadFrameList(const QString &clipPath);
    void updateTimerInterval();
    void setPlaying(bool playing);
    void setStatusText(const QString &statusText);
    void setClipMetadata(const QVariantMap &clipMetadata);
    void updateClipMetadata();
    void updateHistogram();
    void resetHistogram();

    QString m_currentClipName;
    QString m_currentClipPath;
    QStringList m_frameFiles;
    int m_currentFrameIndex = -1;
    bool m_playing = false;
    double m_fps = 24.0;
    QString m_statusText;
    QVariantList m_histogramBins;
    QVariantMap m_clipMetadata;
    QHash<QString, QVariantList> m_histogramCache;
    quint64 m_frameToken = 0;
    QTimer m_playbackTimer;
};
