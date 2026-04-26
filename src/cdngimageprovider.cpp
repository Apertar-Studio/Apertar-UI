#include "cdngimageprovider.h"

#include <QFileInfo>
#include <QImage>
#include <QMutexLocker>
#include <QPainter>
#include <QPainterPath>
#include <QCollator>
#include <QDir>
#include <QRectF>
#include <QRunnable>
#include <QThreadPool>
#include <QUrlQuery>
#include <QVector>

#include <algorithm>
#include <array>
#include <cmath>

#include <tiffio.h>

#include "dngdecoder.h"
#include "tiffhelper.h"

namespace {
constexpr uint16_t kPhotometricCfa = 32803;
constexpr uint32_t kSubIfdTag = 330;
constexpr int kPreviewCacheLimit = 256;
constexpr int kPlaybackPrefetchCount = 8;
constexpr int kPlaybackPrefetchThreshold = 3;

QStringList dngNameFilters()
{
    return {QStringLiteral("*.dng"), QStringLiteral("*.DNG")};
}

struct WhiteBalanceGains
{
    float red = 1.0f;
    float green = 1.0f;
    float blue = 1.0f;
};

struct Matrix3x3
{
    float m[9] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };
};

constexpr Matrix3x3 kXyzToSrgb = {{
    3.2406f, -1.5372f, -0.4986f,
   -0.9689f,  1.8758f,  0.0415f,
    0.0557f, -0.2040f,  1.0570f
}};

WhiteBalanceGains previewDaylightGains()
{
    return {2.8f, 1.0f, 1.6f};
}

QImage scaleAndCrop(const QImage &image, const QSize &requestedSize)
{
    if (image.isNull() || !requestedSize.isValid()) {
        return image;
    }

    const int targetPixels = requestedSize.width() * requestedSize.height();
    const Qt::TransformationMode transformMode =
        targetPixels <= 160000 ? Qt::FastTransformation : Qt::SmoothTransformation;
    const QImage scaled = image.scaled(requestedSize, Qt::KeepAspectRatioByExpanding, transformMode);
    const int x = qMax(0, (scaled.width() - requestedSize.width()) / 2);
    const int y = qMax(0, (scaled.height() - requestedSize.height()) / 2);
    return scaled.copy(x, y, requestedSize.width(), requestedSize.height());
}

QImage applyRoundedCorners(const QImage &image, int radius)
{
    if (image.isNull() || radius <= 0) {
        return image;
    }

    QImage rounded(image.size(), QImage::Format_ARGB32_Premultiplied);
    rounded.fill(Qt::transparent);

    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;
    path.addRoundedRect(QRectF(0.0, 0.0, image.width(), image.height()), radius, radius);
    painter.setClipPath(path);
    painter.drawImage(QPoint(0, 0), image);
    return rounded;
}

bool isUsablePreviewDirectory(TIFF *tiff, uint32_t *outWidth = nullptr, uint32_t *outHeight = nullptr)
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t samplesPerPixel = 1;
    uint16_t photometric = 0;

    TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_PHOTOMETRIC, &photometric);

    if (outWidth) {
        *outWidth = width;
    }
    if (outHeight) {
        *outHeight = height;
    }

    return width > 0
           && height > 0
           && samplesPerPixel >= 3
           && photometric != kPhotometricCfa;
}

QImage imageFromCurrentDirectory(TIFF *tiff, const QSize &requestedSize)
{
    uint32_t width = 0;
    uint32_t height = 0;
    if (!isUsablePreviewDirectory(tiff, &width, &height)) {
        return {};
    }

    QVector<uint32_t> raster(width * height);
    const int ok = TIFFReadRGBAImageOriented(tiff,
                                             width,
                                             height,
                                             raster.data(),
                                             ORIENTATION_TOPLEFT,
                                             0);
    if (!ok) {
        return {};
    }

    QImage image(static_cast<int>(width), static_cast<int>(height), QImage::Format_RGB32);
    for (uint32_t y = 0; y < height; ++y) {
        QRgb *scanline = reinterpret_cast<QRgb *>(image.scanLine(static_cast<int>(y)));
        for (uint32_t x = 0; x < width; ++x) {
            const uint32_t pixel = raster.at(static_cast<int>(y * width + x));
            scanline[x] = qRgb(TIFFGetR(pixel), TIFFGetG(pixel), TIFFGetB(pixel));
        }
    }

    return scaleAndCrop(image, requestedSize);
}

QImage loadTopLevelPreview(const QString &filePath, const QSize &requestedSize)
{
    TIFF *tiff = openTiffWithCustomTags(filePath, "r");
    if (!tiff) {
        return {};
    }

    struct PreviewLocation {
        bool valid = false;
        bool isSubDirectory = false;
        tdir_t directoryIndex = 0;
        uint64_t subDirectoryOffset = 0;
        uint64_t pixelCount = 0;
    };

    PreviewLocation bestPreview;

    for (tdir_t dirIndex = 0; TIFFSetDirectory(tiff, dirIndex); ++dirIndex) {
        uint32_t width = 0;
        uint32_t height = 0;
        if (isUsablePreviewDirectory(tiff, &width, &height)) {
            const uint64_t pixels = static_cast<uint64_t>(width) * static_cast<uint64_t>(height);
            if (!bestPreview.valid || pixels > bestPreview.pixelCount) {
                bestPreview.valid = true;
                bestPreview.isSubDirectory = false;
                bestPreview.directoryIndex = dirIndex;
                bestPreview.subDirectoryOffset = 0;
                bestPreview.pixelCount = pixels;
            }
        }

        uint16_t subIfdCount = 0;
        uint64_t *subIfdOffsets = nullptr;
        if (TIFFGetField(tiff, kSubIfdTag, &subIfdCount, &subIfdOffsets) && subIfdCount > 0 && subIfdOffsets) {
            for (uint16_t subIndex = 0; subIndex < subIfdCount; ++subIndex) {
                if (!TIFFSetSubDirectory(tiff, static_cast<toff_t>(subIfdOffsets[subIndex]))) {
                    continue;
                }

                if (isUsablePreviewDirectory(tiff, &width, &height)) {
                    const uint64_t pixels = static_cast<uint64_t>(width) * static_cast<uint64_t>(height);
                    if (!bestPreview.valid || pixels > bestPreview.pixelCount) {
                        bestPreview.valid = true;
                        bestPreview.isSubDirectory = true;
                        bestPreview.directoryIndex = dirIndex;
                        bestPreview.subDirectoryOffset = subIfdOffsets[subIndex];
                        bestPreview.pixelCount = pixels;
                    }
                }

                TIFFSetDirectory(tiff, dirIndex);
            }
        }
    }

    QImage previewImage;
    if (bestPreview.valid) {
        if (bestPreview.isSubDirectory) {
            TIFFSetSubDirectory(tiff, static_cast<toff_t>(bestPreview.subDirectoryOffset));
        } else {
            TIFFSetDirectory(tiff, bestPreview.directoryIndex);
        }
        previewImage = imageFromCurrentDirectory(tiff, requestedSize);
    }

    TIFFClose(tiff);
    return previewImage;
}

QRectF aspectFillRect(int sourceWidth, int sourceHeight, const QSize &targetSize)
{
    if (sourceWidth <= 0 || sourceHeight <= 0 || !targetSize.isValid()) {
        return QRectF(0.0, 0.0, sourceWidth, sourceHeight);
    }

    const qreal sourceAspect = static_cast<qreal>(sourceWidth) / static_cast<qreal>(sourceHeight);
    const qreal targetAspect = static_cast<qreal>(targetSize.width()) / static_cast<qreal>(targetSize.height());

    if (targetAspect > sourceAspect) {
        const qreal cropHeight = static_cast<qreal>(sourceWidth) / targetAspect;
        const qreal top = (static_cast<qreal>(sourceHeight) - cropHeight) * 0.5;
        return QRectF(0.0, top, sourceWidth, cropHeight);
    }

    const qreal cropWidth = static_cast<qreal>(sourceHeight) * targetAspect;
    const qreal left = (static_cast<qreal>(sourceWidth) - cropWidth) * 0.5;
    return QRectF(left, 0.0, cropWidth, sourceHeight);
}

Matrix3x3 multiplyMatrix(const Matrix3x3 &left, const Matrix3x3 &right)
{
    Matrix3x3 result;
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            result.m[row * 3 + column] =
                left.m[row * 3] * right.m[column]
                + left.m[row * 3 + 1] * right.m[3 + column]
                + left.m[row * 3 + 2] * right.m[6 + column];
        }
    }
    return result;
}

bool invertMatrix(const Matrix3x3 &matrix, Matrix3x3 &inverse)
{
    const float a = matrix.m[0];
    const float b = matrix.m[1];
    const float c = matrix.m[2];
    const float d = matrix.m[3];
    const float e = matrix.m[4];
    const float f = matrix.m[5];
    const float g = matrix.m[6];
    const float h = matrix.m[7];
    const float i = matrix.m[8];

    const float det = a * (e * i - f * h)
                      - b * (d * i - f * g)
                      + c * (d * h - e * g);
    if (qFuzzyIsNull(det)) {
        return false;
    }

    const float invDet = 1.0f / det;
    inverse.m[0] = (e * i - f * h) * invDet;
    inverse.m[1] = (c * h - b * i) * invDet;
    inverse.m[2] = (b * f - c * e) * invDet;
    inverse.m[3] = (f * g - d * i) * invDet;
    inverse.m[4] = (a * i - c * g) * invDet;
    inverse.m[5] = (c * d - a * f) * invDet;
    inverse.m[6] = (d * h - e * g) * invDet;
    inverse.m[7] = (b * g - a * h) * invDet;
    inverse.m[8] = (a * e - b * d) * invDet;
    return true;
}

std::array<float, 3> multiplyMatrixVector(const Matrix3x3 &matrix, const std::array<float, 3> &vector)
{
    return {
        matrix.m[0] * vector[0] + matrix.m[1] * vector[1] + matrix.m[2] * vector[2],
        matrix.m[3] * vector[0] + matrix.m[4] * vector[1] + matrix.m[5] * vector[2],
        matrix.m[6] * vector[0] + matrix.m[7] * vector[1] + matrix.m[8] * vector[2]
    };
}

bool buildCameraToSrgbMatrix(const RawFrame &frame, Matrix3x3 &cameraToSrgb)
{
    if (!frame.hasColorMatrix1) {
        return false;
    }

    Matrix3x3 xyzToCamera;
    for (int i = 0; i < 9; ++i) {
        xyzToCamera.m[i] = frame.colorMatrix1[i];
    }

    Matrix3x3 cameraToXyz;
    if (!invertMatrix(xyzToCamera, cameraToXyz)) {
        return false;
    }

    cameraToSrgb = multiplyMatrix(kXyzToSrgb, cameraToXyz);
    return true;
}

WhiteBalanceGains previewWhiteBalance(const RawFrame &frame)
{
    if (frame.hasAsShotNeutral) {
        WhiteBalanceGains gains;
        gains.red = 1.0f / qMax(frame.asShotNeutral[0], 0.0001f);
        gains.green = 1.0f / qMax(frame.asShotNeutral[1], 0.0001f);
        gains.blue = 1.0f / qMax(frame.asShotNeutral[2], 0.0001f);

        if (frame.hasAnalogBalance) {
            gains.red *= frame.analogBalance[0];
            gains.green *= frame.analogBalance[1];
            gains.blue *= frame.analogBalance[2];
        }

        const float greenReference = qMax(gains.green, 0.0001f);
        gains.red /= greenReference;
        gains.green = 1.0f;
        gains.blue /= greenReference;
        return gains;
    }

    return previewDaylightGains();
}

std::array<float, 3> cameraRgbFromQuad(const RawFrame &frame,
                                       int x,
                                       int y,
                                       const std::function<float (int, int)> &normAt)
{
    const float p00 = normAt(x, y);
    const float p10 = normAt(x + 1, y);
    const float p01 = normAt(x, y + 1);
    const float p11 = normAt(x + 1, y + 1);

    switch (frame.cfaPattern) {
    case RawFrame::RGGB:
        return {p00, (p10 + p01) * 0.5f, p11};
    case RawFrame::BGGR:
        return {p11, (p10 + p01) * 0.5f, p00};
    case RawFrame::GRBG:
        return {p10, (p00 + p11) * 0.5f, p01};
    case RawFrame::GBRG:
        return {p01, (p00 + p11) * 0.5f, p10};
    }

    return {p00, (p10 + p01) * 0.5f, p11};
}

int gamma8(float value)
{
    const float gamma = std::pow(qBound(0.0f, value, 1.0f), 1.0f / 2.2f);
    return qBound(0, qRound(gamma * 255.0f), 255);
}

float smoothstep(float edge0, float edge1, float value)
{
    if (edge1 <= edge0) {
        return value >= edge1 ? 1.0f : 0.0f;
    }

    const float t = qBound(0.0f, (value - edge0) / (edge1 - edge0), 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

QImage renderRawPreview(const RawFrame &frame, const QSize &requestedSize)
{
    if (!frame.valid || frame.width <= 1 || frame.height <= 1 || frame.pixels.isEmpty()) {
        return {};
    }

    const int left = frame.hasActiveArea ? qBound(0, frame.activeLeft, frame.width - 2) : 0;
    const int top = frame.hasActiveArea ? qBound(0, frame.activeTop, frame.height - 2) : 0;
    const int right = frame.hasActiveArea ? qBound(left + 2, frame.activeRight, frame.width) : frame.width;
    const int bottom = frame.hasActiveArea ? qBound(top + 2, frame.activeBottom, frame.height) : frame.height;

    const int evenWidth = qMax(2, ((right - left) / 2) * 2);
    const int evenHeight = qMax(2, ((bottom - top) / 2) * 2);
    const int sourceQuadWidth = evenWidth / 2;
    const int sourceQuadHeight = evenHeight / 2;
    const QSize outputSize = requestedSize.isValid() ? requestedSize : QSize(sourceQuadWidth, sourceQuadHeight);
    const int outWidth = qMax(2, outputSize.width());
    const int outHeight = qMax(2, outputSize.height());
    if (outWidth <= 0 || outHeight <= 0) {
        return {};
    }

    const QRectF cropRect = aspectFillRect(sourceQuadWidth, sourceQuadHeight, QSize(outWidth, outHeight));

    auto normAt = [&](int px, int py) -> float {
        px = qBound(0, px, frame.width - 1);
        py = qBound(0, py, frame.height - 1);
        const int index = py * frame.width + px;
        const float range = qMax(1.0f, frame.whiteLevel - frame.blackLevel);
        return qBound(0.0f, (static_cast<float>(frame.pixels.at(index)) - frame.blackLevel) / range, 1.0f);
    };

    Matrix3x3 cameraToSrgb;
    const bool hasColorTransform = buildCameraToSrgbMatrix(frame, cameraToSrgb);
    const WhiteBalanceGains gains = previewWhiteBalance(frame);

    QVector<std::array<float, 3>> linearRgb;
    linearRgb.resize(outWidth * outHeight);
    QVector<float> sensorHighlights;
    sensorHighlights.resize(outWidth * outHeight);
    QVector<float> lumas;
    lumas.reserve(outWidth * outHeight);

    for (int y = 0; y < outHeight; ++y) {
        const qreal sampleY = cropRect.top() + (static_cast<qreal>(y) + 0.5) * cropRect.height() / static_cast<qreal>(outHeight);
        const int qy = qBound(0, static_cast<int>(sampleY), sourceQuadHeight - 1);
        const int sy = top + qy * 2;
        for (int x = 0; x < outWidth; ++x) {
            const qreal sampleX = cropRect.left() + (static_cast<qreal>(x) + 0.5) * cropRect.width() / static_cast<qreal>(outWidth);
            const int qx = qBound(0, static_cast<int>(sampleX), sourceQuadWidth - 1);
            const int sx = left + qx * 2;
            std::array<float, 3> camera = cameraRgbFromQuad(frame, sx, sy, normAt);
            sensorHighlights[y * outWidth + x] = qMax(camera[0], qMax(camera[1], camera[2]));
            std::array<float, 3> balanced = {
                camera[0] * gains.red,
                camera[1] * gains.green,
                camera[2] * gains.blue
            };
            std::array<float, 3> rgb;

            if (hasColorTransform) {
                rgb = multiplyMatrixVector(cameraToSrgb, balanced);
            } else {
                rgb = balanced;
            }

            linearRgb[y * outWidth + x] = rgb;
            const float luma = qMax(0.0f, 0.2126f * rgb[0] + 0.7152f * rgb[1] + 0.0722f * rgb[2]);
            lumas.append(luma);
        }
    }

    float scale = 1.0f;
    if (!lumas.isEmpty()) {
        std::sort(lumas.begin(), lumas.end());
        const int percentileIndex = qBound(0, static_cast<int>(lumas.size() * 0.995f), lumas.size() - 1);
        scale = 1.0f / qMax(lumas.at(percentileIndex), 0.0001f);
    }

    QImage image(outWidth, outHeight, QImage::Format_RGB32);
    constexpr float kSaturationBoost = 1.35f;

    for (int y = 0; y < outHeight; ++y) {
        QRgb *scanline = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < outWidth; ++x) {
            const int index = y * outWidth + x;
            std::array<float, 3> rgb = linearRgb.at(index);
            const float sensorHighlight = sensorHighlights.at(index);
            float r = qMax(0.0f, rgb[0] * scale);
            float g = qMax(0.0f, rgb[1] * scale);
            float b = qMax(0.0f, rgb[2] * scale);

            r = r / (1.0f + r);
            g = g / (1.0f + g);
            b = b / (1.0f + b);

            const float displayPeak = qMax(r, qMax(g, b));
            const float highlightBlend = qMax(smoothstep(0.88f, 1.0f, sensorHighlight),
                                              smoothstep(0.84f, 0.98f, displayPeak));
            const float displayLuma = 0.2126f * r + 0.7152f * g + 0.0722f * b;
            const float saturationBoost = 1.0f + (kSaturationBoost - 1.0f) * (1.0f - highlightBlend);
            r = qBound(0.0f, displayLuma + (r - displayLuma) * saturationBoost, 1.0f);
            g = qBound(0.0f, displayLuma + (g - displayLuma) * saturationBoost, 1.0f);
            b = qBound(0.0f, displayLuma + (b - displayLuma) * saturationBoost, 1.0f);

            const float whiteBlend = smoothstep(0.93f, 1.0f, sensorHighlight);
            const float whiteLevel = qBound(0.0f, qMax(displayLuma, qMax(r, qMax(g, b))), 1.0f);
            r = r * (1.0f - whiteBlend) + whiteLevel * whiteBlend;
            g = g * (1.0f - whiteBlend) + whiteLevel * whiteBlend;
            b = b * (1.0f - whiteBlend) + whiteLevel * whiteBlend;

            scanline[x] = qRgb(gamma8(r), gamma8(g), gamma8(b));
        }
    }

    return image;
}

QImage loadResolvedImage(const QString &filePath, const QSize &targetSize, bool previewOnly, int radius)
{
    if (filePath.isEmpty()) {
        return {};
    }

    QImage image;
    const QImage previewImage = loadTopLevelPreview(filePath, targetSize);
    if (previewOnly) {
        image = previewImage;
        if (image.isNull()) {
            const RawFrame frame = DngDecoder::decodeFile(filePath);
            if (frame.valid) {
                image = renderRawPreview(frame, targetSize);
            }
        }
    } else {
        const RawFrame frame = DngDecoder::decodeFile(filePath);
        if (frame.valid) {
            image = renderRawPreview(frame, targetSize);
        }
        if (image.isNull()) {
            image = previewImage;
        }
    }

    if (radius > 0 && !image.isNull()) {
        image = applyRoundedCorners(image, radius);
    }

    return image;
}

QStringList neighboringFramePaths(const QString &filePath, int limit)
{
    if (filePath.isEmpty() || limit <= 0)
        return {};

    const QFileInfo currentInfo(filePath);
    const QDir dir = currentInfo.dir();
    QStringList frames = dir.entryList(dngNameFilters(), QDir::Files, QDir::Name);
    if (frames.isEmpty())
        return {};

    QCollator collator;
    collator.setNumericMode(true);
    std::sort(frames.begin(), frames.end(), [&collator](const QString &lhs, const QString &rhs) {
        return collator.compare(lhs, rhs) < 0;
    });

    const int currentIndex = frames.indexOf(currentInfo.fileName());
    if (currentIndex < 0)
        return {};

    QStringList results;
    results.reserve(limit);
    for (int index = currentIndex + 1; index < frames.size() && results.size() < limit; ++index)
        results.append(dir.absoluteFilePath(frames.at(index)));
    return results;
}

bool resolvePrefetchClip(const QString &filePath,
                         const QSize &requestedSize,
                         int radius,
                         QString *clipKey,
                         QStringList *frames,
                         int *currentIndex)
{
    if (filePath.isEmpty() || !clipKey || !frames || !currentIndex)
        return false;

    const QFileInfo currentInfo(filePath);
    const QDir dir = currentInfo.dir();
    QStringList clipFrames = dir.entryList(dngNameFilters(), QDir::Files, QDir::Name);
    if (clipFrames.isEmpty())
        return false;

    QCollator collator;
    collator.setNumericMode(true);
    std::sort(clipFrames.begin(), clipFrames.end(), [&collator](const QString &lhs, const QString &rhs) {
        return collator.compare(lhs, rhs) < 0;
    });

    const int index = clipFrames.indexOf(currentInfo.fileName());
    if (index < 0)
        return false;

    for (QString &frameName : clipFrames)
        frameName = dir.absoluteFilePath(frameName);

    *clipKey = QStringLiteral("%1|%2x%3|r%4")
        .arg(dir.absolutePath())
        .arg(requestedSize.width())
        .arg(requestedSize.height())
        .arg(radius);
    *frames = std::move(clipFrames);
    *currentIndex = index;
    return true;
}
}

CdngImageProvider::CdngImageProvider(bool forceAsynchronousLoading, bool previewOnly, bool enablePrefetch)
    : QQuickImageProvider(QQuickImageProvider::Image,
                          forceAsynchronousLoading
                              ? QQuickImageProvider::ForceAsynchronousImageLoading
                              : QQuickImageProvider::Flags())
    , m_previewOnly(previewOnly)
    , m_enablePrefetch(enablePrefetch)
{
    m_prefetchThreadPool.setMaxThreadCount(1);
    m_prefetchThreadPool.setExpiryTimeout(-1);
}

QImage CdngImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const QSize targetSize = requestedSize.isValid() ? requestedSize : QSize(720, 720);
    const int queryStart = id.indexOf(QLatin1Char('?'));
    const QString queryString = queryStart >= 0 ? id.mid(queryStart + 1) : QString();
    const QUrlQuery query(queryString);
    const QByteArray pathBytes = QByteArray::fromBase64(query.queryItemValue(QStringLiteral("path64")).toLatin1(),
                                                        QByteArray::Base64UrlEncoding);
    const QString filePath = QString::fromUtf8(pathBytes);
    const int radius = qMax(0, query.queryItemValue(QStringLiteral("radius")).toInt());
    const QString cacheKey = (m_previewOnly && !filePath.isEmpty())
                                 ? cacheKeyForPath(filePath, targetSize, radius)
                                 : cacheKeyForId(id, targetSize);

    if (m_previewOnly) {
        QMutexLocker locker(&m_cacheMutex);
        const auto cached = m_thumbnailCache.constFind(cacheKey);
        if (cached != m_thumbnailCache.constEnd()) {
            if (size) {
                *size = cached->size();
            }
            return *cached;
        }
    }

    QImage image;

    if (id == QLatin1String("empty")) {
        image = makePlaceholder(QStringLiteral("Select a clip from /media/RAW"), targetSize);
    } else {
        if (filePath.isEmpty()) {
            image = makePlaceholder(QStringLiteral("No frame path provided"), targetSize);
        } else {
            image = loadResolvedImage(filePath, targetSize, m_previewOnly, radius);

            if (image.isNull()) {
                const RawFrame frame = m_previewOnly ? RawFrame() : DngDecoder::decodeFile(filePath);
                const QString message = frame.error.isEmpty()
                                            ? QStringLiteral("Could not render %1").arg(QFileInfo(filePath).fileName())
                                            : QStringLiteral("Decode failed: %1").arg(frame.error);
                image = makePlaceholder(message, targetSize);
            }
        }
    }

    if (size) {
        *size = image.size();
    }

    if (m_previewOnly && !image.isNull()) {
        insertCachedImage(cacheKey, image);
        if (m_enablePrefetch && !filePath.isEmpty())
            schedulePrefetch(filePath, targetSize, radius);
    }

    return image;
}

QString CdngImageProvider::cacheKeyForId(const QString &id, const QSize &requestedSize) const
{
    return QStringLiteral("%1|%2x%3").arg(id).arg(requestedSize.width()).arg(requestedSize.height());
}

QString CdngImageProvider::cacheKeyForPath(const QString &filePath, const QSize &requestedSize, int radius) const
{
    return QStringLiteral("%1|%2x%3|r%4")
        .arg(filePath)
        .arg(requestedSize.width())
        .arg(requestedSize.height())
        .arg(radius);
}

void CdngImageProvider::insertCachedImage(const QString &cacheKey, const QImage &image)
{
    if (image.isNull()) {
        return;
    }

    QMutexLocker locker(&m_cacheMutex);
    if (m_thumbnailCache.size() > kPreviewCacheLimit) {
        m_thumbnailCache.clear();
        m_prefetchFrontierByClip.clear();
        m_prefetchRequestsInFlight.clear();
    }
    m_thumbnailCache.insert(cacheKey, image);
}

void CdngImageProvider::schedulePrefetch(const QString &filePath, const QSize &requestedSize, int radius)
{
    if (!m_enablePrefetch || filePath.isEmpty())
        return;

    QString clipKey;
    QStringList clipFrames;
    int currentIndex = -1;
    if (!resolvePrefetchClip(filePath, requestedSize, radius, &clipKey, &clipFrames, &currentIndex))
        return;
    if (currentIndex >= clipFrames.size() - 1)
        return;

    int startIndex = -1;
    int endIndex = -1;
    {
        QMutexLocker locker(&m_cacheMutex);
        const int frontier = m_prefetchFrontierByClip.value(clipKey, currentIndex);
        if (currentIndex + kPlaybackPrefetchThreshold < frontier)
            return;
        if (m_prefetchRequestsInFlight.contains(clipKey))
            return;

        startIndex = qMax(currentIndex + 1, frontier + 1);
        endIndex = qMin(clipFrames.size() - 1, qMax(frontier, currentIndex) + kPlaybackPrefetchCount);
        if (startIndex > endIndex)
            return;

        m_prefetchRequestsInFlight.insert(clipKey);
    }

    m_prefetchThreadPool.start(QRunnable::create([this, clipKey, clipFrames, startIndex, endIndex, requestedSize, radius]() {
        for (int index = startIndex; index <= endIndex; ++index) {
            const QString &candidatePath = clipFrames.at(index);
            const QString candidateKey = cacheKeyForPath(candidatePath, requestedSize, radius);
            {
                QMutexLocker locker(&m_cacheMutex);
                if (m_thumbnailCache.contains(candidateKey))
                    continue;
            }

            const QImage image = loadResolvedImage(candidatePath, requestedSize, m_previewOnly, radius);
            if (!image.isNull())
                insertCachedImage(candidateKey, image);
        }

        QMutexLocker locker(&m_cacheMutex);
        const int previousFrontier = m_prefetchFrontierByClip.value(clipKey, -1);
        m_prefetchFrontierByClip.insert(clipKey, qMax(previousFrontier, endIndex));
        m_prefetchRequestsInFlight.remove(clipKey);
    }));
}

QImage CdngImageProvider::makePlaceholder(const QString &message, const QSize &requestedSize) const
{
    const QSize canvasSize = requestedSize.isValid() ? requestedSize : QSize(720, 720);
    QImage canvas(canvasSize, QImage::Format_RGB32);
    canvas.fill(QColor(14, 14, 14));

    QPainter painter(&canvas);
    painter.setPen(Qt::white);
    painter.setFont(QFont(QStringLiteral("Sans Serif"), 18));
    painter.drawText(canvas.rect().adjusted(24, 24, -24, -24), Qt::AlignCenter | Qt::TextWordWrap, message);
    return canvas;
}
