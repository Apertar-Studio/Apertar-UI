#include "dngdecoder.h"

#include <QByteArray>
#include <QFile>
#include <QVariantList>
#include <QtGlobal>
#include <QtEndian>

#if defined(__aarch64__) && defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#include <tiffio.h>

#include "tiffhelper.h"
#include "../third_party/liblj92/lj92.h"
#include <limits>

namespace {
constexpr uint16_t kPhotometricCfa = 32803;
constexpr uint16_t kPhotometricLinearRaw = 34892;
constexpr uint32_t kCfaRepeatPatternDimTag = 33421;
constexpr uint32_t kCfaPatternTag = 33422;
constexpr uint32_t kBlackLevelTag = 50714;
constexpr uint32_t kWhiteLevelTag = 50717;
constexpr uint32_t kLinearizationTableTag = 50712;
constexpr uint32_t kColorMatrix1Tag = 50721;
constexpr uint32_t kColorMatrix2Tag = 50722;
constexpr uint32_t kAnalogBalanceTag = 50727;
constexpr uint32_t kSubIfdTag = 330;
constexpr uint32_t kActiveAreaTag = 50829;
constexpr uint32_t kAsShotNeutralTag = 50728;
constexpr uint32_t kForwardMatrix1Tag = 0xC714;
constexpr uint32_t kForwardMatrix2Tag = 0xC715;
constexpr uint32_t kFrameRateTag = 0xC764;

enum class TiffByteOrder {
    LittleEndian,
    BigEndian
};

struct TiffEntry
{
    uint16_t tag = 0;
    uint16_t type = 0;
    quint32 count = 0;
    QByteArray payload;
};

bool decodeJpegStrip(const uchar *data, quint32 size, int width, int rows, int bits, quint16 *target)
{
    if (size > static_cast<quint32>(std::numeric_limits<int>::max()))
        return false;
    lj92 decoder = nullptr;
    int jpegWidth = 0, jpegHeight = 0, precision = 0, components = 0;
    const int opened = lj92_open(&decoder, data, static_cast<int>(size),
                                 &jpegWidth, &jpegHeight, &precision, &components);
    if (opened != LJ92_ERROR_NONE)
        return false;
    const bool geometry = static_cast<qint64>(jpegWidth) * components == width &&
                          jpegHeight == rows && precision == bits;
    const bool ok = geometry && lj92_decode(decoder, target, width, 0, nullptr, 0) == LJ92_ERROR_NONE;
    lj92_close(decoder);
    return ok;
}

int mapCfaPattern(const QByteArray &pattern);
void unpackSamples(const uchar *data, qsizetype dataSize, int width, int bitsPerSample, quint16 *target);
int packedRowBytes(int width, int bitsPerSample);
bool selectRawDirectory(TIFF *tiff);
float firstFloatTagValue(TIFF *tiff, uint32_t tag, float fallback);
bool readScanline(TIFF *tiff, uint32_t row, QByteArray &buffer);
QVector<int> readLongArrayTag(TIFF *tiff, uint32_t tag);

#if defined(__aarch64__) && defined(__ARM_NEON)
alignas(16) constexpr uchar kPacked10BitIdx0[16] = {0, 5, 10, 15, 20, 25, 30, 35, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
alignas(16) constexpr uchar kPacked10BitIdx1[16] = {1, 6, 11, 16, 21, 26, 31, 36, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
alignas(16) constexpr uchar kPacked10BitIdx2[16] = {2, 7, 12, 17, 22, 27, 32, 37, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
alignas(16) constexpr uchar kPacked10BitIdx3[16] = {3, 8, 13, 18, 23, 28, 33, 38, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
alignas(16) constexpr uchar kPacked10BitIdx4[16] = {4, 9, 14, 19, 24, 29, 34, 39, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

inline int unpack10BitNeon(const uchar *data, qsizetype dataSize, int width, quint16 *target)
{
    int src = 0;
    int dst = 0;
    const uint8x16_t idx0 = vld1q_u8(kPacked10BitIdx0);
    const uint8x16_t idx1 = vld1q_u8(kPacked10BitIdx1);
    const uint8x16_t idx2 = vld1q_u8(kPacked10BitIdx2);
    const uint8x16_t idx3 = vld1q_u8(kPacked10BitIdx3);
    const uint8x16_t idx4 = vld1q_u8(kPacked10BitIdx4);
    const uint16x8_t mask03 = vdupq_n_u16(0x03);
    const uint16x8_t mask0f = vdupq_n_u16(0x0F);
    const uint16x8_t mask3f = vdupq_n_u16(0x3F);

    while (dst + 31 < width && src + 39 < dataSize) {
        uint8x16x3_t table;
        table.val[0] = vld1q_u8(data + src);
        table.val[1] = vld1q_u8(data + src + 16);
        table.val[2] = vcombine_u8(vld1_u8(data + src + 32), vdup_n_u8(0));

        const uint8x8_t b0 = vget_low_u8(vqtbl3q_u8(table, idx0));
        const uint8x8_t b1 = vget_low_u8(vqtbl3q_u8(table, idx1));
        const uint8x8_t b2 = vget_low_u8(vqtbl3q_u8(table, idx2));
        const uint8x8_t b3 = vget_low_u8(vqtbl3q_u8(table, idx3));
        const uint8x8_t b4 = vget_low_u8(vqtbl3q_u8(table, idx4));

        const uint16x8_t b0w = vmovl_u8(b0);
        const uint16x8_t b1w = vmovl_u8(b1);
        const uint16x8_t b2w = vmovl_u8(b2);
        const uint16x8_t b3w = vmovl_u8(b3);
        const uint16x8_t b4w = vmovl_u8(b4);

        uint16x8x4_t samples;
        samples.val[0] = vorrq_u16(vshlq_n_u16(b0w, 2), vshrq_n_u16(b1w, 6));
        samples.val[1] = vorrq_u16(vshlq_n_u16(vandq_u16(b1w, mask3f), 4), vshrq_n_u16(b2w, 4));
        samples.val[2] = vorrq_u16(vshlq_n_u16(vandq_u16(b2w, mask0f), 6), vshrq_n_u16(b3w, 2));
        samples.val[3] = vorrq_u16(vshlq_n_u16(vandq_u16(b3w, mask03), 8), b4w);
        vst4q_u16(target + dst, samples);

        src += 40;
        dst += 32;
    }

    return dst;
}

inline int unpack12BitNeon(const uchar *data, qsizetype dataSize, int width, quint16 *target)
{
    int src = 0;
    int dst = 0;
    const uint16x8_t lowNibbleMask = vdupq_n_u16(0x0F);

    while (dst + 15 < width && src + 23 < dataSize) {
        const uint8x8x3_t triples = vld3_u8(data + src);
        const uint16x8_t b0 = vmovl_u8(triples.val[0]);
        const uint16x8_t b1 = vmovl_u8(triples.val[1]);
        const uint16x8_t b2 = vmovl_u8(triples.val[2]);

        uint16x8x2_t samples;
        samples.val[0] = vorrq_u16(vshlq_n_u16(b0, 4), vshrq_n_u16(b1, 4));
        samples.val[1] = vorrq_u16(vshlq_n_u16(vandq_u16(b1, lowNibbleMask), 8), b2);
        vst2q_u16(target + dst, samples);

        src += 24;
        dst += 16;
    }

    return dst;
}
#endif

int tiffTypeSize(uint16_t type)
{
    switch (type) {
    case 1:
    case 2:
    case 6:
    case 7:
        return 1;
    case 3:
    case 8:
        return 2;
    case 4:
    case 9:
    case 13:
        return 4;
    case 5:
    case 10:
    case 12:
        return 8;
    case 11:
        return 4;
    default:
        return 0;
    }
}

quint16 readTiffUInt16(const QByteArray &data, int offset, TiffByteOrder order)
{
    if (offset < 0 || offset + 2 > data.size()) {
        return 0;
    }

    const uchar *bytes = reinterpret_cast<const uchar *>(data.constData() + offset);
    if (order == TiffByteOrder::LittleEndian) {
        return static_cast<quint16>(bytes[0] | (bytes[1] << 8));
    }
    return static_cast<quint16>((bytes[0] << 8) | bytes[1]);
}

quint32 readTiffUInt32(const QByteArray &data, int offset, TiffByteOrder order)
{
    if (offset < 0 || offset + 4 > data.size()) {
        return 0;
    }

    const uchar *bytes = reinterpret_cast<const uchar *>(data.constData() + offset);
    if (order == TiffByteOrder::LittleEndian) {
        return static_cast<quint32>(bytes[0])
               | (static_cast<quint32>(bytes[1]) << 8)
               | (static_cast<quint32>(bytes[2]) << 16)
               | (static_cast<quint32>(bytes[3]) << 24);
    }

    return (static_cast<quint32>(bytes[0]) << 24)
           | (static_cast<quint32>(bytes[1]) << 16)
           | (static_cast<quint32>(bytes[2]) << 8)
           | static_cast<quint32>(bytes[3]);
}

qint32 readTiffInt32(const QByteArray &data, int offset, TiffByteOrder order)
{
    const quint32 value = readTiffUInt32(data, offset, order);
    if (value <= 0x7FFFFFFFu) {
        return static_cast<qint32>(value);
    }
    return -static_cast<qint32>(0x100000000ULL - value);
}

QByteArray readEntryPayload(const QByteArray &fileData,
                            int entryOffset,
                            uint16_t type,
                            quint32 count,
                            TiffByteOrder order)
{
    const int typeSize = tiffTypeSize(type);
    if (typeSize <= 0) {
        return {};
    }

    const quint32 byteCount = count * static_cast<quint32>(typeSize);
    if (byteCount == 0) {
        return {};
    }

    if (byteCount <= 4) {
        return fileData.mid(entryOffset + 8, static_cast<int>(byteCount));
    }

    const quint32 valueOffset = readTiffUInt32(fileData, entryOffset + 8, order);
    if (valueOffset > static_cast<quint32>(fileData.size())
        || byteCount > static_cast<quint32>(fileData.size())
        || valueOffset + byteCount > static_cast<quint32>(fileData.size())) {
        return {};
    }

    return fileData.mid(static_cast<int>(valueOffset), static_cast<int>(byteCount));
}

QVector<TiffEntry> readIfdEntries(const QByteArray &fileData, quint32 ifdOffset, TiffByteOrder order)
{
    QVector<TiffEntry> entries;
    if (ifdOffset == 0 || ifdOffset + 2 > static_cast<quint32>(fileData.size())) {
        return entries;
    }

    const quint16 entryCount = readTiffUInt16(fileData, static_cast<int>(ifdOffset), order);
    entries.reserve(entryCount);

    int entryOffset = static_cast<int>(ifdOffset) + 2;
    for (quint16 entryIndex = 0; entryIndex < entryCount; ++entryIndex, entryOffset += 12) {
        if (entryOffset + 12 > fileData.size()) {
            break;
        }

        TiffEntry entry;
        entry.tag = readTiffUInt16(fileData, entryOffset, order);
        entry.type = readTiffUInt16(fileData, entryOffset + 2, order);
        entry.count = readTiffUInt32(fileData, entryOffset + 4, order);
        entry.payload = readEntryPayload(fileData, entryOffset, entry.type, entry.count, order);
        entries.append(entry);
    }

    return entries;
}

const TiffEntry *findEntry(const QVector<TiffEntry> &entries, uint32_t tag)
{
    for (const TiffEntry &entry : entries) {
        if (entry.tag == tag) {
            return &entry;
        }
    }
    return nullptr;
}

QVector<quint32> parseUnsignedArray(const TiffEntry &entry, TiffByteOrder order)
{
    QVector<quint32> values;
    values.reserve(static_cast<int>(entry.count));

    switch (entry.type) {
    case 1:
    case 7:
        for (int index = 0; index < entry.payload.size(); ++index) {
            values.append(static_cast<quint8>(entry.payload.at(index)));
        }
        break;
    case 3:
        for (quint32 index = 0; index < entry.count; ++index) {
            values.append(readTiffUInt16(entry.payload, static_cast<int>(index * 2), order));
        }
        break;
    case 4:
    case 13:
        for (quint32 index = 0; index < entry.count; ++index) {
            values.append(readTiffUInt32(entry.payload, static_cast<int>(index * 4), order));
        }
        break;
    default:
        break;
    }

    return values;
}

QVector<float> parseRationalArray(const QByteArray &payload, TiffByteOrder order)
{
    QVector<float> result;
    const int count = payload.size() / 8;
    result.reserve(count);
    for (int index = 0; index < count; ++index) {
        const quint32 numerator = readTiffUInt32(payload, index * 8, order);
        const quint32 denominator = readTiffUInt32(payload, index * 8 + 4, order);
        result.append(denominator == 0 ? 0.0f : static_cast<float>(numerator) / static_cast<float>(denominator));
    }
    return result;
}

QVector<float> parseSignedRationalArray(const QByteArray &payload, TiffByteOrder order)
{
    QVector<float> result;
    const int count = payload.size() / 8;
    result.reserve(count);
    for (int index = 0; index < count; ++index) {
        const qint32 numerator = readTiffInt32(payload, index * 8, order);
        const qint32 denominator = readTiffInt32(payload, index * 8 + 4, order);
        result.append(denominator == 0 ? 0.0f : static_cast<float>(numerator) / static_cast<float>(denominator));
    }
    return result;
}

QVector<float> parseFloatArray(const TiffEntry &entry, TiffByteOrder order)
{
    switch (entry.type) {
    case 5:
        return parseRationalArray(entry.payload, order);
    case 10:
        return parseSignedRationalArray(entry.payload, order);
    case 3: {
        QVector<float> values;
        values.reserve(static_cast<int>(entry.count));
        for (quint32 index = 0; index < entry.count; ++index) {
            values.append(static_cast<float>(readTiffUInt16(entry.payload, static_cast<int>(index * 2), order)));
        }
        return values;
    }
    case 4:
    case 13: {
        QVector<float> values;
        values.reserve(static_cast<int>(entry.count));
        for (quint32 index = 0; index < entry.count; ++index) {
            values.append(static_cast<float>(readTiffUInt32(entry.payload, static_cast<int>(index * 4), order)));
        }
        return values;
    }
    default:
        return {};
    }
}

void applyRootColorMetadata(const QVector<float> &values, bool &flag, float *target, int count)
{
    if (values.size() < count) {
        return;
    }

    flag = true;
    for (int index = 0; index < count; ++index) {
        target[index] = values[index];
    }
}

void applyColorMetadataFromEntries(const QVector<TiffEntry> &entries, TiffByteOrder order, RawFrame &frame)
{
    if (const TiffEntry *colorMatrix = findEntry(entries, kColorMatrix1Tag)) {
        applyRootColorMetadata(parseFloatArray(*colorMatrix, order),
                               frame.hasColorMatrix1,
                               frame.colorMatrix1,
                               9);
    }

    if (const TiffEntry *colorMatrix = findEntry(entries, kColorMatrix2Tag)) {
        applyRootColorMetadata(parseFloatArray(*colorMatrix, order),
                               frame.hasColorMatrix2,
                               frame.colorMatrix2,
                               9);
    }

    if (const TiffEntry *forwardMatrix = findEntry(entries, kForwardMatrix1Tag)) {
        applyRootColorMetadata(parseFloatArray(*forwardMatrix, order),
                               frame.hasForwardMatrix1,
                               frame.forwardMatrix1,
                               9);
    }

    if (const TiffEntry *forwardMatrix = findEntry(entries, kForwardMatrix2Tag)) {
        applyRootColorMetadata(parseFloatArray(*forwardMatrix, order),
                               frame.hasForwardMatrix2,
                               frame.forwardMatrix2,
                               9);
    }

    if (const TiffEntry *asShotNeutral = findEntry(entries, kAsShotNeutralTag)) {
        applyRootColorMetadata(parseFloatArray(*asShotNeutral, order),
                               frame.hasAsShotNeutral,
                               frame.asShotNeutral,
                               3);
    }

    if (const TiffEntry *analogBalance = findEntry(entries, kAnalogBalanceTag)) {
        applyRootColorMetadata(parseFloatArray(*analogBalance, order),
                               frame.hasAnalogBalance,
                               frame.analogBalance,
                               3);
    }
}

bool matchesRawEntries(const QVector<TiffEntry> &entries, TiffByteOrder order)
{
    const TiffEntry *widthEntry = findEntry(entries, TIFFTAG_IMAGEWIDTH);
    const TiffEntry *heightEntry = findEntry(entries, TIFFTAG_IMAGELENGTH);
    const TiffEntry *samplesEntry = findEntry(entries, TIFFTAG_SAMPLESPERPIXEL);
    const TiffEntry *photometricEntry = findEntry(entries, TIFFTAG_PHOTOMETRIC);

    if (!widthEntry || !heightEntry || !photometricEntry) {
        return false;
    }

    const QVector<quint32> widths = parseUnsignedArray(*widthEntry, order);
    const QVector<quint32> heights = parseUnsignedArray(*heightEntry, order);
    const QVector<quint32> samples = samplesEntry ? parseUnsignedArray(*samplesEntry, order) : QVector<quint32> {1};
    const QVector<quint32> photometric = parseUnsignedArray(*photometricEntry, order);

    return !widths.isEmpty()
           && !heights.isEmpty()
           && !samples.isEmpty()
           && samples.first() == 1
           && !photometric.isEmpty()
           && (photometric.first() == kPhotometricCfa || photometric.first() == kPhotometricLinearRaw);
}

struct HistogramAccumulator
{
    int width = 0;
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
    int sampleStep = 1;
    float blackLevel = 0.0f;
    float range = 1.0f;
    QVector<int> counts;
    QVector<quint16> rowBuffer;
    int maxCount = 0;

    HistogramAccumulator(int frameWidth,
                         int frameHeight,
                         bool hasActiveArea,
                         int activeLeft,
                         int activeTop,
                         int activeRight,
                         int activeBottom,
                         float frameBlackLevel,
                         float frameWhiteLevel,
                         int binCount,
                         int targetSamples)
        : width(frameWidth),
          blackLevel(frameBlackLevel),
          range(qMax(1.0f, frameWhiteLevel - frameBlackLevel)),
          counts(qMax(1, binCount), 0),
          rowBuffer(qMax(1, frameWidth))
    {
        left = hasActiveArea ? qBound(0, activeLeft, qMax(0, frameWidth - 1)) : 0;
        top = hasActiveArea ? qBound(0, activeTop, qMax(0, frameHeight - 1)) : 0;
        right = hasActiveArea ? qBound(left + 1, activeRight, frameWidth) : frameWidth;
        bottom = hasActiveArea ? qBound(top + 1, activeBottom, frameHeight) : frameHeight;

        const int sampleWidth = qMax(1, right - left);
        const int sampleHeight = qMax(1, bottom - top);
        const double totalPixels = static_cast<double>(sampleWidth) * static_cast<double>(sampleHeight);
        sampleStep = qMax(1, static_cast<int>(std::sqrt(totalPixels / static_cast<double>(qMax(1, targetSamples)))));
    }

    bool shouldSampleRow(int row) const
    {
        return row >= top && row < bottom && ((row - top) % sampleStep) == 0;
    }

    quint16 *rowStorage()
    {
        return rowBuffer.data();
    }

    void consumeCurrentRow(int startColumn = 0, int columnCount = -1)
    {
        const int endColumn = columnCount < 0 ? right : qMin(right, startColumn + columnCount);
        const int firstSample = left + (qMax(0, startColumn - left) + sampleStep - 1) / sampleStep * sampleStep;
        for (int x = firstSample; x < endColumn; x += sampleStep) {
            const float normalized = qBound(0.0f,
                                            (static_cast<float>(rowBuffer.at(x)) - blackLevel) / range,
                                            1.0f);
            const int binIndex = qMin(counts.size() - 1,
                                      static_cast<int>(normalized * static_cast<float>(counts.size() - 1)));
            const int newCount = ++counts[binIndex];
            if (newCount > maxCount)
                maxCount = newCount;
        }
    }

    QVariantList finish() const
    {
        QVariantList bins;
        bins.reserve(counts.size());

        if (maxCount <= 0) {
            for (int i = 0; i < counts.size(); ++i)
                bins.append(0.0);
            return bins;
        }

        for (int count : counts) {
            const double normalized = static_cast<double>(count) / static_cast<double>(maxCount);
            bins.append(std::pow(normalized, 0.65));
        }

        return bins;
    }
};

QVariantList decodeManualHistogram(const QString &filePath, int binCount, int targetSamples)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    const QByteArray fileData = file.readAll();
    if (fileData.size() < 8)
        return {};

    TiffByteOrder order;
    if (fileData.startsWith("II")) {
        order = TiffByteOrder::LittleEndian;
    } else if (fileData.startsWith("MM")) {
        order = TiffByteOrder::BigEndian;
    } else {
        return {};
    }

    if (readTiffUInt16(fileData, 2, order) != 42)
        return {};

    const quint32 firstIfdOffset = readTiffUInt32(fileData, 4, order);
    const QVector<TiffEntry> rootEntries = readIfdEntries(fileData, firstIfdOffset, order);
    if (rootEntries.isEmpty())
        return {};

    QVector<TiffEntry> rawEntries;
    const bool rootIsRaw = matchesRawEntries(rootEntries, order);
    if (rootIsRaw) {
        rawEntries = rootEntries;
    } else {
        const TiffEntry *subIfdEntry = findEntry(rootEntries, kSubIfdTag);
        if (!subIfdEntry)
            return {};

        const QVector<quint32> subIfdOffsets = parseUnsignedArray(*subIfdEntry, order);
        for (quint32 subIfdOffset : subIfdOffsets) {
            const QVector<TiffEntry> candidateEntries = readIfdEntries(fileData, subIfdOffset, order);
            if (matchesRawEntries(candidateEntries, order)) {
                rawEntries = candidateEntries;
                break;
            }
        }
    }

    if (rawEntries.isEmpty())
        return {};

    const TiffEntry *widthEntry = findEntry(rawEntries, TIFFTAG_IMAGEWIDTH);
    const TiffEntry *heightEntry = findEntry(rawEntries, TIFFTAG_IMAGELENGTH);
    const TiffEntry *bitsEntry = findEntry(rawEntries, TIFFTAG_BITSPERSAMPLE);
    const TiffEntry *tileWidthEntry = findEntry(rawEntries, TIFFTAG_TILEWIDTH);
    const bool tiled = tileWidthEntry != nullptr;
    const TiffEntry *stripOffsetsEntry = findEntry(rawEntries, tiled ? TIFFTAG_TILEOFFSETS : TIFFTAG_STRIPOFFSETS);
    const TiffEntry *stripByteCountsEntry = findEntry(rawEntries, tiled ? TIFFTAG_TILEBYTECOUNTS : TIFFTAG_STRIPBYTECOUNTS);
    if (!widthEntry || !heightEntry || !bitsEntry || !stripOffsetsEntry || !stripByteCountsEntry)
        return {};

    const QVector<quint32> widths = parseUnsignedArray(*widthEntry, order);
    const QVector<quint32> heights = parseUnsignedArray(*heightEntry, order);
    const QVector<quint32> bitDepths = parseUnsignedArray(*bitsEntry, order);
    const QVector<quint32> stripOffsets = parseUnsignedArray(*stripOffsetsEntry, order);
    const QVector<quint32> stripByteCounts = parseUnsignedArray(*stripByteCountsEntry, order);
    if (widths.isEmpty() || heights.isEmpty() || bitDepths.isEmpty() || stripOffsets.isEmpty() || stripByteCounts.isEmpty())
        return {};

    const int width = static_cast<int>(widths.first());
    const int height = static_cast<int>(heights.first());
    const int bitsPerSample = static_cast<int>(bitDepths.first());
    if (width <= 0 || height <= 0 || bitsPerSample <= 0)
        return {};

    float blackLevel = 0.0f;
    if (const TiffEntry *blackLevelEntry = findEntry(rawEntries, kBlackLevelTag)) {
        const QVector<float> values = parseFloatArray(*blackLevelEntry, order);
        if (!values.isEmpty())
            blackLevel = values.first();
    }

    float whiteLevel = static_cast<float>((1u << qMin<int>(bitsPerSample, 15)) - 1u);
    if (const TiffEntry *whiteLevelEntry = findEntry(rawEntries, kWhiteLevelTag)) {
        const QVector<float> values = parseFloatArray(*whiteLevelEntry, order);
        if (!values.isEmpty())
            whiteLevel = values.first();
    }
    if (whiteLevel <= blackLevel)
        whiteLevel = blackLevel + 1.0f;

    bool hasActiveArea = false;
    int activeTop = 0;
    int activeLeft = 0;
    int activeBottom = height;
    int activeRight = width;
    if (const TiffEntry *activeAreaEntry = findEntry(rawEntries, kActiveAreaTag)) {
        const QVector<quint32> activeArea = parseUnsignedArray(*activeAreaEntry, order);
        if (activeArea.size() >= 4) {
            hasActiveArea = true;
            activeTop = static_cast<int>(activeArea[0]);
            activeLeft = static_cast<int>(activeArea[1]);
            activeBottom = static_cast<int>(activeArea[2] + 1);
            activeRight = static_cast<int>(activeArea[3] + 1);
        }
    }

    quint32 rowsPerStrip = static_cast<quint32>(height);
    if (const TiffEntry *rowsPerStripEntry = findEntry(rawEntries, tiled ? TIFFTAG_TILELENGTH : TIFFTAG_ROWSPERSTRIP)) {
        const QVector<quint32> values = parseUnsignedArray(*rowsPerStripEntry, order);
        if (!values.isEmpty() && values.first() > 0)
            rowsPerStrip = values.first();
    }

    quint32 compression = COMPRESSION_NONE;
    if (const TiffEntry *compressionEntry = findEntry(rawEntries, TIFFTAG_COMPRESSION)) {
        const QVector<quint32> values = parseUnsignedArray(*compressionEntry, order);
        if (!values.isEmpty())
            compression = values.first();
    }
    if (compression != COMPRESSION_NONE && compression != COMPRESSION_JPEG)
        return {};
    int encodedWidth = width;
    if (tiled) {
        const auto values = parseUnsignedArray(*tileWidthEntry, order);
        if (compression != COMPRESSION_JPEG || values.isEmpty() || values.first() == 0
            || values.first() > 131070 || rowsPerStrip > 65535)
            return {};
        encodedWidth = static_cast<int>(values.first());
    }
    const quint64 tileColumns = (static_cast<quint64>(width) + encodedWidth - 1) / encodedWidth;
    if (stripOffsets.size() != stripByteCounts.size() ||
        static_cast<quint64>(stripOffsets.size()) != tileColumns * ((static_cast<quint64>(height) + rowsPerStrip - 1) / rowsPerStrip))
        return {};

    HistogramAccumulator histogram(width,
                                   height,
                                   hasActiveArea,
                                   activeLeft,
                                   activeTop,
                                   activeRight,
                                   activeBottom,
                                   blackLevel,
                                   whiteLevel,
                                   binCount,
                                   targetSamples);

    const int rowBytes = packedRowBytes(width, bitsPerSample);
    QVector<quint32> linearization;
    if (const TiffEntry *entry = findEntry(rawEntries, kLinearizationTableTag)) {
        linearization = parseUnsignedArray(*entry, order);
        if (linearization.isEmpty())
            return {};
    }
    QVector<quint16> jpegPixels;
    for (int stripIndex = 0; stripIndex < stripOffsets.size(); ++stripIndex) {
        const quint32 stripOffset = stripOffsets.at(stripIndex);
        const quint32 stripByteCount = stripByteCounts.at(qMin(stripIndex, stripByteCounts.size() - 1));
        const int startRow = (stripIndex / tileColumns) * rowsPerStrip;
        const int startColumn = (stripIndex % tileColumns) * encodedWidth;
        const int columnsInTile = qMin(encodedWidth, width - startColumn);
        if (startRow >= height)
            break;

        const int rowsInStrip = qMin<int>(static_cast<int>(rowsPerStrip), height - startRow);
        const int requiredBytes = rowsInStrip * rowBytes;
        if (stripOffset > static_cast<quint32>(fileData.size())
            || stripByteCount > static_cast<quint32>(fileData.size())
            || static_cast<quint64>(stripOffset) + stripByteCount > static_cast<quint64>(fileData.size())
            || (compression == COMPRESSION_NONE && stripByteCount < static_cast<quint32>(requiredBytes))) {
            return {};
        }

        const uchar *stripData = reinterpret_cast<const uchar *>(fileData.constData() + static_cast<int>(stripOffset));
        if (compression == COMPRESSION_JPEG) {
            const int encodedRows = tiled ? static_cast<int>(rowsPerStrip) : rowsInStrip;
            jpegPixels.resize(static_cast<qsizetype>(encodedWidth) * encodedRows);
            if (!decodeJpegStrip(stripData, stripByteCount, encodedWidth, encodedRows, bitsPerSample, jpegPixels.data()))
                return {};
        }
        for (int rowInStrip = 0; rowInStrip < rowsInStrip; ++rowInStrip) {
            const int row = startRow + rowInStrip;
            if (!histogram.shouldSampleRow(row))
                continue;

            if (compression == COMPRESSION_JPEG) {
                std::copy_n(jpegPixels.constData() + static_cast<qsizetype>(rowInStrip) * encodedWidth,
                            columnsInTile, histogram.rowStorage() + startColumn);
            } else {
                const uchar *rowData = stripData + (rowInStrip * rowBytes);
                unpackSamples(rowData, rowBytes, width, bitsPerSample, histogram.rowStorage());
            }
            if (!linearization.isEmpty()) {
                quint16 *samples = histogram.rowStorage() + startColumn;
                for (int x = 0; x < columnsInTile; ++x) {
                    if (samples[x] >= linearization.size() || linearization[samples[x]] > 65535)
                        return {};
                    samples[x] = static_cast<quint16>(linearization[samples[x]]);
                }
            }
            histogram.consumeCurrentRow(startColumn, columnsInTile);
        }
    }

    return histogram.finish();
}

QVariantList decodeTiffHistogram(const QString &filePath, int binCount, int targetSamples)
{
    TIFF *tiff = openTiffWithCustomTags(filePath, "r");
    if (!tiff)
        return {};

    if (!selectRawDirectory(tiff)) {
        TIFFClose(tiff);
        return {};
    }

    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t bitsPerSample = 0;
    uint16_t samplesPerPixel = 1;
    TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);

    if (width == 0 || height == 0 || samplesPerPixel != 1) {
        TIFFClose(tiff);
        return {};
    }

    const QVector<int> activeArea = readLongArrayTag(tiff, kActiveAreaTag);
    const bool hasActiveArea = activeArea.size() == 4;
    const float blackLevel = firstFloatTagValue(tiff, kBlackLevelTag, 0.0f);
    float whiteLevel = firstFloatTagValue(tiff, kWhiteLevelTag, static_cast<float>((1u << qMin<int>(bitsPerSample, 15)) - 1u));
    if (whiteLevel <= blackLevel)
        whiteLevel = blackLevel + 1.0f;

    HistogramAccumulator histogram(static_cast<int>(width),
                                   static_cast<int>(height),
                                   hasActiveArea,
                                   hasActiveArea ? activeArea[1] : 0,
                                   hasActiveArea ? activeArea[0] : 0,
                                   hasActiveArea ? activeArea[3] + 1 : static_cast<int>(width),
                                   hasActiveArea ? activeArea[2] + 1 : static_cast<int>(height),
                                   blackLevel,
                                   whiteLevel,
                                   binCount,
                                   targetSamples);

    bool ok = true;
    if (TIFFIsTiled(tiff)) {
        ok = false;
    } else if (TIFFNumberOfStrips(tiff) > 1) {
        uint32_t rowsPerStrip = 0;
        TIFFGetFieldDefaulted(tiff, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip);
        if (rowsPerStrip == 0) {
            ok = false;
        } else {
            const int rowBytes = packedRowBytes(static_cast<int>(width), static_cast<int>(bitsPerSample));
            const tstrip_t stripCount = TIFFNumberOfStrips(tiff);
            QByteArray stripData;

            for (tstrip_t strip = 0; ok && strip < stripCount; ++strip) {
                const uint32_t startRow = strip * rowsPerStrip;
                if (startRow >= height)
                    break;

                const uint32_t rowsInStrip = qMin<uint32_t>(rowsPerStrip, height - startRow);
                const tsize_t expectedBytes = static_cast<tsize_t>(rowsInStrip) * rowBytes;
                stripData.resize(expectedBytes);
                const tsize_t bytesRead = TIFFReadEncodedStrip(tiff, strip, stripData.data(), expectedBytes);
                if (bytesRead < expectedBytes) {
                    ok = false;
                    break;
                }

                const uchar *stripPtr = reinterpret_cast<const uchar *>(stripData.constData());
                for (uint32_t rowInStrip = 0; rowInStrip < rowsInStrip; ++rowInStrip) {
                    const int row = static_cast<int>(startRow + rowInStrip);
                    if (!histogram.shouldSampleRow(row))
                        continue;

                    const uchar *rowData = stripPtr + (rowInStrip * rowBytes);
                    unpackSamples(rowData,
                                  rowBytes,
                                  static_cast<int>(width),
                                  static_cast<int>(bitsPerSample),
                                  histogram.rowStorage());
                    histogram.consumeCurrentRow();
                }
            }
        }
    } else {
        const tsize_t scanlineSize = TIFFScanlineSize(tiff);
        if (scanlineSize <= 0) {
            ok = false;
        } else {
            QByteArray rowBytes(scanlineSize, Qt::Uninitialized);
            for (uint32_t row = 0; ok && row < height; ++row) {
                if (!histogram.shouldSampleRow(static_cast<int>(row)))
                    continue;

                if (!readScanline(tiff, row, rowBytes)) {
                    ok = false;
                    break;
                }

                unpackSamples(reinterpret_cast<const uchar *>(rowBytes.constData()),
                              rowBytes.size(),
                              static_cast<int>(width),
                              static_cast<int>(bitsPerSample),
                              histogram.rowStorage());
                histogram.consumeCurrentRow();
            }
        }
    }

    TIFFClose(tiff);
    return ok ? histogram.finish() : QVariantList{};
}

bool decodeManualRaw(const QString &filePath, RawFrame &frame)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray fileData = file.readAll();
    if (fileData.size() < 8) {
        return false;
    }

    TiffByteOrder order;
    if (fileData.startsWith("II")) {
        order = TiffByteOrder::LittleEndian;
    } else if (fileData.startsWith("MM")) {
        order = TiffByteOrder::BigEndian;
    } else {
        return false;
    }

    if (readTiffUInt16(fileData, 2, order) != 42) {
        return false;
    }

    const quint32 firstIfdOffset = readTiffUInt32(fileData, 4, order);
    const QVector<TiffEntry> rootEntries = readIfdEntries(fileData, firstIfdOffset, order);
    if (rootEntries.isEmpty()) {
        return false;
    }

    QVector<TiffEntry> rawEntries;
    const bool rootIsRaw = matchesRawEntries(rootEntries, order);
    if (rootIsRaw) {
        rawEntries = rootEntries;
    } else {
        const TiffEntry *subIfdEntry = findEntry(rootEntries, kSubIfdTag);
        if (!subIfdEntry) {
            return false;
        }

        const QVector<quint32> subIfdOffsets = parseUnsignedArray(*subIfdEntry, order);
        if (subIfdOffsets.isEmpty()) {
            return false;
        }

        for (quint32 subIfdOffset : subIfdOffsets) {
            const QVector<TiffEntry> candidateEntries = readIfdEntries(fileData, subIfdOffset, order);
            if (matchesRawEntries(candidateEntries, order)) {
                rawEntries = candidateEntries;
                break;
            }
        }
    }

    if (rawEntries.isEmpty()) {
        return false;
    }

    applyColorMetadataFromEntries(rootEntries, order, frame);
    if (!rootIsRaw) {
        applyColorMetadataFromEntries(rawEntries, order, frame);
    }

    const TiffEntry *widthEntry = findEntry(rawEntries, TIFFTAG_IMAGEWIDTH);
    const TiffEntry *heightEntry = findEntry(rawEntries, TIFFTAG_IMAGELENGTH);
    const TiffEntry *bitsEntry = findEntry(rawEntries, TIFFTAG_BITSPERSAMPLE);
    const TiffEntry *tileWidthEntry = findEntry(rawEntries, TIFFTAG_TILEWIDTH);
    const bool tiled = tileWidthEntry != nullptr;
    const TiffEntry *stripOffsetsEntry = findEntry(rawEntries, tiled ? TIFFTAG_TILEOFFSETS : TIFFTAG_STRIPOFFSETS);
    const TiffEntry *stripByteCountsEntry = findEntry(rawEntries, tiled ? TIFFTAG_TILEBYTECOUNTS : TIFFTAG_STRIPBYTECOUNTS);

    if (!widthEntry || !heightEntry || !bitsEntry || !stripOffsetsEntry || !stripByteCountsEntry) {
        return false;
    }

    const QVector<quint32> widths = parseUnsignedArray(*widthEntry, order);
    const QVector<quint32> heights = parseUnsignedArray(*heightEntry, order);
    const QVector<quint32> bitDepths = parseUnsignedArray(*bitsEntry, order);
    const QVector<quint32> stripOffsets = parseUnsignedArray(*stripOffsetsEntry, order);
    const QVector<quint32> stripByteCounts = parseUnsignedArray(*stripByteCountsEntry, order);

    if (widths.isEmpty() || heights.isEmpty() || bitDepths.isEmpty() || stripOffsets.isEmpty() || stripByteCounts.isEmpty()) {
        return false;
    }

    frame.width = static_cast<int>(widths.first());
    frame.height = static_cast<int>(heights.first());
    frame.bitsPerSample = static_cast<int>(bitDepths.first());
    if (frame.width <= 0 || frame.height <= 0 || frame.bitsPerSample <= 0) {
        return false;
    }

    if (const TiffEntry *photometricEntry = findEntry(rawEntries, TIFFTAG_PHOTOMETRIC)) {
        const auto values = parseUnsignedArray(*photometricEntry, order);
        frame.monochrome = !values.isEmpty() && values.first() == kPhotometricLinearRaw;
    }
    if (const TiffEntry *cfaPatternEntry = findEntry(rawEntries, kCfaPatternTag)) {
        frame.cfaPattern = mapCfaPattern(cfaPatternEntry->payload);
    }

    if (const TiffEntry *blackLevelEntry = findEntry(rawEntries, kBlackLevelTag)) {
        const QVector<float> values = parseFloatArray(*blackLevelEntry, order);
        if (!values.isEmpty()) {
            frame.blackLevel = values.first();
        }
    }

    if (const TiffEntry *whiteLevelEntry = findEntry(rawEntries, kWhiteLevelTag)) {
        const QVector<float> values = parseFloatArray(*whiteLevelEntry, order);
        if (!values.isEmpty()) {
            frame.whiteLevel = values.first();
        }
    }
    if (frame.whiteLevel <= frame.blackLevel) {
        frame.whiteLevel = static_cast<float>((1u << qMin<int>(frame.bitsPerSample, 15)) - 1u);
    }

    if (const TiffEntry *activeAreaEntry = findEntry(rawEntries, kActiveAreaTag)) {
        const QVector<quint32> activeArea = parseUnsignedArray(*activeAreaEntry, order);
        if (activeArea.size() >= 4) {
            frame.hasActiveArea = true;
            frame.activeTop = static_cast<int>(activeArea[0]);
            frame.activeLeft = static_cast<int>(activeArea[1]);
            frame.activeBottom = static_cast<int>(activeArea[2] + 1);
            frame.activeRight = static_cast<int>(activeArea[3] + 1);
        }
    }

    quint32 rowsPerStrip = static_cast<quint32>(frame.height);
    if (const TiffEntry *rowsPerStripEntry = findEntry(rawEntries, tiled ? TIFFTAG_TILELENGTH : TIFFTAG_ROWSPERSTRIP)) {
        const QVector<quint32> values = parseUnsignedArray(*rowsPerStripEntry, order);
        if (!values.isEmpty() && values.first() > 0) {
            rowsPerStrip = values.first();
        }
    }

    quint32 compression = COMPRESSION_NONE;
    if (const TiffEntry *compressionEntry = findEntry(rawEntries, TIFFTAG_COMPRESSION)) {
        const QVector<quint32> values = parseUnsignedArray(*compressionEntry, order);
        if (!values.isEmpty())
            compression = values.first();
    }
    if (compression != COMPRESSION_NONE && compression != COMPRESSION_JPEG)
        return false;
    int encodedWidth = frame.width;
    if (tiled) {
        const auto values = parseUnsignedArray(*tileWidthEntry, order);
        if (compression != COMPRESSION_JPEG || values.isEmpty() || values.first() == 0
            || values.first() > 131070 || rowsPerStrip > 65535)
            return false;
        encodedWidth = static_cast<int>(values.first());
    }
    const quint64 tileColumns = (static_cast<quint64>(frame.width) + encodedWidth - 1) / encodedWidth;
    if (stripOffsets.size() != stripByteCounts.size() ||
        static_cast<quint64>(stripOffsets.size()) != tileColumns * ((static_cast<quint64>(frame.height) + rowsPerStrip - 1) / rowsPerStrip))
        return false;
    QVector<quint16> jpegPixels;

    frame.pixels.resize(frame.width * frame.height);
    const int rowBytes = packedRowBytes(frame.width, frame.bitsPerSample);

    for (int stripIndex = 0; stripIndex < stripOffsets.size(); ++stripIndex) {
        const quint32 stripOffset = stripOffsets.at(stripIndex);
        const quint32 stripByteCount = stripByteCounts.at(qMin(stripIndex, stripByteCounts.size() - 1));
        const int startRow = (stripIndex / tileColumns) * rowsPerStrip;
        const int startColumn = (stripIndex % tileColumns) * encodedWidth;
        const int columnsInTile = qMin(encodedWidth, frame.width - startColumn);
        if (startRow >= frame.height) {
            break;
        }

        const int rowsInStrip = qMin<int>(static_cast<int>(rowsPerStrip), frame.height - startRow);
        const int requiredBytes = rowsInStrip * rowBytes;
        if (stripOffset > static_cast<quint32>(fileData.size())
            || stripByteCount > static_cast<quint32>(fileData.size())
            || static_cast<quint64>(stripOffset) + stripByteCount > static_cast<quint64>(fileData.size())
            || (compression == COMPRESSION_NONE && stripByteCount < static_cast<quint32>(requiredBytes))) {
            return false;
        }

        const uchar *stripData = reinterpret_cast<const uchar *>(fileData.constData() + static_cast<int>(stripOffset));
        if (compression == COMPRESSION_JPEG) {
            const int encodedRows = tiled ? static_cast<int>(rowsPerStrip) : rowsInStrip;
            jpegPixels.resize(static_cast<qsizetype>(encodedWidth) * encodedRows);
            if (!decodeJpegStrip(stripData, stripByteCount, encodedWidth, encodedRows, frame.bitsPerSample,
                                 jpegPixels.data()))
                return false;
            for (int row = 0; row < rowsInStrip; ++row)
                std::copy_n(jpegPixels.constData() + static_cast<qsizetype>(row) * encodedWidth, columnsInTile,
                            frame.pixels.data() + static_cast<qsizetype>(startRow + row) * frame.width + startColumn);
            continue;
        }
        for (int rowInStrip = 0; rowInStrip < rowsInStrip; ++rowInStrip) {
            const uchar *rowData = stripData + (rowInStrip * rowBytes);
            unpackSamples(rowData,
                          rowBytes,
                          frame.width,
                          frame.bitsPerSample,
                          frame.pixels.data() + ((startRow + rowInStrip) * frame.width));
        }
    }

    if (const TiffEntry *entry = findEntry(rawEntries, kLinearizationTableTag)) {
        const auto table = parseUnsignedArray(*entry, order);
        if (table.isEmpty())
            return false;
        for (quint16 &sample : frame.pixels) {
            if (sample >= table.size() || table[sample] > 65535)
                return false;
            sample = static_cast<quint16>(table[sample]);
        }
        frame.bitsPerSample = 16;
    }
    frame.valid = true;
    return true;
}

double readFrameRateFromEntries(const QVector<TiffEntry> &entries, TiffByteOrder order)
{
    const TiffEntry *frameRateEntry = findEntry(entries, kFrameRateTag);
    if (!frameRateEntry) {
        return 0.0;
    }

    const QVector<float> values = parseFloatArray(*frameRateEntry, order);
    if (values.isEmpty()) {
        return 0.0;
    }

    const double fps = static_cast<double>(values.first());
    if (fps < 1.0 || fps > 120.0) {
        return 0.0;
    }
    return fps;
}

int mapCfaPattern(const QByteArray &pattern)
{
    if (pattern.size() < 4) {
        return RawFrame::RGGB;
    }

    const uchar r = 0;
    const uchar g = 1;
    const uchar b = 2;

    if (pattern[0] == r && pattern[1] == g && pattern[2] == g && pattern[3] == b) {
        return RawFrame::RGGB;
    }
    if (pattern[0] == b && pattern[1] == g && pattern[2] == g && pattern[3] == r) {
        return RawFrame::BGGR;
    }
    if (pattern[0] == g && pattern[1] == r && pattern[2] == b && pattern[3] == g) {
        return RawFrame::GRBG;
    }
    if (pattern[0] == g && pattern[1] == b && pattern[2] == r && pattern[3] == g) {
        return RawFrame::GBRG;
    }

    return RawFrame::RGGB;
}

quint16 unpackBitsMsbFirst(const unsigned char *data, int bitOffset, int bitsPerSample)
{
    quint32 value = 0;
    for (int bit = 0; bit < bitsPerSample; ++bit) {
        const int index = bitOffset + bit;
        const unsigned char currentByte = data[index / 8];
        const int shift = 7 - (index % 8);
        value = (value << 1) | ((currentByte >> shift) & 0x1);
    }
    return static_cast<quint16>(value);
}

void unpackSamples(const uchar *data, qsizetype dataSize, int width, int bitsPerSample, quint16 *target)
{
    if (bitsPerSample == 8) {
        for (int x = 0; x < width; ++x) {
            target[x] = static_cast<quint8>(data[x]);
        }
        return;
    }

    if (bitsPerSample == 16) {
        const quint16 *src = reinterpret_cast<const quint16 *>(data);
        for (int x = 0; x < width; ++x) {
            target[x] = qFromLittleEndian(src[x]);
        }
        return;
    }

    if (bitsPerSample == 10) {
#if defined(__aarch64__) && defined(__ARM_NEON)
        int dst = unpack10BitNeon(data, dataSize, width, target);
        int src = (dst / 4) * 5;
#else
        int src = 0;
        int dst = 0;
#endif
        while (dst + 3 < width && src + 4 < dataSize) {
            const uchar b0 = data[src++];
            const uchar b1 = data[src++];
            const uchar b2 = data[src++];
            const uchar b3 = data[src++];
            const uchar b4 = data[src++];

            target[dst++] = static_cast<quint16>((static_cast<quint16>(b0) << 2) | (b1 >> 6));
            target[dst++] = static_cast<quint16>((static_cast<quint16>(b1 & 0x3F) << 4) | (b2 >> 4));
            target[dst++] = static_cast<quint16>((static_cast<quint16>(b2 & 0x0F) << 6) | (b3 >> 2));
            target[dst++] = static_cast<quint16>((static_cast<quint16>(b3 & 0x03) << 8) | b4);
        }
        while (dst < width && src < dataSize) {
            target[dst] = unpackBitsMsbFirst(data, dst * bitsPerSample, bitsPerSample);
            ++dst;
        }
        return;
    }

    if (bitsPerSample == 12) {
#if defined(__aarch64__) && defined(__ARM_NEON)
        int dst = unpack12BitNeon(data, dataSize, width, target);
        int src = (dst / 2) * 3;
#else
        int src = 0;
        int dst = 0;
#endif
        while (dst + 1 < width && src + 2 < dataSize) {
            const uchar b0 = data[src++];
            const uchar b1 = data[src++];
            const uchar b2 = data[src++];

            target[dst++] = static_cast<quint16>((static_cast<quint16>(b0) << 4) | (b1 >> 4));
            target[dst++] = static_cast<quint16>((static_cast<quint16>(b1 & 0x0F) << 8) | b2);
        }
        while (dst < width && src < dataSize) {
            target[dst++] = data[src++];
        }
        return;
    }

    int bitOffset = 0;
    for (int x = 0; x < width; ++x) {
        target[x] = unpackBitsMsbFirst(data, bitOffset, bitsPerSample);
        bitOffset += bitsPerSample;
    }
}

bool matchesRawDirectory(TIFF *tiff)
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t samplesPerPixel = 1;
    uint16_t photometric = 0;

    TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_PHOTOMETRIC, &photometric);

    return width > 0 && height > 0 && samplesPerPixel == 1 &&
        (photometric == kPhotometricCfa || photometric == kPhotometricLinearRaw);
}

bool selectRawDirectory(TIFF *tiff)
{
    for (tdir_t dirIndex = 0; TIFFSetDirectory(tiff, dirIndex); ++dirIndex) {
        if (matchesRawDirectory(tiff)) {
            return true;
        }

        uint16_t subIfdCount = 0;
        uint64_t *subIfdOffsets = nullptr;
        if (TIFFGetField(tiff, kSubIfdTag, &subIfdCount, &subIfdOffsets) && subIfdCount > 0 && subIfdOffsets) {
            for (uint16_t subIndex = 0; subIndex < subIfdCount; ++subIndex) {
                if (!TIFFSetSubDirectory(tiff, static_cast<toff_t>(subIfdOffsets[subIndex]))) {
                    continue;
                }
                if (matchesRawDirectory(tiff)) {
                    return true;
                }
            }
        }
    }
    return false;
}

float firstFloatTagValue(TIFF *tiff, uint32_t tag, float fallback)
{
    float *values = nullptr;
    uint16_t count = 0;
    if (TIFFGetField(tiff, tag, &count, &values) && count > 0 && values) {
        return values[0];
    }
    return fallback;
}

bool readScanline(TIFF *tiff, uint32_t row, QByteArray &buffer)
{
    if (TIFFReadScanline(tiff, buffer.data(), row, 0) < 0) {
        return false;
    }
    return true;
}

int packedRowBytes(int width, int bitsPerSample)
{
    return (width * bitsPerSample + 7) / 8;
}

bool decodeByScanlines(TIFF *tiff, RawFrame &frame)
{
    const tsize_t scanlineSize = TIFFScanlineSize(tiff);
    if (scanlineSize <= 0) {
        frame.error = QStringLiteral("Could not read DNG scanline size.");
        return false;
    }

    QByteArray rowBytes(scanlineSize, Qt::Uninitialized);
    for (uint32_t row = 0; row < static_cast<uint32_t>(frame.height); ++row) {
        if (!readScanline(tiff, row, rowBytes)) {
            frame.error = QStringLiteral("Failed to decode DNG scanline %1.").arg(row);
            return false;
        }

        unpackSamples(reinterpret_cast<const uchar *>(rowBytes.constData()),
                      rowBytes.size(),
                      frame.width,
                      frame.bitsPerSample,
                      frame.pixels.data() + (row * frame.width));
    }

    return true;
}

bool decodeByStrips(TIFF *tiff, RawFrame &frame)
{
    uint32_t rowsPerStrip = 0;
    TIFFGetFieldDefaulted(tiff, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip);
    if (rowsPerStrip == 0) {
        frame.error = QStringLiteral("Invalid rows-per-strip value.");
        return false;
    }

    const int rowBytes = packedRowBytes(frame.width, frame.bitsPerSample);
    const tstrip_t stripCount = TIFFNumberOfStrips(tiff);

    for (tstrip_t strip = 0; strip < stripCount; ++strip) {
        const uint32_t startRow = strip * rowsPerStrip;
        if (startRow >= static_cast<uint32_t>(frame.height)) {
            break;
        }

        const uint32_t rowsInStrip = qMin<uint32_t>(rowsPerStrip, frame.height - startRow);
        const tsize_t expectedBytes = static_cast<tsize_t>(rowsInStrip) * rowBytes;
        QByteArray stripData(expectedBytes, Qt::Uninitialized);
        const tsize_t bytesRead = TIFFReadEncodedStrip(tiff, strip, stripData.data(), expectedBytes);
        if (bytesRead < expectedBytes) {
            frame.error = QStringLiteral("Failed to decode strip %1.").arg(strip);
            return false;
        }

        const uchar *stripPtr = reinterpret_cast<const uchar *>(stripData.constData());
        for (uint32_t rowInStrip = 0; rowInStrip < rowsInStrip; ++rowInStrip) {
            const uchar *rowData = stripPtr + (rowInStrip * rowBytes);
            unpackSamples(rowData,
                          rowBytes,
                          frame.width,
                          frame.bitsPerSample,
                          frame.pixels.data() + ((startRow + rowInStrip) * frame.width));
        }
    }

    return true;
}

bool decodeByTiles(TIFF *tiff, RawFrame &frame)
{
    uint32_t tileWidth = 0;
    uint32_t tileLength = 0;
    TIFFGetField(tiff, TIFFTAG_TILEWIDTH, &tileWidth);
    TIFFGetField(tiff, TIFFTAG_TILELENGTH, &tileLength);

    if (tileWidth == 0 || tileLength == 0) {
        frame.error = QStringLiteral("Invalid tile geometry.");
        return false;
    }

    const int tileRowBytes = packedRowBytes(static_cast<int>(tileWidth), frame.bitsPerSample);
    const tsize_t tileBufferSize = TIFFTileSize(tiff);
    if (tileBufferSize <= 0) {
        frame.error = QStringLiteral("Invalid tile size.");
        return false;
    }

    QByteArray tileData(tileBufferSize, Qt::Uninitialized);
    for (uint32_t tileY = 0; tileY < static_cast<uint32_t>(frame.height); tileY += tileLength) {
        for (uint32_t tileX = 0; tileX < static_cast<uint32_t>(frame.width); tileX += tileWidth) {
            const ttile_t tileIndex = TIFFComputeTile(tiff, tileX, tileY, 0, 0);
            const tsize_t bytesRead = TIFFReadEncodedTile(tiff, tileIndex, tileData.data(), tileBufferSize);
            if (bytesRead <= 0) {
                frame.error = QStringLiteral("Failed to decode tile at %1,%2.").arg(tileX).arg(tileY);
                return false;
            }

            const uint32_t copyRows = qMin<uint32_t>(tileLength, frame.height - tileY);
            const uint32_t copyWidth = qMin<uint32_t>(tileWidth, frame.width - tileX);
            const int copyRowBytes = packedRowBytes(static_cast<int>(copyWidth), frame.bitsPerSample);
            const uchar *tilePtr = reinterpret_cast<const uchar *>(tileData.constData());

            for (uint32_t row = 0; row < copyRows; ++row) {
                const uchar *rowData = tilePtr + (row * tileRowBytes);
                unpackSamples(rowData,
                              copyRowBytes,
                              static_cast<int>(copyWidth),
                              frame.bitsPerSample,
                              frame.pixels.data() + ((tileY + row) * frame.width + tileX));
            }
        }
    }

    return true;
}

QVector<int> readLongArrayTag(TIFF *tiff, uint32_t tag)
{
    uint32_t *values = nullptr;
    uint16_t count16 = 0;
    uint32_t count32 = 0;

    if (TIFFGetField(tiff, tag, &count16, &values) && values && count16 > 0) {
        QVector<int> result;
        result.reserve(count16);
        for (uint16_t i = 0; i < count16; ++i) {
            result.append(static_cast<int>(values[i]));
        }
        return result;
    }

    if (TIFFGetField(tiff, tag, &count32, &values) && values && count32 > 0) {
        QVector<int> result;
        result.reserve(static_cast<int>(count32));
        for (uint32_t i = 0; i < count32; ++i) {
            result.append(static_cast<int>(values[i]));
        }
        return result;
    }

    return {};
}

void resetFrameForDecode(RawFrame &frame)
{
    QVector<quint16> pixels = std::move(frame.pixels);
    frame = RawFrame{};
    frame.pixels = std::move(pixels);
    frame.pixels.clear();
}
}

bool DngDecoder::decodeFileInto(const QString &filePath, RawFrame &frame)
{
    resetFrameForDecode(frame);
    if (decodeManualRaw(filePath, frame)) {
        return true;
    }

    TIFF *tiff = openTiffWithCustomTags(filePath, "r");
    if (!tiff) {
        frame.error = QStringLiteral("Could not open DNG file.");
        return false;
    }

    if (!selectRawDirectory(tiff)) {
        frame.error = QStringLiteral("No supported RAW image directory found.");
        TIFFClose(tiff);
        return false;
    }

    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t bitsPerSample = 0;
    uint16_t samplesPerPixel = 1;

    TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
    TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);

    if (width == 0 || height == 0 || samplesPerPixel != 1) {
        frame.error = QStringLiteral("Unsupported DNG layout.");
        TIFFClose(tiff);
        return false;
    }

    uint16_t repeatDims[2] = {2, 2};
    void *patternData = nullptr;
    uint32_t patternCount = 0;
    TIFFGetFieldDefaulted(tiff, kCfaRepeatPatternDimTag, &repeatDims[0], &repeatDims[1]);
    TIFFGetField(tiff, kCfaPatternTag, &patternCount, &patternData);

    QByteArray cfaPattern;
    if (patternData && repeatDims[0] == 2 && repeatDims[1] == 2 && patternCount >= 4) {
        cfaPattern = QByteArray(static_cast<const char *>(patternData), 4);
    }

    frame.width = static_cast<int>(width);
    frame.height = static_cast<int>(height);
    frame.bitsPerSample = static_cast<int>(bitsPerSample);
    uint16_t photometric = 0;
    TIFFGetFieldDefaulted(tiff, TIFFTAG_PHOTOMETRIC, &photometric);
    frame.monochrome = photometric == kPhotometricLinearRaw;
    frame.cfaPattern = mapCfaPattern(cfaPattern);
    frame.blackLevel = firstFloatTagValue(tiff, kBlackLevelTag, 0.0f);
    frame.whiteLevel = firstFloatTagValue(tiff, kWhiteLevelTag, static_cast<float>((1u << qMin<int>(bitsPerSample, 15)) - 1u));
    const QVector<int> activeArea = readLongArrayTag(tiff, kActiveAreaTag);
    if (activeArea.size() == 4) {
        frame.hasActiveArea = true;
        frame.activeTop = activeArea[0];
        frame.activeLeft = activeArea[1];
        frame.activeBottom = activeArea[2] + 1;
        frame.activeRight = activeArea[3] + 1;
    }
    frame.pixels.resize(frame.width * frame.height);

    bool ok = false;
    if (TIFFIsTiled(tiff)) {
        ok = decodeByTiles(tiff, frame);
    } else if (TIFFNumberOfStrips(tiff) > 1) {
        ok = decodeByStrips(tiff, frame);
    } else {
        ok = decodeByScanlines(tiff, frame);
    }

    if (!ok) {
        frame.pixels.clear();
        TIFFClose(tiff);
        return false;
    }

    frame.valid = true;
    TIFFClose(tiff);
    return true;
}

RawFrame DngDecoder::decodeFile(const QString &filePath)
{
    RawFrame frame;
    decodeFileInto(filePath, frame);
    return frame;
}

QVariantList DngDecoder::decodeHistogramBins(const QString &filePath, int binCount, int targetSamples)
{
    QVariantList bins = decodeManualHistogram(filePath, binCount, targetSamples);
    if (!bins.isEmpty())
        return bins;

    return decodeTiffHistogram(filePath, binCount, targetSamples);
}

double DngDecoder::detectFrameRate(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return 0.0;
    }

    const QByteArray fileData = file.readAll();
    if (fileData.size() < 8) {
        return 0.0;
    }

    TiffByteOrder order;
    if (fileData.startsWith("II")) {
        order = TiffByteOrder::LittleEndian;
    } else if (fileData.startsWith("MM")) {
        order = TiffByteOrder::BigEndian;
    } else {
        return 0.0;
    }

    if (readTiffUInt16(fileData, 2, order) != 42) {
        return 0.0;
    }

    const quint32 firstIfdOffset = readTiffUInt32(fileData, 4, order);
    const QVector<TiffEntry> rootEntries = readIfdEntries(fileData, firstIfdOffset, order);
    if (rootEntries.isEmpty()) {
        return 0.0;
    }

    double fps = readFrameRateFromEntries(rootEntries, order);
    if (fps > 0.0) {
        return fps;
    }

    const TiffEntry *subIfdEntry = findEntry(rootEntries, kSubIfdTag);
    if (!subIfdEntry) {
        return 0.0;
    }

    const QVector<quint32> subIfdOffsets = parseUnsignedArray(*subIfdEntry, order);
    for (quint32 subIfdOffset : subIfdOffsets) {
        const QVector<TiffEntry> entries = readIfdEntries(fileData, subIfdOffset, order);
        fps = readFrameRateFromEntries(entries, order);
        if (fps > 0.0) {
            return fps;
        }
    }

    return 0.0;
}
