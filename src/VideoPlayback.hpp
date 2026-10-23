#pragma once

#include "VideoPlaybackApi.h"
#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>
#include <QString>
#include <atomic>
#include <memory>

class MlvReader;

namespace VideoPlayback {
bool isMovie(const QString &path);
QStringList movieNameFilters();
QString formatLabel(const ApertarVideoInfo &info);
QString movieInClip(const QString &path);
bool probe(const QString &path, ApertarVideoInfo &info, QString *error = nullptr);
QString imageSource(const QString &path, int frame = 0, bool thumbnail = false);
}

class VideoImageProvider : public QQuickImageProvider
{
    Q_OBJECT
public:
    explicit VideoImageProvider(bool thumbnails = false);
    ~VideoImageProvider() override;
    QImage requestImage(const QString &, QSize *, const QSize &) override;
    Q_INVOKABLE void setActive(bool active);
signals:
    void frameDecoded(const QString &path, int frame, const QImage &image);
    void decodeFailed(const QString &path, const QString &error);
private:
    void closeDecoder();
    const bool m_thumbnails;
    std::atomic<bool> m_active {true};
    QMutex m_mutex;
    QString m_path;
    void *m_decoder = nullptr;
    std::unique_ptr<MlvReader> m_mlv;
    ApertarVideoInfo m_info {};
};
