#include "VideoPlayback.hpp"
#include "MlvReader.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QPainter>
#include <QUrlQuery>
#include <QCache>
#include <algorithm>
#include <cmath>

namespace {
const ApertarVideoApi *videoApi(QString *error = nullptr)
{
    static QLibrary library;
    static QMutex mutex;
    QMutexLocker lock(&mutex);
    static const ApertarVideoApi *api = nullptr;
    if (api) return api;
    QStringList paths;
    const QString overridePath = qEnvironmentVariable("APERTAR_VIDEO_PLAYBACK_PLUGIN");
    if (!overridePath.isEmpty()) paths.append(overridePath);
    paths.append(QCoreApplication::applicationDirPath() + QStringLiteral("/apertar-video-libav.so"));
    paths.append(QCoreApplication::applicationDirPath() + QStringLiteral("/../lib/apertar-ui/apertar-video-libav.so"));
    for (const auto &path : paths) {
        library.setFileName(path);
        if (!library.load()) continue;
        auto entry = reinterpret_cast<const ApertarVideoApi *(*)()>(library.resolve("apertar_video_api"));
        if (entry && entry()->version == 2) { api = entry(); return api; }
        library.unload();
    }
    if (error) *error = QStringLiteral("Video playback decoder unavailable. Build apertar-video-libav (libswscale-dev required).");
    return nullptr;
}
}

bool VideoPlayback::isMovie(const QString &path)
{
    const QString extension = QFileInfo(path).suffix().toLower();
    return extension == QStringLiteral("mov") || extension == QStringLiteral("mp4") || extension == QStringLiteral("mlv");
}

QStringList VideoPlayback::movieNameFilters()
{
    return {QStringLiteral("*.mov"), QStringLiteral("*.MOV"), QStringLiteral("*.mp4"), QStringLiteral("*.MP4"),
            QStringLiteral("*.mlv"), QStringLiteral("*.MLV")};
}

QString VideoPlayback::formatLabel(const ApertarVideoInfo &info)
{
    return (QString::fromUtf8(info.codec) + QLatin1Char(' ') + QString::fromUtf8(info.profile)).trimmed();
}

QString VideoPlayback::movieInClip(const QString &path)
{
    if (QFileInfo(path).isFile()) return isMovie(path) ? path : QString();
    const QDir dir(path);
    const auto files = dir.entryList(movieNameFilters(), QDir::Files, QDir::Name);
    return files.isEmpty() ? QString() : dir.absoluteFilePath(files.first());
}

bool VideoPlayback::probe(const QString &path, ApertarVideoInfo &info, QString *error)
{
    static QMutex mutex;
    static QCache<QString, ApertarVideoInfo> cache(128);
    const QFileInfo file(path);
    const QString key = file.absoluteFilePath() + QLatin1Char(':') + QString::number(file.size())
        + QLatin1Char(':') + QString::number(file.lastModified().toMSecsSinceEpoch());
    QMutexLocker lock(&mutex);
    if (auto *cached = cache.object(key)) { info = *cached; return true; }
    if (file.suffix().compare(QStringLiteral("mlv"), Qt::CaseInsensitive) == 0) {
        MlvReader reader;
        if (!reader.open(path, error)) return false;
        info = reader.info();
        cache.insert(key, new ApertarVideoInfo(info));
        return true;
    }
    auto *api = videoApi(error);
    if (!api) return false;
    char detail[256] = {};
    if (!api->probe(path.toUtf8().constData(), &info, detail, sizeof(detail))) {
        if (error) *error = QString::fromUtf8(detail);
        return false;
    }
    cache.insert(key, new ApertarVideoInfo(info));
    return true;
}

QString VideoPlayback::imageSource(const QString &path, int frame, bool thumbnail)
{
    return QStringLiteral("image://%1/frame?path64=%2&frame=%3")
        .arg(thumbnail ? QStringLiteral("videothumb") : QStringLiteral("video"),
             QString::fromLatin1(path.toUtf8().toBase64(QByteArray::Base64UrlEncoding)))
        .arg(frame);
}

VideoImageProvider::VideoImageProvider(bool thumbnails)
    : QQuickImageProvider(QQuickImageProvider::Image, ForceAsynchronousImageLoading),
      m_thumbnails(thumbnails)
{
}

VideoImageProvider::~VideoImageProvider() { closeDecoder(); }

void VideoImageProvider::closeDecoder()
{
    if (m_decoder) videoApi()->close(m_decoder);
    m_decoder = nullptr;
    m_mlv.reset();
    m_path.clear();
}

void VideoImageProvider::setActive(bool active)
{
    m_active.store(active);
    // Never block the UI waiting for an in-flight video decode.
    if (!active && m_mutex.tryLock()) {
        closeDecoder();
        m_mutex.unlock();
    }
}

QImage VideoImageProvider::requestImage(const QString &id, QSize *size, const QSize &requested)
{
    QMutexLocker lock(&m_mutex);
    if (!m_active.load()) return {};
    const QUrlQuery query(id.mid(id.indexOf(QLatin1Char('?')) + 1));
    const QString path = QString::fromUtf8(QByteArray::fromBase64(
        query.queryItemValue(QStringLiteral("path64")).toLatin1(), QByteArray::Base64UrlEncoding));
    const int index = query.queryItemValue(QStringLiteral("frame")).toInt();
    QString error;
    const bool mlv = QFileInfo(path).suffix().compare(QStringLiteral("mlv"), Qt::CaseInsensitive) == 0;
    const auto *api = mlv ? nullptr : videoApi(&error);
    if (m_path != path) {
        closeDecoder();
        if ((mlv || api) && VideoPlayback::probe(path, m_info, &error)) {
            if (mlv) {
                m_mlv = std::make_unique<MlvReader>();
                if (m_mlv->open(path, &error)) m_path = path;
                else m_mlv.reset();
            } else {
                char detail[256] = {};
                m_decoder = api->open(path.toUtf8().constData(), detail, sizeof(detail));
                if (m_decoder) m_path = path;
                else error = QString::fromUtf8(detail);
            }
        }
    }
    QImage image;
    if (m_decoder || m_mlv) {
        QSize bounds = requested.isValid() ? requested : QSize(720, 720);
        bounds = bounds.boundedTo(m_thumbnails ? QSize(480, 480) : QSize(1280, 1280));
        const QSize display(std::max(1, int(std::round(m_info.width * m_info.pixelAspect))), m_info.height);
        const QSize output = display.scaled(bounds, Qt::KeepAspectRatio);
        if (m_mlv) {
            image = m_mlv->render(index, output, &error, &m_active);
            if (image.isNull()) closeDecoder();
        } else {
            image = QImage(output, QImage::Format_ARGB32);
            char detail[256] = {};
            if (image.isNull() || !api->read(m_decoder, index, image.bits(), image.width(),
                                           image.height(), image.bytesPerLine(), detail, sizeof(detail))) {
                error = QString::fromUtf8(detail);
                image = {};
                closeDecoder();
            }
        }
    }
    if (m_thumbnails || !m_active.load()) closeDecoder();
    if (!m_active.load()) return {};
    if (image.isNull()) {
        emit decodeFailed(path, error);
        image = QImage(480, 270, QImage::Format_RGB32);
        image.fill(QColor("#171717"));
        QPainter painter(&image);
        painter.setPen(Qt::white);
        painter.drawText(image.rect().adjusted(20, 20, -20, -20), Qt::AlignCenter | Qt::TextWordWrap,
                         error.isEmpty() ? QStringLiteral("Could not decode video") : error);
    } else if (!m_thumbnails) {
        emit frameDecoded(path, index, image);
    }
    if (size) *size = image.size();
    return image;
}
