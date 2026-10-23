#include "cdngimageprovider.h"
#include "RawPreview.hpp"

#include <QFileInfo>
#include <QImage>
#include <QImageReader>
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
constexpr int kPlaybackPrefetchThreshold = 8;

QStringList dngNameFilters()
{
    return {QStringLiteral("*.dng"), QStringLiteral("*.DNG")};
}

QImage scaleAndFitOnBlack(const QImage &image, const QSize &requestedSize);
QImage loadTopLevelPreview(const QString &filePath, const QSize &requestedSize);

bool isDngFile(const QString &filePath)
{
    return QFileInfo(filePath).suffix().compare(QStringLiteral("dng"), Qt::CaseInsensitive) == 0;
}

bool isTiffFile(const QString &filePath)
{
    const QString suffix = QFileInfo(filePath).suffix();
    return suffix.compare(QStringLiteral("tif"), Qt::CaseInsensitive) == 0 ||
           suffix.compare(QStringLiteral("tiff"), Qt::CaseInsensitive) == 0;
}

QImage loadStandardImage(const QString &filePath, const QSize &targetSize)
{
    if (isTiffFile(filePath)) {
        const QImage tiffImage = loadTopLevelPreview(filePath, targetSize);
        if (!tiffImage.isNull())
            return tiffImage;
    }

    QImageReader reader(filePath);
    reader.setAutoTransform(true);

    QImage image;
    if (!reader.read(&image) || image.isNull())
        return {};

    if (targetSize.isValid() && targetSize.width() > 0 && targetSize.height() > 0)
        return scaleAndFitOnBlack(image, targetSize);
    return image;
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

constexpr Matrix3x3 kD50ToD65 = {{
     0.9555766f, -0.0230393f,  0.0631636f,
    -0.0282895f,  1.0099416f,  0.0210077f,
     0.0122982f, -0.0204830f,  1.3299098f
}};

WhiteBalanceGains previewDaylightGains()
{
    return {2.8f, 1.0f, 1.6f};
}

QImage scaleAndFitOnBlack(const QImage &image, const QSize &requestedSize)
{
    if (image.isNull() || !requestedSize.isValid()) {
        return image;
    }

    const QSize canvasSize(qMax(1, requestedSize.width()), qMax(1, requestedSize.height()));
    QSize scaledSize = image.size();
    scaledSize.scale(canvasSize, Qt::KeepAspectRatio);
    if (scaledSize.isEmpty()) {
        return {};
    }

    const int targetPixels = canvasSize.width() * canvasSize.height();
    const Qt::TransformationMode transformMode =
        targetPixels <= 160000 ? Qt::FastTransformation : Qt::SmoothTransformation;
    const QImage scaled = image.scaled(scaledSize, Qt::KeepAspectRatio, transformMode);

    QImage canvas(canvasSize, QImage::Format_RGB32);
    canvas.fill(Qt::black);

    QPainter painter(&canvas);
    painter.drawImage((canvas.width() - scaled.width()) / 2,
                      (canvas.height() - scaled.height()) / 2,
                      scaled);
    return canvas;
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

bool isRawCfaDirectory(TIFF *tiff)
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t samplesPerPixel = 1;
    uint16_t photometric = 0;

    TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_PHOTOMETRIC, &photometric);

    return width > 0
           && height > 0
           && samplesPerPixel == 1
           && photometric == kPhotometricCfa;
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

    return scaleAndFitOnBlack(image, requestedSize);
}

QImage loadTopLevelPreview(const QString &filePath, const QSize &requestedSize)
{
    TIFF *tiff = openTiffWithCustomTags(filePath, "r");
    if (!tiff) {
        return {};
    }

    uint16_t topLevelSubIfdCount = 0;
    uint64_t *topLevelSubIfdOffsets = nullptr;
    const bool hasTopLevelSubIfds =
        TIFFGetField(tiff, kSubIfdTag, &topLevelSubIfdCount, &topLevelSubIfdOffsets)
        && topLevelSubIfdCount > 0
        && topLevelSubIfdOffsets;
    if (isRawCfaDirectory(tiff) && !hasTopLevelSubIfds) {
        TIFFClose(tiff);
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

QRectF aspectFitRect(int sourceWidth, int sourceHeight, const QSize &targetSize)
{
    if (sourceWidth <= 0 || sourceHeight <= 0 || !targetSize.isValid()) {
        return QRectF();
    }

    const qreal sourceAspect = static_cast<qreal>(sourceWidth) / static_cast<qreal>(sourceHeight);
    const qreal targetAspect = static_cast<qreal>(targetSize.width()) / static_cast<qreal>(targetSize.height());

    if (targetAspect > sourceAspect) {
        const qreal fittedWidth = static_cast<qreal>(targetSize.height()) * sourceAspect;
        const qreal left = (static_cast<qreal>(targetSize.width()) - fittedWidth) * 0.5;
        return QRectF(left, 0.0, fittedWidth, targetSize.height());
    }

    const qreal fittedHeight = static_cast<qreal>(targetSize.width()) / sourceAspect;
    const qreal top = (static_cast<qreal>(targetSize.height()) - fittedHeight) * 0.5;
    return QRectF(0.0, top, targetSize.width(), fittedHeight);
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

float profileBlendForWhiteBalance(const RawFrame &frame)
{
    if (!frame.hasAsShotNeutral) {
        return 0.5f;
    }

    // Apertar writes AsShotNeutral from the camera CT curve. The blue/red
    // neutral ratio moves steadily from tungsten toward daylight, which is
    // enough for the lightweight playback preview to choose between profile
    // matrix 1 and 2 without a full DNG temperature solver.
    const float redNeutral = qMax(frame.asShotNeutral[0], 0.0001f);
    const float blueNeutral = qMax(frame.asShotNeutral[2], 0.0001f);
    const float ratio = blueNeutral / redNeutral;
    const float t = qBound(0.0f, (ratio - 0.30f) / (0.90f - 0.30f), 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

Matrix3x3 matrixFromArray(const float *values)
{
    Matrix3x3 matrix;
    for (int i = 0; i < 9; ++i) {
        matrix.m[i] = values[i];
    }
    return matrix;
}

Matrix3x3 blendedMatrix(const float *first, const float *second, float blend)
{
    Matrix3x3 matrix;
    for (int i = 0; i < 9; ++i) {
        matrix.m[i] = first[i] + (second[i] - first[i]) * blend;
    }
    return matrix;
}

bool buildCameraToSrgbMatrix(const RawFrame &frame, Matrix3x3 &cameraToSrgb)
{
    const float blend = profileBlendForWhiteBalance(frame);

    if (frame.hasForwardMatrix1 || frame.hasForwardMatrix2) {
        Matrix3x3 cameraToXyzD50;
        if (frame.hasForwardMatrix1 && frame.hasForwardMatrix2) {
            cameraToXyzD50 = blendedMatrix(frame.forwardMatrix1, frame.forwardMatrix2, blend);
        } else {
            cameraToXyzD50 = matrixFromArray(frame.hasForwardMatrix1 ? frame.forwardMatrix1 : frame.forwardMatrix2);
        }

        const Matrix3x3 cameraToXyzD65 = multiplyMatrix(kD50ToD65, cameraToXyzD50);
        cameraToSrgb = multiplyMatrix(kXyzToSrgb, cameraToXyzD65);
        return true;
    }

    if (!frame.hasColorMatrix1 && !frame.hasColorMatrix2) {
        return false;
    }

    Matrix3x3 xyzToCamera;
    if (frame.hasColorMatrix1 && frame.hasColorMatrix2) {
        xyzToCamera = blendedMatrix(frame.colorMatrix1, frame.colorMatrix2, blend);
    } else {
        xyzToCamera = matrixFromArray(frame.hasColorMatrix1 ? frame.colorMatrix1 : frame.colorMatrix2);
    }

    Matrix3x3 cameraToXyzD50;
    if (!invertMatrix(xyzToCamera, cameraToXyzD50)) {
        return false;
    }

    const Matrix3x3 cameraToXyzD65 = multiplyMatrix(kD50ToD65, cameraToXyzD50);
    cameraToSrgb = multiplyMatrix(kXyzToSrgb, cameraToXyzD65);
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

template <typename Normalize>
std::array<float, 3> cameraRgbFromQuad(const RawFrame &frame,
                                       int x,
                                       int y,
                                       const Normalize &normAt)
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

template <typename Normalize>
std::array<float, 3> interpolateCameraRgb(const RawFrame &frame,
                                          qreal sampleX,
                                          qreal sampleY,
                                          int left,
                                          int top,
                                          int sourceQuadWidth,
                                          int sourceQuadHeight,
                                          const Normalize &normAt)
{
    const qreal clampedX = qBound<qreal>(0.0, sampleX, static_cast<qreal>(sourceQuadWidth - 1));
    const qreal clampedY = qBound<qreal>(0.0, sampleY, static_cast<qreal>(sourceQuadHeight - 1));
    const int qx0 = qBound(0, static_cast<int>(std::floor(clampedX)), sourceQuadWidth - 1);
    const int qy0 = qBound(0, static_cast<int>(std::floor(clampedY)), sourceQuadHeight - 1);
    const int qx1 = qMin(qx0 + 1, sourceQuadWidth - 1);
    const int qy1 = qMin(qy0 + 1, sourceQuadHeight - 1);
    const float tx = static_cast<float>(clampedX - static_cast<qreal>(qx0));
    const float ty = static_cast<float>(clampedY - static_cast<qreal>(qy0));

    const auto c00 = cameraRgbFromQuad(frame, left + qx0 * 2, top + qy0 * 2, normAt);
    const auto c10 = cameraRgbFromQuad(frame, left + qx1 * 2, top + qy0 * 2, normAt);
    const auto c01 = cameraRgbFromQuad(frame, left + qx0 * 2, top + qy1 * 2, normAt);
    const auto c11 = cameraRgbFromQuad(frame, left + qx1 * 2, top + qy1 * 2, normAt);

    std::array<float, 3> result;
    for (int channel = 0; channel < 3; ++channel) {
        const float topBlend = c00[channel] + (c10[channel] - c00[channel]) * tx;
        const float bottomBlend = c01[channel] + (c11[channel] - c01[channel]) * tx;
        result[channel] = topBlend + (bottomBlend - topBlend) * ty;
    }
    return result;
}

int gamma8(float value)
{
    constexpr int kGammaLutSize = 4096;
    static const std::array<uint8_t, kGammaLutSize + 1> lut = []() {
        std::array<uint8_t, kGammaLutSize + 1> values{};
        for (int i = 0; i <= kGammaLutSize; ++i) {
            const float normalized = static_cast<float>(i) / kGammaLutSize;
            values[static_cast<size_t>(i)] = static_cast<uint8_t>(
                qBound(0, qRound(std::pow(normalized, 1.0f / 2.2f) * 255.0f), 255));
        }
        return values;
    }();

    const int index = qBound(0, qRound(value * kGammaLutSize), kGammaLutSize);
    return lut[static_cast<size_t>(index)];
}

int srgb8(float value)
{
    static const std::array<uint8_t, 4097> lut = [] {
        std::array<uint8_t, 4097> values {};
        for (size_t i = 0; i < values.size(); ++i) {
            const float linear = float(i) / 4096.0f;
            const float encoded = linear <= 0.0031308f ? 12.92f * linear
                : 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
            values[i] = qBound(0, qRound(encoded * 255.0f), 255);
        }
        return values;
    }();
    return lut[qBound(0, qRound(value * 4096.0f), 4096)];
}

float smoothstep(float edge0, float edge1, float value)
{
    if (edge1 <= edge0) {
        return value >= edge1 ? 1.0f : 0.0f;
    }

    const float t = qBound(0.0f, (value - edge0) / (edge1 - edge0), 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

template <typename Sample>
QImage renderRawPreviewImpl(const RawFrame &frame, const QSize &requestedSize, const Sample &sample,
                           const float *cameraToLinearSrgb = nullptr, float digitalGain = 1.0f)
{
    if (!frame.valid || frame.width <= 1 || frame.height <= 1) {
        return {};
    }

    const int left = frame.hasActiveArea ? qBound(0, frame.activeLeft, frame.width - 2) : 0;
    const int top = frame.hasActiveArea ? qBound(0, frame.activeTop, frame.height - 2) : 0;
    const int right = frame.hasActiveArea ? qBound(left + 2, frame.activeRight, frame.width) : frame.width;
    const int bottom = frame.hasActiveArea ? qBound(top + 2, frame.activeBottom, frame.height) : frame.height;

    const int evenWidth = qMax(2, ((right - left) / 2) * 2);
    const int evenHeight = qMax(2, ((bottom - top) / 2) * 2);
    const int sourceQuadWidth = frame.monochrome ? right - left : evenWidth / 2;
    const int sourceQuadHeight = frame.monochrome ? bottom - top : evenHeight / 2;
    const QSize outputSize = requestedSize.isValid() ? requestedSize : QSize(sourceQuadWidth, sourceQuadHeight);
    const int outWidth = qMax(2, outputSize.width());
    const int outHeight = qMax(2, outputSize.height());
    if (outWidth <= 0 || outHeight <= 0) {
        return {};
    }

    const QRectF fitRect = aspectFitRect(sourceQuadWidth, sourceQuadHeight, QSize(outWidth, outHeight));
    const int drawLeft = qBound(0, qRound(fitRect.left()), outWidth - 1);
    const int drawTop = qBound(0, qRound(fitRect.top()), outHeight - 1);
    const int drawWidth = qBound(1, qRound(fitRect.width()), outWidth - drawLeft);
    const int drawHeight = qBound(1, qRound(fitRect.height()), outHeight - drawTop);

    const float inverseRange = 1.0f / qMax(1.0f, frame.whiteLevel - frame.blackLevel);
    auto normAt = [&](int px, int py) -> float {
        return qBound(0.0f,
                      (static_cast<float>(sample(px, py)) - frame.blackLevel) * inverseRange,
                      1.0f);
    };

    Matrix3x3 cameraToSrgb;
    const bool ispColor = cameraToLinearSrgb != nullptr;
    const bool hasColorTransform = !frame.monochrome && (ispColor || buildCameraToSrgbMatrix(frame, cameraToSrgb));
    if (ispColor) cameraToSrgb = matrixFromArray(cameraToLinearSrgb);
    const WhiteBalanceGains gains = previewWhiteBalance(frame);
    const float ispCommonGain = digitalGain / qMax(0.1f, qMin(1.0f, qMin(gains.red, gains.blue)));
    constexpr float kPreviewExposureScale = 1.0f;
    QImage image(outWidth, outHeight, QImage::Format_RGB32);
    image.fill(Qt::black);
    constexpr float kSaturationBoost = 1.35f;

    for (int y = 0; y < drawHeight; ++y) {
        QRgb *scanline = reinterpret_cast<QRgb *>(image.scanLine(drawTop + y));
        const qreal sampleY = (static_cast<qreal>(y) + 0.5) * sourceQuadHeight / static_cast<qreal>(drawHeight);
        for (int x = 0; x < drawWidth; ++x) {
            const qreal sampleX = (static_cast<qreal>(x) + 0.5) * sourceQuadWidth / static_cast<qreal>(drawWidth);
            if (frame.monochrome) {
                // Mono samples already contain the complete image: no CFA,
                // white balance or colour matrix should be applied.
                const int px = qBound(left, left + int(sampleX), right - 1);
                const int py = qBound(top, top + int(sampleY), bottom - 1);
                const int grey = srgb8(normAt(px, py));
                scanline[drawLeft + x] = qRgb(grey, grey, grey);
                continue;
            }
            std::array<float, 3> camera = interpolateCameraRgb(frame,
                                                               sampleX,
                                                               sampleY,
                                                               left,
                                                               top,
                                                               sourceQuadWidth,
                                                               sourceQuadHeight,
                                                               normAt);
            const float sensorHighlight = qMax(camera[0], qMax(camera[1], camera[2]));
            std::array<float, 3> balanced = {
                camera[0] * gains.red,
                camera[1] * gains.green,
                camera[2] * gains.blue
            };
            std::array<float, 3> rgb;

            if (ispColor) {
                // The ISP clips balanced channels before its RGB CCM. Apply
                // that recorded CCM directly, without an XYZ inverse or CAT.
                for (float &channel : balanced)
                    channel = qBound(0.0f, channel * ispCommonGain, 1.0f);
                rgb = multiplyMatrixVector(cameraToSrgb, balanced);
                scanline[drawLeft + x] = qRgb(srgb8(rgb[0]), srgb8(rgb[1]), srgb8(rgb[2]));
                continue;
            }

            if (hasColorTransform) {
                rgb = multiplyMatrixVector(cameraToSrgb, balanced);
            } else {
                rgb = balanced;
            }
            float r = qMax(0.0f, rgb[0] * kPreviewExposureScale);
            float g = qMax(0.0f, rgb[1] * kPreviewExposureScale);
            float b = qMax(0.0f, rgb[2] * kPreviewExposureScale);

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

            scanline[drawLeft + x] = qRgb(gamma8(r), gamma8(g), gamma8(b));
        }
    }

    return image;
}

QImage renderRawPreview(const RawFrame &frame, const QSize &size)
{
    if (frame.pixels.isEmpty()) return {};
    const auto *pixels = frame.pixels.constData();
    return renderRawPreviewImpl(frame, size, [&](int x, int y) { return pixels[y * frame.width + x]; });
}

const RawFrame &decodeReusableFrame(const QString &filePath)
{
    static thread_local RawFrame scratchFrame;
    DngDecoder::decodeFileInto(filePath, scratchFrame);
    return scratchFrame;
}

QImage loadResolvedImage(const QString &filePath,
                         const QSize &targetSize,
                         bool previewOnly,
                         int radius,
                         QString *decodeError = nullptr)
{
    if (filePath.isEmpty()) {
        return {};
    }

    if (!isDngFile(filePath)) {
        QImage image = loadStandardImage(filePath, targetSize);
        if (radius > 0 && !image.isNull())
            image = applyRoundedCorners(image, radius);
        if (image.isNull() && decodeError)
            *decodeError = QStringLiteral("Could not open image file.");
        return image;
    }

    QImage image;
    if (previewOnly) {
        const RawFrame &frame = decodeReusableFrame(filePath);
        if (frame.valid) {
            image = renderRawPreview(frame, targetSize);
        } else {
            image = loadTopLevelPreview(filePath, targetSize);
            if (image.isNull() && decodeError) {
                *decodeError = frame.error;
            }
        }
    } else {
        const RawFrame &frame = decodeReusableFrame(filePath);
        if (frame.valid) {
            image = renderRawPreview(frame, targetSize);
        } else if (decodeError) {
            *decodeError = frame.error;
        }
        if (image.isNull()) {
            const QImage previewImage = loadTopLevelPreview(filePath, targetSize);
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

QImage renderRawFramePreview(const RawFrame &frame, const QSize &size,
                             RawPreviewSample sample, const void *context,
                             const float *cameraToLinearSrgb, float digitalGain)
{
    if (!sample) {
        if (frame.pixels.isEmpty()) return {};
        return renderRawPreviewImpl(frame, size, [&](int x, int y) { return frame.pixels[y * frame.width + x]; },
                                    cameraToLinearSrgb, digitalGain);
    }
    return renderRawPreviewImpl(frame, size, [&](int x, int y) { return sample(context, x, y); },
                                cameraToLinearSrgb, digitalGain);
}

CdngImageProvider::CdngImageProvider(bool forceAsynchronousLoading,
                                     bool previewOnly,
                                     bool enablePrefetch,
                                     int maxCachedImages,
                                     int playbackPrefetchCount,
                                     qint64 maxCachedBytes)
    : QQuickImageProvider(QQuickImageProvider::Image,
                              forceAsynchronousLoading
                              ? QQuickImageProvider::ForceAsynchronousImageLoading
                              : QQuickImageProvider::Flags())
    , m_previewOnly(previewOnly)
    , m_enablePrefetch(enablePrefetch)
    , m_maxCachedImages(std::max(1, maxCachedImages))
    , m_playbackPrefetchCount(std::max(1, playbackPrefetchCount))
    , m_maxCachedBytes(std::max<qint64>(0, maxCachedBytes))
{
    m_prefetchThreadPool.setMaxThreadCount(m_enablePrefetch ? 2 : 1);
    m_prefetchThreadPool.setExpiryTimeout(10000);
}

void CdngImageProvider::clearCache()
{
    m_cacheEpoch.fetch_add(1, std::memory_order_relaxed);
    m_prefetchThreadPool.clear();

    QMutexLocker locker(&m_cacheMutex);
    m_thumbnailCache.clear();
    m_cacheOrder.clear();
    m_cachedBytes = 0;
    m_prefetchFrontierByClip.clear();
    m_prefetchRequestsInFlight.clear();
}

void CdngImageProvider::prefetchAround(const QString &filePath,
                                       int width,
                                       int height,
                                       int radius,
                                       int count)
{
    if (!m_enablePrefetch || filePath.isEmpty() || count <= 0)
        return;

    const QSize requestedSize(qMax(2, width), qMax(2, height));

    QString clipKey;
    QStringList clipFrames;
    int currentIndex = -1;
    if (!resolvePrefetchClip(filePath, requestedSize, radius, &clipKey, &clipFrames, &currentIndex))
        return;

    const int startIndex = currentIndex;
    const int endIndex = qMin(clipFrames.size() - 1, currentIndex + count - 1);
    if (startIndex < 0 || startIndex > endIndex)
        return;
    const uint64_t cacheEpoch = m_cacheEpoch.load(std::memory_order_relaxed);

    {
        QMutexLocker locker(&m_cacheMutex);
        if (m_prefetchRequestsInFlight.contains(clipKey))
            return;
        m_prefetchRequestsInFlight.insert(clipKey);
    }

    m_prefetchThreadPool.start(QRunnable::create([this, clipKey, clipFrames, startIndex, endIndex, requestedSize, radius, cacheEpoch]() {
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
                insertCachedImage(candidateKey, image, cacheEpoch);
        }

        QMutexLocker locker(&m_cacheMutex);
        if (cacheEpoch == m_cacheEpoch.load(std::memory_order_relaxed)) {
            const int previousFrontier = m_prefetchFrontierByClip.value(clipKey, -1);
            m_prefetchFrontierByClip.insert(clipKey, qMax(previousFrontier, endIndex));
        }
        m_prefetchRequestsInFlight.remove(clipKey);
    }));
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
    const uint64_t cacheEpoch = m_cacheEpoch.load(std::memory_order_relaxed);

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
            QString decodeError;
            image = loadResolvedImage(filePath, targetSize, m_previewOnly, radius, &decodeError);

            if (image.isNull()) {
                const QString message = decodeError.isEmpty()
                                            ? QStringLiteral("Could not render %1").arg(QFileInfo(filePath).fileName())
                                            : QStringLiteral("Decode failed: %1").arg(decodeError);
                image = makePlaceholder(message, targetSize);
            }
        }
    }

    if (size) {
        *size = image.size();
    }

    if (m_previewOnly && !image.isNull()) {
        insertCachedImage(cacheKey, image, cacheEpoch);
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

void CdngImageProvider::insertCachedImage(const QString &cacheKey, const QImage &image, uint64_t cacheEpoch)
{
    if (image.isNull()) {
        return;
    }

    QMutexLocker locker(&m_cacheMutex);
    if (cacheEpoch != m_cacheEpoch.load(std::memory_order_relaxed))
        return;

    m_cacheOrder.removeAll(cacheKey);
    m_cacheOrder.append(cacheKey);

    const auto existing = m_thumbnailCache.constFind(cacheKey);
    if (existing != m_thumbnailCache.constEnd())
        m_cachedBytes -= static_cast<qint64>(existing.value().sizeInBytes());
    m_thumbnailCache.insert(cacheKey, image);
    m_cachedBytes += static_cast<qint64>(image.sizeInBytes());

    while ((m_thumbnailCache.size() > m_maxCachedImages
            || (m_maxCachedBytes > 0 && m_cachedBytes > m_maxCachedBytes))
           && m_cacheOrder.size() > 1) {
        const QString oldestKey = m_cacheOrder.takeFirst();
        const auto oldest = m_thumbnailCache.find(oldestKey);
        if (oldest == m_thumbnailCache.end())
            continue;
        m_cachedBytes -= static_cast<qint64>(oldest.value().sizeInBytes());
        m_thumbnailCache.erase(oldest);
    }
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
    const uint64_t cacheEpoch = m_cacheEpoch.load(std::memory_order_relaxed);
    {
        QMutexLocker locker(&m_cacheMutex);
        const int frontier = m_prefetchFrontierByClip.value(clipKey, currentIndex);
        if (currentIndex + kPlaybackPrefetchThreshold < frontier)
            return;
        if (m_prefetchRequestsInFlight.contains(clipKey))
            return;

        startIndex = qMax(currentIndex + 1, frontier + 1);
        endIndex = qMin(clipFrames.size() - 1, qMax(frontier, currentIndex) + m_playbackPrefetchCount);
        if (startIndex > endIndex)
            return;

        m_prefetchRequestsInFlight.insert(clipKey);
    }

    m_prefetchThreadPool.start(QRunnable::create([this, clipKey, clipFrames, startIndex, endIndex, requestedSize, radius, cacheEpoch]() {
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
                insertCachedImage(candidateKey, image, cacheEpoch);
        }

        QMutexLocker locker(&m_cacheMutex);
        if (cacheEpoch == m_cacheEpoch.load(std::memory_order_relaxed)) {
            const int previousFrontier = m_prefetchFrontierByClip.value(clipKey, -1);
            m_prefetchFrontierByClip.insert(clipKey, qMax(previousFrontier, endIndex));
        }
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
