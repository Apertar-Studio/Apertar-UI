#include "cliplistmodel.h"

#include "dngdecoder.h"
#include "VideoPlayback.hpp"

#include <QByteArray>
#include <QCollator>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QSet>

#include <tiffio.h>

#include "tiffhelper.h"

namespace {
QStringList dngNameFilters()
{
    return {QStringLiteral("*.dng"), QStringLiteral("*.DNG")};
}

QStringList stillNameFilters()
{
    return {
        QStringLiteral("*.dng"), QStringLiteral("*.DNG"),
        QStringLiteral("*.jpg"), QStringLiteral("*.JPG"),
        QStringLiteral("*.jpeg"), QStringLiteral("*.JPEG"),
        QStringLiteral("*.png"), QStringLiteral("*.PNG"),
        QStringLiteral("*.tif"), QStringLiteral("*.TIF"),
        QStringLiteral("*.tiff"), QStringLiteral("*.TIFF"),
    };
}

bool hasTiffMetadata(const QString &filePath)
{
    const QString suffix = QFileInfo(filePath).suffix();
    return suffix.compare(QStringLiteral("dng"), Qt::CaseInsensitive) == 0 ||
           suffix.compare(QStringLiteral("tif"), Qt::CaseInsensitive) == 0 ||
           suffix.compare(QStringLiteral("tiff"), Qt::CaseInsensitive) == 0;
}

QString stillFormatLabel(const QString &filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == QStringLiteral("jpg") || suffix == QStringLiteral("jpeg"))
        return QStringLiteral("JPEG STILL");
    if (suffix == QStringLiteral("png"))
        return QStringLiteral("PNG STILL");
    if (suffix == QStringLiteral("tif") || suffix == QStringLiteral("tiff"))
        return QStringLiteral("TIFF STILL");
    return QStringLiteral("DNG STILL");
}

struct ShotDateInfo
{
    QString display;
    qint64 timestampMs = 0;
};

QString imageSourceForFrame(const QString &filePath)
{
    const QString encodedPath = QString::fromLatin1(filePath.toUtf8().toBase64(QByteArray::Base64UrlEncoding));
    return QStringLiteral("image://cdngthumb/thumb?path64=%1&radius=18").arg(encodedPath);
}

ShotDateInfo shotDateInfoForFrame(const QString &filePath)
{
    auto formatShotDate = [](const QString &rawDate) -> QString {
        const QDateTime parsed = QDateTime::fromString(rawDate.trimmed(), QStringLiteral("yyyy:MM:dd HH:mm:ss"));
        if (parsed.isValid()) {
            return QLocale::system().toString(parsed, QStringLiteral("dd MMM yyyy  HH:mm"));
        }
        return rawDate.trimmed();
    };
    auto parseShotDate = [](const QString &rawDate) -> QDateTime {
        return QDateTime::fromString(rawDate.trimmed(), QStringLiteral("yyyy:MM:dd HH:mm:ss"));
    };

    char *dateTimeValue = nullptr;
    ShotDateInfo info;
    if (hasTiffMetadata(filePath)) {
        TIFF *tiff = openTiffWithCustomTags(filePath, "r");
        if (!tiff) {
            const QFileInfo fileInfo(filePath);
            info.timestampMs = fileInfo.lastModified().toMSecsSinceEpoch();
            return info;
        }

        if (TIFFGetField(tiff, kTiffTagDateTimeOriginal, &dateTimeValue) && dateTimeValue) {
            const QString rawDate = QString::fromLatin1(dateTimeValue);
            info.display = formatShotDate(rawDate);
            const QDateTime parsed = parseShotDate(rawDate);
            if (parsed.isValid())
                info.timestampMs = parsed.toMSecsSinceEpoch();
        } else if (TIFFGetField(tiff, TIFFTAG_DATETIME, &dateTimeValue) && dateTimeValue) {
            const QString rawDate = QString::fromLatin1(dateTimeValue);
            info.display = formatShotDate(rawDate);
            const QDateTime parsed = parseShotDate(rawDate);
            if (parsed.isValid())
                info.timestampMs = parsed.toMSecsSinceEpoch();
        }

        TIFFClose(tiff);
    } else {
        const QFileInfo fileInfo(filePath);
        info.timestampMs = fileInfo.lastModified().toMSecsSinceEpoch();
        info.display = QLocale::system().toString(fileInfo.lastModified(), QStringLiteral("dd MMM yyyy  HH:mm"));
        return info;
    }

    if (!info.display.isEmpty()) {
        return info;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        const QFileInfo fileInfo(filePath);
        info.timestampMs = fileInfo.lastModified().toMSecsSinceEpoch();
        return info;
    }

    // Fallback date sniffing must not load a full RAW/16-bit TIFF just to sort it.
    const QString fileText = QString::fromLatin1(file.read(64 * 1024));
    const QRegularExpression timestampPattern(QStringLiteral(R"((20\d\d:\d\d:\d\d \d\d:\d\d:\d\d))"));
    const QRegularExpressionMatch match = timestampPattern.match(fileText);
    if (!match.hasMatch()) {
        const QFileInfo fileInfo(filePath);
        info.timestampMs = fileInfo.lastModified().toMSecsSinceEpoch();
        return info;
    }

    const QString rawDate = match.captured(1);
    info.display = formatShotDate(rawDate);
    const QDateTime parsed = parseShotDate(rawDate);
    if (parsed.isValid())
        info.timestampMs = parsed.toMSecsSinceEpoch();
    else
        info.timestampMs = QFileInfo(filePath).lastModified().toMSecsSinceEpoch();
    return info;
}

QString durationTextForClip(int frameCount, double fps)
{
    if (frameCount <= 0) {
        return QStringLiteral("00:00");
    }

    const double safeFps = fps > 0.0 ? fps : 24.0;
    const int totalSeconds = qMax(0, qRound(static_cast<double>(frameCount) / safeFps));
    const int hours = totalSeconds / 3600;
    const int minutes = (totalSeconds % 3600) / 60;
    const int seconds = totalSeconds % 60;

    if (hours > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(hours, 2, 10, QLatin1Char('0'))
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }

    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}
}

ClipListModel::ClipListModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_rootPath(QStringLiteral("/media/RAW"))
{
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(150);
    connect(&m_mediaWatcher, &QFileSystemWatcher::directoryChanged, this, [this]() {
        if (m_browserOpen)
            m_refreshTimer.start();
    });
    connect(&m_mediaWatcher, &QFileSystemWatcher::fileChanged, this, [this]() {
        if (m_browserOpen)
            m_refreshTimer.start();
    });
    connect(&m_refreshTimer, &QTimer::timeout, this, [this]() {
        if (m_browserOpen)
            refresh();
    });
}

int ClipListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_clips.size();
}

QVariant ClipListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_clips.size()) {
        return {};
    }

    const ClipInfo &clip = m_clips.at(index.row());
    switch (role) {
    case NameRole:
        return clip.name;
    case PathRole:
        return clip.path;
    case FrameCountRole:
        return clip.frameCount;
    case DurationTextRole:
        return clip.durationText;
    case ThumbnailSourceRole:
        return clip.thumbnailSource;
    case ShotDateRole:
        return clip.shotDate;
    case FormatLabelRole:
        return clip.formatLabel;
    case Qt::DisplayRole:
        return clip.name;
    default:
        return {};
    }
}

QHash<int, QByteArray> ClipListModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {PathRole, "path"},
        {FrameCountRole, "frameCount"},
        {DurationTextRole, "durationText"},
        {ThumbnailSourceRole, "thumbnailSource"},
        {ShotDateRole, "shotDate"},
        {FormatLabelRole, "formatLabel"},
    };
}

QString ClipListModel::rootPath() const
{
    return m_rootPath;
}

bool ClipListModel::stillMode() const
{
    return m_stillMode;
}

QString ClipListModel::sortMode() const
{
    return m_sortMode;
}

int ClipListModel::count() const
{
    return m_clips.size();
}

void ClipListModel::setRootPath(const QString &rootPath)
{
    if (m_rootPath == rootPath) {
        return;
    }

    m_rootPath = rootPath;
    emit rootPathChanged();
    if (m_hasScanned)
        scanClips();
}

void ClipListModel::setStillMode(bool stillMode)
{
    if (m_stillMode == stillMode) {
        return;
    }

    m_stillMode = stillMode;
    emit stillModeChanged();
    if (m_hasScanned)
        scanClips();
}

void ClipListModel::setSortMode(const QString &sortMode)
{
    const QString normalized = sortMode == QStringLiteral("Newest First")
        ? QStringLiteral("Newest First")
        : QStringLiteral("Oldest First");
    if (m_sortMode == normalized)
        return;

    m_sortMode = normalized;
    emit sortModeChanged();
    if (m_hasScanned)
        scanClips();
}

bool ClipListModel::loading() const
{
    return m_loading;
}

bool ClipListModel::browserOpen() const
{
    return m_browserOpen;
}

void ClipListModel::setBrowserOpen(bool open)
{
    if (m_browserOpen == open)
        return;
    m_browserOpen = open;
    emit browserOpenChanged();
    m_refreshTimer.stop();
    if (open)
        refresh();
    else
        updateDirectoryWatches();
}

QString ClipListModel::statusText() const
{
    return m_statusText;
}

void ClipListModel::refresh()
{
    m_hasScanned = true;
    scanClips();
}

int ClipListModel::indexOfPath(const QString &path) const
{
    for (int i = 0; i < m_clips.size(); ++i) {
        if (m_clips.at(i).path == path) {
            return i;
        }
    }

    return -1;
}

QString ClipListModel::pathAt(int index) const
{
    if (index < 0 || index >= m_clips.size()) {
        return {};
    }

    return m_clips.at(index).path;
}

QString ClipListModel::nameAt(int index) const
{
    if (index < 0 || index >= m_clips.size()) {
        return {};
    }

    return m_clips.at(index).name;
}

int ClipListModel::frameCountAt(int index) const
{
    if (index < 0 || index >= m_clips.size()) {
        return 0;
    }

    return m_clips.at(index).frameCount;
}

QString ClipListModel::durationTextAt(int index) const
{
    if (index < 0 || index >= m_clips.size()) {
        return {};
    }

    return m_clips.at(index).durationText;
}

bool ClipListModel::removeClip(const QString &path)
{
    if (path.isEmpty()) {
        setStatusText(QStringLiteral("No clip path provided."));
        return false;
    }

    const QFileInfo pathInfo(path);
    const QString root = QFileInfo(m_rootPath).canonicalFilePath();
    const QString target = pathInfo.canonicalFilePath();
    if (root.isEmpty() || target.isEmpty() || pathInfo.isSymLink() ||
        !target.startsWith(root + QLatin1Char('/')) || indexOfPath(path) < 0) {
        setStatusText(QStringLiteral("Only listed media inside the recording folder can be deleted."));
        return false;
    }
    const QString clipName = pathInfo.completeBaseName().isEmpty() ? pathInfo.fileName() : pathInfo.completeBaseName();
    if (pathInfo.isDir()) {
        QDir clipDir(path);
        if (!clipDir.exists()) {
            setStatusText(QStringLiteral("Clip folder is not available: %1").arg(path));
            return false;
        }

        if (!clipDir.removeRecursively()) {
            setStatusText(QStringLiteral("Failed to remove %1.").arg(clipName));
            return false;
        }
    } else if (pathInfo.isFile()) {
        QStringList targets{path};
        if (m_stillMode) {
            targets.clear();
            // A paired DNG/processed still is one browser item, not two photos.
            for (const auto &sibling : pathInfo.dir().entryInfoList(stillNameFilters(), QDir::Files | QDir::NoSymLinks)) {
                if (sibling.completeBaseName().compare(pathInfo.completeBaseName(), Qt::CaseInsensitive) == 0)
                    targets.append(sibling.absoluteFilePath());
            }
        }
        for (const auto &file : targets) {
            const QFileInfo item(file);
            if (item.isSymLink() || !item.canonicalFilePath().startsWith(root + QLatin1Char('/'))) {
                setStatusText(QStringLiteral("Unsafe media deletion target."));
                return false;
            }
        }
        for (const auto &file : targets) {
            if (!QFile::remove(file)) {
                scanClips();
                setStatusText(QStringLiteral("Failed to remove %1.").arg(clipName));
                return false;
            }
        }
    } else {
        setStatusText(QStringLiteral("Item is not available: %1").arg(path));
        return false;
    }

    scanClips();
    setStatusText(QStringLiteral("Removed %1.").arg(clipName));
    return true;
}

void ClipListModel::scanClips()
{
    setLoading(true);
    // Watch empty/new clip directories too: the first DNG may still be writing.
    updateDirectoryWatches();

    QList<ClipInfo> clips;
    const QDir rootDir(m_rootPath);

    if (!rootDir.exists()) {
        beginResetModel();
        m_clips.clear();
        endResetModel();
        emit countChanged();
        setStatusText(QStringLiteral("%1 is not available.").arg(m_rootPath));
        setLoading(false);
        return;
    }

    QCollator collator;
    collator.setNumericMode(true);

    if (m_stillMode) {
        const QDir stillDir(rootDir.absoluteFilePath(QStringLiteral("Photos")));
        if (stillDir.exists() && !QFileInfo(stillDir.absolutePath()).isSymLink()) {
            QStringList stills = stillDir.entryList(stillNameFilters(), QDir::Files | QDir::NoSymLinks, QDir::Name);
            std::sort(stills.begin(), stills.end(), [&collator](const QString &lhs, const QString &rhs) {
                return collator.compare(lhs, rhs) < 0;
            });

            QSet<QString> emittedBases;
            for (const QString &stillName : stills) {
                const QFileInfo stillInfo(stillName);
                const QString baseKey = stillInfo.completeBaseName().toLower();
                if (emittedBases.contains(baseKey))
                    continue;
                emittedBases.insert(baseKey);

                const QString stillPath = stillDir.absoluteFilePath(stillName);
                const ShotDateInfo shotInfo = shotDateInfoForFrame(stillPath);
                ClipInfo clip;
                clip.name = stillInfo.completeBaseName();
                clip.path = stillPath;
                clip.frameCount = 1;
                clip.durationText = stillFormatLabel(stillPath);
                clip.thumbnailSource = imageSourceForFrame(stillPath);
                clip.shotDate = shotInfo.display;
                clip.shotTimestampMs = shotInfo.timestampMs;
                clips.append(clip);
            }
        }
    } else {
        const QFileInfoList dirEntries = rootDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name);
        for (const QFileInfo &entry : dirEntries) {
            if (entry.fileName().compare(QStringLiteral("Photos"), Qt::CaseInsensitive) == 0) {
                continue;
            }

            const QDir clipDir(entry.absoluteFilePath());
            QStringList frames = clipDir.entryList(dngNameFilters(), QDir::Files | QDir::NoSymLinks, QDir::Name);
            if (frames.isEmpty()) {
                const QString movie = VideoPlayback::movieInClip(entry.absoluteFilePath());
                if (!movie.isEmpty()) {
                    ApertarVideoInfo info {};
                    const bool valid = VideoPlayback::probe(movie, info);
                    const ShotDateInfo date = shotDateInfoForFrame(movie);
                    ClipInfo clip;
                    clip.name = entry.fileName();
                    clip.path = entry.absoluteFilePath();
                    clip.frameCount = valid ? info.frames : 0;
                    clip.durationText = valid ? durationTextForClip(info.frames, info.fps) : QStringLiteral("VIDEO");
                    clip.thumbnailSource = VideoPlayback::imageSource(movie, 0, true);
                    clip.formatLabel = valid ? QString::fromUtf8(info.codec) + QStringLiteral(" clip") : QStringLiteral("Video clip");
                    clip.shotDate = date.display;
                    clip.shotTimestampMs = date.timestampMs;
                    clips.append(clip);
                }
                continue;
            }

            std::sort(frames.begin(), frames.end(), [&collator](const QString &lhs, const QString &rhs) {
                return collator.compare(lhs, rhs) < 0;
            });

            ClipInfo clip;
            clip.name = entry.fileName();
            clip.path = entry.absoluteFilePath();
            clip.frameCount = frames.size();
            const QString firstFramePath = clipDir.absoluteFilePath(frames.first());
            const ShotDateInfo shotInfo = shotDateInfoForFrame(firstFramePath);
            const double detectedFps = DngDecoder::detectFrameRate(firstFramePath);
            clip.durationText = durationTextForClip(clip.frameCount, detectedFps);
            clip.thumbnailSource = imageSourceForFrame(firstFramePath);
            clip.shotDate = shotInfo.display;
            clip.shotTimestampMs = shotInfo.timestampMs;
            clips.append(clip);
        }
        // Also accept movies copied directly to the media root.
        for (const auto &entry : rootDir.entryInfoList(VideoPlayback::movieNameFilters(), QDir::Files | QDir::NoSymLinks)) {
            ApertarVideoInfo info {};
            const QString movie = entry.absoluteFilePath();
            const bool valid = VideoPlayback::probe(movie, info);
            const ShotDateInfo date = shotDateInfoForFrame(movie);
            ClipInfo clip;
            clip.name = entry.completeBaseName();
            clip.path = movie;
            clip.frameCount = valid ? info.frames : 0;
            clip.durationText = valid ? durationTextForClip(info.frames, info.fps) : QStringLiteral("VIDEO");
            clip.thumbnailSource = VideoPlayback::imageSource(movie, 0, true);
            clip.formatLabel = valid ? QString::fromUtf8(info.codec) + QStringLiteral(" clip") : QStringLiteral("Video clip");
            clip.shotDate = date.display;
            clip.shotTimestampMs = date.timestampMs;
            clips.append(clip);
        }
    }

    const bool newestFirst = (m_sortMode == QStringLiteral("Newest First"));
    std::sort(clips.begin(), clips.end(), [&collator, newestFirst](const ClipInfo &lhs, const ClipInfo &rhs) {
        if (lhs.shotTimestampMs != rhs.shotTimestampMs) {
            if (newestFirst)
                return lhs.shotTimestampMs > rhs.shotTimestampMs;
            return lhs.shotTimestampMs < rhs.shotTimestampMs;
        }
        const int nameOrder = collator.compare(lhs.name, rhs.name);
        return newestFirst ? nameOrder > 0 : nameOrder < 0;
    });

    beginResetModel();
    m_clips = clips;
    endResetModel();
    emit countChanged();

    if (m_clips.isEmpty()) {
        setStatusText(m_stillMode
                          ? QStringLiteral("No still files found in %1/Photos.").arg(m_rootPath)
                          : QStringLiteral("No cDNG, ProRes, H.264 or MLV clips found in %1.").arg(m_rootPath));
    } else {
        setStatusText(m_stillMode
                          ? QStringLiteral("Found %1 still(s) in %2/Photos.").arg(m_clips.size()).arg(m_rootPath)
                          : QStringLiteral("Found %1 clip(s) in %2.").arg(m_clips.size()).arg(m_rootPath));
    }

    setLoading(false);
}

void ClipListModel::updateDirectoryWatches()
{
    QStringList wanted;
    if (m_browserOpen) {
        const QDir root(m_rootPath);
        if (root.exists()) {
            wanted.append(root.absolutePath());
            if (m_stillMode) {
                const QString photos = root.absoluteFilePath(QStringLiteral("Photos"));
                if (QDir(photos).exists())
                    wanted.append(photos);
            } else {
                const QStringList mlvFilters {QStringLiteral("*.mlv"), QStringLiteral("*.MLV")};
                for (const auto &file : root.entryInfoList(mlvFilters, QDir::Files))
                    wanted.append(file.absoluteFilePath());
                for (const auto &directory : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                    if (directory.fileName().compare(QStringLiteral("Photos"), Qt::CaseInsensitive) != 0) {
                        wanted.append(directory.absoluteFilePath());
                        for (const auto &file : QDir(directory.absoluteFilePath()).entryInfoList(mlvFilters, QDir::Files))
                            wanted.append(file.absoluteFilePath());
                    }
                }
            }
        }
    }
    const auto current = m_mediaWatcher.directories() + m_mediaWatcher.files();
    QStringList removed;
    QStringList added;
    for (const auto &path : current)
        if (!wanted.contains(path)) removed.append(path);
    for (const auto &path : wanted)
        if (!current.contains(path)) added.append(path);
    if (!removed.isEmpty()) m_mediaWatcher.removePaths(removed);
    if (!added.isEmpty()) m_mediaWatcher.addPaths(added);
}

void ClipListModel::setLoading(bool loading)
{
    if (m_loading == loading) {
        return;
    }

    m_loading = loading;
    emit loadingChanged();
}

void ClipListModel::setStatusText(const QString &statusText)
{
    if (m_statusText == statusText) {
        return;
    }

    m_statusText = statusText;
    emit statusTextChanged();
}
