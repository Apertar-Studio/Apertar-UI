#include "playbackcontroller.h"

#include <QByteArray>
#include <QCollator>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>
#include <QLocale>
#include <QRegularExpression>
#include <QTime>
#include <QUrl>

#include <array>
#include <algorithm>
#include <cmath>

#include <tiffio.h>

#include "dngdecoder.h"
#include "tiffhelper.h"

namespace {
QStringList dngNameFilters()
{
    return {QStringLiteral("*.dng"), QStringLiteral("*.DNG")};
}

QString formatShotDateTime(const QString &rawDate)
{
    const QDateTime parsed = QDateTime::fromString(rawDate.trimmed(), QStringLiteral("yyyy:MM:dd HH:mm:ss"));
    if (parsed.isValid()) {
        return QLocale::system().toString(parsed, QStringLiteral("dd MMM yyyy  HH:mm"));
    }
    return rawDate.trimmed();
}

QString shotDateTimeForFrame(const QString &filePath)
{
    TIFF *tiff = openTiffWithCustomTags(filePath, "r");
    char *dateTimeValue = nullptr;
    QString formattedDate;
    if (tiff) {
        if (TIFFGetField(tiff, kTiffTagDateTimeOriginal, &dateTimeValue) && dateTimeValue) {
            formattedDate = formatShotDateTime(QString::fromLatin1(dateTimeValue));
        } else if (TIFFGetField(tiff, TIFFTAG_DATETIME, &dateTimeValue) && dateTimeValue) {
            formattedDate = formatShotDateTime(QString::fromLatin1(dateTimeValue));
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

    return formatShotDateTime(match.captured(1));
}

QString formatByteSize(quint64 bytes)
{
    static const char *kUnits[] = {"B", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(bytes);
    int unitIndex = 0;
    while (value >= 1024.0 && unitIndex < 4) {
        value /= 1024.0;
        ++unitIndex;
    }

    const int precision = value >= 100.0 || unitIndex == 0 ? 0 : 1;
    return QStringLiteral("%1 %2").arg(QString::number(value, 'f', precision), QString::fromLatin1(kUnits[unitIndex]));
}

QString formatDurationText(int frameCount, double fps)
{
    if (frameCount <= 0 || fps <= 0.0) {
        return QStringLiteral("Unavailable");
    }

    const qint64 totalMs = qRound64((static_cast<double>(frameCount) / fps) * 1000.0);
    const QTime duration = QTime(0, 0).addMSecs(static_cast<int>(totalMs));
    return totalMs >= 3600000
           ? duration.toString(QStringLiteral("hh:mm:ss"))
           : duration.toString(QStringLiteral("mm:ss"));
}

QString resolutionTextForFrame(const RawFrame &frame)
{
    if (!frame.valid || frame.width <= 0 || frame.height <= 0) {
        return QStringLiteral("Unavailable");
    }

    const int width = frame.hasActiveArea ? qMax(1, frame.activeRight - frame.activeLeft) : frame.width;
    const int height = frame.hasActiveArea ? qMax(1, frame.activeBottom - frame.activeTop) : frame.height;
    return QStringLiteral("%1 x %2").arg(width).arg(height);
}

QString bitDepthTextForFrame(const RawFrame &frame)
{
    return frame.valid && frame.bitsPerSample > 0
           ? QStringLiteral("%1-bit").arg(frame.bitsPerSample)
           : QStringLiteral("Unavailable");
}

quint64 clipSizeBytes(const QString &clipPath)
{
    const QFileInfo pathInfo(clipPath);
    if (pathInfo.isFile()) {
        return static_cast<quint64>(pathInfo.size());
    }

    quint64 totalBytes = 0;
    QDirIterator it(clipPath, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        totalBytes += static_cast<quint64>(it.fileInfo().size());
    }
    return totalBytes;
}

constexpr double kFallbackPlaybackFps = 24.0;
constexpr int kHistogramBinCount = 48;
constexpr int kHistogramTargetSamples = 16000;
constexpr int kHistogramPlaybackStride = 8;

QVariantList zeroHistogramBins()
{
    QVariantList bins;
    bins.reserve(kHistogramBinCount);
    for (int i = 0; i < kHistogramBinCount; ++i) {
        bins.append(0.0);
    }
    return bins;
}

QVariantList buildHistogramBins(const RawFrame &frame)
{
    QVariantList bins = zeroHistogramBins();
    if (!frame.valid || frame.width <= 0 || frame.height <= 0 || frame.pixels.isEmpty()) {
        return bins;
    }

    const int left = frame.hasActiveArea ? qBound(0, frame.activeLeft, frame.width - 1) : 0;
    const int top = frame.hasActiveArea ? qBound(0, frame.activeTop, frame.height - 1) : 0;
    const int right = frame.hasActiveArea ? qBound(left + 1, frame.activeRight, frame.width) : frame.width;
    const int bottom = frame.hasActiveArea ? qBound(top + 1, frame.activeBottom, frame.height) : frame.height;
    const int sampleWidth = qMax(1, right - left);
    const int sampleHeight = qMax(1, bottom - top);
    const double totalPixels = static_cast<double>(sampleWidth) * static_cast<double>(sampleHeight);
    const int sampleStep = qMax(1, static_cast<int>(std::sqrt(totalPixels / static_cast<double>(kHistogramTargetSamples))));
    const float range = qMax(1.0f, frame.whiteLevel - frame.blackLevel);

    std::array<int, kHistogramBinCount> counts {};
    int maxCount = 0;

    for (int y = top; y < bottom; y += sampleStep) {
        const int rowOffset = y * frame.width;
        for (int x = left; x < right; x += sampleStep) {
            const float normalized = qBound(0.0f,
                                            (static_cast<float>(frame.pixels.at(rowOffset + x)) - frame.blackLevel) / range,
                                            1.0f);
            const int binIndex = qMin(kHistogramBinCount - 1,
                                      static_cast<int>(normalized * static_cast<float>(kHistogramBinCount - 1)));
            const int newCount = ++counts[binIndex];
            if (newCount > maxCount) {
                maxCount = newCount;
            }
        }
    }

    if (maxCount <= 0) {
        return bins;
    }

    bins.clear();
    bins.reserve(kHistogramBinCount);
    for (int count : counts) {
        const double normalized = static_cast<double>(count) / static_cast<double>(maxCount);
        bins.append(std::pow(normalized, 0.65));
    }

    return bins;
}
}

PlaybackController::PlaybackController(QObject *parent)
    : QObject(parent)
{
    m_playbackTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_playbackTimer, &QTimer::timeout, this, &PlaybackController::nextFrame);
    m_fps = kFallbackPlaybackFps;
    m_histogramBins = zeroHistogramBins();
    updateTimerInterval();
    setStatusText(QStringLiteral("Select a clip to preview."));
}

QString PlaybackController::currentClipName() const
{
    return m_currentClipName;
}

QString PlaybackController::currentClipPath() const
{
    return m_currentClipPath;
}

int PlaybackController::frameCount() const
{
    return m_frameFiles.size();
}

int PlaybackController::currentFrameIndex() const
{
    return m_currentFrameIndex;
}

bool PlaybackController::playing() const
{
    return m_playing;
}

double PlaybackController::fps() const
{
    return m_fps;
}

QString PlaybackController::statusText() const
{
    return m_statusText;
}

QVariantList PlaybackController::histogramBins() const
{
    return m_histogramBins;
}

QVariantMap PlaybackController::clipMetadata() const
{
    return m_clipMetadata;
}

QString PlaybackController::frameSource() const
{
    if (m_currentFrameIndex < 0 || m_currentFrameIndex >= m_frameFiles.size()) {
        return QStringLiteral("image://cdng/empty");
    }

    const QString encodedPath = QString::fromLatin1(m_frameFiles.at(m_currentFrameIndex).toUtf8().toBase64(QByteArray::Base64UrlEncoding));
    return QStringLiteral("image://cdng/frame?path64=%1&token=%2").arg(encodedPath).arg(m_frameToken);
}

QString PlaybackController::fastFrameSource() const
{
    if (m_currentFrameIndex < 0 || m_currentFrameIndex >= m_frameFiles.size()) {
        return QStringLiteral("image://cdngplay/empty");
    }

    const QString encodedPath = QString::fromLatin1(m_frameFiles.at(m_currentFrameIndex).toUtf8().toBase64(QByteArray::Base64UrlEncoding));
    return QStringLiteral("image://cdngplay/frame?path64=%1").arg(encodedPath);
}

quint64 PlaybackController::currentFrameToken() const
{
    return m_frameToken;
}

void PlaybackController::loadClip(const QString &clipPath)
{
    reloadFrameList(clipPath);
}

void PlaybackController::togglePlayback()
{
    if (m_frameFiles.isEmpty()) {
        setStatusText(QStringLiteral("No frames to play in the selected clip."));
        return;
    }

    setPlaying(!m_playing);
}

void PlaybackController::stop()
{
    setPlaying(false);
    if (m_frameFiles.isEmpty()) {
        resetHistogram();
        return;
    }

    m_currentFrameIndex = 0;
    ++m_frameToken;
    emit currentFrameChanged();
    updateHistogram();
    setStatusText(QStringLiteral("Stopped on frame 1 of %1.").arg(m_frameFiles.size()));
}

void PlaybackController::nextFrame()
{
    if (m_frameFiles.isEmpty()) {
        return;
    }

    m_currentFrameIndex = (m_currentFrameIndex + 1) % m_frameFiles.size();
    ++m_frameToken;
    emit currentFrameChanged();
    if (!m_playing || (m_currentFrameIndex % kHistogramPlaybackStride) == 0) {
        updateHistogram();
    }
}

void PlaybackController::previousFrame()
{
    if (m_frameFiles.isEmpty()) {
        return;
    }

    m_currentFrameIndex = (m_currentFrameIndex - 1 + m_frameFiles.size()) % m_frameFiles.size();
    ++m_frameToken;
    emit currentFrameChanged();
    updateHistogram();
}

void PlaybackController::seekToFrame(int frameIndex)
{
    if (m_frameFiles.isEmpty()) {
        return;
    }

    const int clampedIndex = qBound(0, frameIndex, m_frameFiles.size() - 1);
    if (m_currentFrameIndex == clampedIndex) {
        return;
    }

    m_currentFrameIndex = clampedIndex;
    ++m_frameToken;
    emit currentFrameChanged();
    updateHistogram();

    if (!m_playing) {
        setStatusText(QStringLiteral("Paused %1 on frame %2 of %3.")
                          .arg(m_currentClipName)
                          .arg(m_currentFrameIndex + 1)
                          .arg(m_frameFiles.size()));
    }
}

void PlaybackController::setFps(double fps)
{
    const double clampedFps = qBound(1.0, fps, 60.0);
    if (qFuzzyCompare(m_fps, clampedFps)) {
        return;
    }

    m_fps = clampedFps;
    updateTimerInterval();
    emit fpsChanged();
    if (!m_frameFiles.isEmpty()) {
        updateClipMetadata();
    }
}

void PlaybackController::reloadFrameList(const QString &clipPath)
{
    const QFileInfo clipInfo(clipPath);
    const bool isSingleStill = clipInfo.isFile();
    const QDir clipDir(clipPath);

    if (!isSingleStill && !clipDir.exists()) {
        m_currentClipName = clipInfo.fileName();
        m_currentClipPath = clipPath;
        m_frameFiles.clear();
        m_histogramCache.clear();
        m_currentFrameIndex = -1;
        setClipMetadata({});
        ++m_frameToken;
        emit clipChanged();
        emit currentFrameChanged();
        resetHistogram();
        setPlaying(false);
        setStatusText(QStringLiteral("Clip folder is not available: %1").arg(clipPath));
        return;
    }

    m_frameFiles.clear();
    if (isSingleStill) {
        m_frameFiles.append(clipInfo.absoluteFilePath());
    } else {
        QStringList frames = clipDir.entryList(dngNameFilters(), QDir::Files, QDir::Name);
        QCollator collator;
        collator.setNumericMode(true);
        std::sort(frames.begin(), frames.end(), [&collator](const QString &lhs, const QString &rhs) {
            return collator.compare(lhs, rhs) < 0;
        });

        m_frameFiles.reserve(frames.size());
        for (const QString &frameName : frames) {
            m_frameFiles.append(clipDir.absoluteFilePath(frameName));
        }
    }

    m_histogramCache.clear();
    m_currentClipName = isSingleStill && !clipInfo.completeBaseName().isEmpty()
        ? clipInfo.completeBaseName()
        : clipInfo.fileName();
    m_currentClipPath = clipPath;
    m_currentFrameIndex = m_frameFiles.isEmpty() ? -1 : 0;
    const double detectedFps = m_frameFiles.isEmpty() ? 0.0 : DngDecoder::detectFrameRate(m_frameFiles.first());
    setFps(detectedFps > 0.0 ? detectedFps : kFallbackPlaybackFps);
    updateClipMetadata();
    emit clipChanged();
    setPlaying(false);

    if (m_frameFiles.isEmpty()) {
        ++m_frameToken;
        emit currentFrameChanged();
        resetHistogram();
        setStatusText(isSingleStill
                          ? QStringLiteral("Still file is not available: %1").arg(m_currentClipName)
                          : QStringLiteral("No DNG frames found in %1.").arg(m_currentClipName));
    } else {
        ++m_frameToken;
        emit currentFrameChanged();
        updateHistogram();
        if (isSingleStill) {
            setStatusText(QStringLiteral("Loaded still %1.").arg(m_currentClipName));
        } else if (detectedFps > 0.0) {
            setStatusText(QStringLiteral("Loaded %1 with %2 frame(s) at %3 fps.")
                              .arg(m_currentClipName)
                              .arg(m_frameFiles.size())
                              .arg(detectedFps, 0, 'f', 3));
        } else {
            setStatusText(QStringLiteral("Loaded %1 with %2 frame(s). Using %3 fps fallback.")
                              .arg(m_currentClipName)
                              .arg(m_frameFiles.size())
                              .arg(m_fps, 0, 'f', 3));
        }
    }
}

void PlaybackController::updateTimerInterval()
{
    const int intervalMs = qMax(1, qRound(1000.0 / m_fps));
    m_playbackTimer.setInterval(intervalMs);
}

void PlaybackController::setPlaying(bool playing)
{
    if (m_playing == playing) {
        return;
    }

    m_playing = playing;
    if (m_playing) {
        m_playbackTimer.start();
        setStatusText(QStringLiteral("Playing %1 at %2 fps.").arg(m_currentClipName).arg(m_fps, 0, 'f', 1));
    } else {
        m_playbackTimer.stop();
        updateHistogram();
        if (!m_currentClipName.isEmpty() && !m_frameFiles.isEmpty()) {
            setStatusText(QStringLiteral("Paused %1 on frame %2 of %3.")
                              .arg(m_currentClipName)
                              .arg(m_currentFrameIndex + 1)
                              .arg(m_frameFiles.size()));
        }
    }
    emit playingChanged();
}

void PlaybackController::setStatusText(const QString &statusText)
{
    if (m_statusText == statusText) {
        return;
    }

    m_statusText = statusText;
    emit statusTextChanged();
}

void PlaybackController::setClipMetadata(const QVariantMap &clipMetadata)
{
    if (m_clipMetadata == clipMetadata) {
        return;
    }

    m_clipMetadata = clipMetadata;
    emit clipMetadataChanged();
}

void PlaybackController::updateClipMetadata()
{
    if (m_currentClipPath.isEmpty() || m_frameFiles.isEmpty()) {
        setClipMetadata({});
        return;
    }

    const QFileInfo pathInfo(m_currentClipPath);
    const bool isSingleStill = pathInfo.isFile();
    const QString firstFramePath = m_frameFiles.first();
    const RawFrame frame = DngDecoder::decodeFile(firstFramePath);
    QVariantMap metadata;
    metadata.insert(QStringLiteral("type"), isSingleStill ? QStringLiteral("DNG still") : QStringLiteral("cDNG sequence"));
    metadata.insert(QStringLiteral("captured"), shotDateTimeForFrame(firstFramePath));
    metadata.insert(QStringLiteral("resolution"), resolutionTextForFrame(frame));
    metadata.insert(QStringLiteral("bitDepth"), bitDepthTextForFrame(frame));
    metadata.insert(QStringLiteral("frameRate"),
                    isSingleStill ? QStringLiteral("Single frame")
                                  : QStringLiteral("%1 fps").arg(m_fps, 0, 'f', 3));
    metadata.insert(QStringLiteral("duration"),
                    isSingleStill ? QStringLiteral("Still frame")
                                  : formatDurationText(m_frameFiles.size(), m_fps));
    metadata.insert(QStringLiteral("frames"), QString::number(m_frameFiles.size()));
    metadata.insert(QStringLiteral("clipSize"), formatByteSize(clipSizeBytes(m_currentClipPath)));
    metadata.insert(QStringLiteral("path"), m_currentClipPath);
    setClipMetadata(metadata);
}

void PlaybackController::updateHistogram()
{
    if (m_currentFrameIndex < 0 || m_currentFrameIndex >= m_frameFiles.size()) {
        resetHistogram();
        return;
    }

    const QString framePath = m_frameFiles.at(m_currentFrameIndex);
    const auto cached = m_histogramCache.constFind(framePath);
    if (cached != m_histogramCache.constEnd()) {
        if (m_histogramBins != cached.value()) {
            m_histogramBins = cached.value();
            emit histogramChanged();
        }
        return;
    }

    const QVariantList bins = buildHistogramBins(DngDecoder::decodeFile(framePath));
    m_histogramCache.insert(framePath, bins);
    if (m_histogramBins != bins) {
        m_histogramBins = bins;
        emit histogramChanged();
    }
}

void PlaybackController::resetHistogram()
{
    const QVariantList bins = zeroHistogramBins();
    if (m_histogramBins != bins) {
        m_histogramBins = bins;
        emit histogramChanged();
    }
}
