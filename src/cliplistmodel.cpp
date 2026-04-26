#include "cliplistmodel.h"

#include "dngdecoder.h"

#include <QByteArray>
#include <QCollator>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QRegularExpression>

#include <tiffio.h>

#include "tiffhelper.h"

namespace {
QStringList dngNameFilters()
{
    return {QStringLiteral("*.dng"), QStringLiteral("*.DNG")};
}

QString imageSourceForFrame(const QString &filePath)
{
    const QString encodedPath = QString::fromLatin1(filePath.toUtf8().toBase64(QByteArray::Base64UrlEncoding));
    return QStringLiteral("image://cdngthumb/thumb?path64=%1&radius=18").arg(encodedPath);
}

QString shotDateForFrame(const QString &filePath)
{
    auto formatShotDate = [](const QString &rawDate) -> QString {
        const QDateTime parsed = QDateTime::fromString(rawDate.trimmed(), QStringLiteral("yyyy:MM:dd HH:mm:ss"));
        if (parsed.isValid()) {
            return QLocale::system().toString(parsed, QStringLiteral("dd MMM yyyy  HH:mm"));
        }
        return rawDate.trimmed();
    };

    TIFF *tiff = openTiffWithCustomTags(filePath, "r");
    char *dateTimeValue = nullptr;
    QString formattedDate;
    if (tiff) {
        if (TIFFGetField(tiff, kTiffTagDateTimeOriginal, &dateTimeValue) && dateTimeValue) {
            formattedDate = formatShotDate(QString::fromLatin1(dateTimeValue));
        } else if (TIFFGetField(tiff, TIFFTAG_DATETIME, &dateTimeValue) && dateTimeValue) {
            formattedDate = formatShotDate(QString::fromLatin1(dateTimeValue));
        }

        TIFFClose(tiff);
    }

    if (!formattedDate.isEmpty()) {
        return formattedDate;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const QString fileText = QString::fromLatin1(file.readAll());
    const QRegularExpression timestampPattern(QStringLiteral(R"((20\d\d:\d\d:\d\d \d\d:\d\d:\d\d))"));
    const QRegularExpressionMatch match = timestampPattern.match(fileText);
    if (!match.hasMatch()) {
        return {};
    }

    return formatShotDate(match.captured(1));
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
    scanClips();
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
    scanClips();
}

void ClipListModel::setStillMode(bool stillMode)
{
    if (m_stillMode == stillMode) {
        return;
    }

    m_stillMode = stillMode;
    emit stillModeChanged();
    scanClips();
}

bool ClipListModel::loading() const
{
    return m_loading;
}

QString ClipListModel::statusText() const
{
    return m_statusText;
}

void ClipListModel::refresh()
{
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
        if (!QFile::remove(path)) {
            setStatusText(QStringLiteral("Failed to remove %1.").arg(clipName));
            return false;
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
        if (stillDir.exists()) {
            QStringList stills = stillDir.entryList(dngNameFilters(), QDir::Files, QDir::Name);
            std::sort(stills.begin(), stills.end(), [&collator](const QString &lhs, const QString &rhs) {
                return collator.compare(lhs, rhs) < 0;
            });

            for (const QString &stillName : stills) {
                const QString stillPath = stillDir.absoluteFilePath(stillName);
                ClipInfo clip;
                clip.name = QFileInfo(stillName).completeBaseName();
                clip.path = stillPath;
                clip.frameCount = 1;
                clip.durationText = QStringLiteral("STILL");
                clip.thumbnailSource = imageSourceForFrame(stillPath);
                clip.shotDate = shotDateForFrame(stillPath);
                clips.append(clip);
            }
        }
    } else {
        const QFileInfoList dirEntries = rootDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &entry : dirEntries) {
            if (entry.fileName().compare(QStringLiteral("Photos"), Qt::CaseInsensitive) == 0) {
                continue;
            }

            const QDir clipDir(entry.absoluteFilePath());
            QStringList frames = clipDir.entryList(dngNameFilters(), QDir::Files, QDir::Name);
            if (frames.isEmpty()) {
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
            const double detectedFps = DngDecoder::detectFrameRate(firstFramePath);
            clip.durationText = durationTextForClip(clip.frameCount, detectedFps);
            clip.thumbnailSource = imageSourceForFrame(firstFramePath);
            clip.shotDate = shotDateForFrame(firstFramePath);
            clips.append(clip);
        }
    }

    std::sort(clips.begin(), clips.end(), [&collator](const ClipInfo &lhs, const ClipInfo &rhs) {
        return collator.compare(lhs.name, rhs.name) < 0;
    });

    beginResetModel();
    m_clips = clips;
    endResetModel();
    emit countChanged();

    if (m_clips.isEmpty()) {
        setStatusText(m_stillMode
                          ? QStringLiteral("No still DNG files found in %1/Photos.").arg(m_rootPath)
                          : QStringLiteral("No cDNG folders found in %1.").arg(m_rootPath));
    } else {
        setStatusText(m_stillMode
                          ? QStringLiteral("Found %1 still(s) in %2/Photos.").arg(m_clips.size()).arg(m_rootPath)
                          : QStringLiteral("Found %1 clip(s) in %2.").arg(m_clips.size()).arg(m_rootPath));
    }

    setLoading(false);
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
