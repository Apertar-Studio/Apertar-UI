#include "dngdecoder.h"

#include <QByteArray>
#include <QFile>
#include <QtGlobal>
#include <QtEndian>

#include <tiffio.h>

#include "tiffhelper.h"

namespace {
constexpr uint16_t kPhotometricCfa = 32803;
constexpr uint32_t kCfaRepeatPatternDimTag = 33421;
constexpr uint32_t kCfaPatternTag = 33422;
constexpr uint32_t kBlackLevelTag = 50714;
constexpr uint32_t kWhiteLevelTag = 50717;
constexpr uint32_t kColorMatrix1Tag = 50721;
constexpr uint32_t kAnalogBalanceTag = 50727;
constexpr uint32_t kSubIfdTag = 330;
constexpr uint32_t kActiveAreaTag = 50829;
constexpr uint32_t kAsShotNeutralTag = 50728;
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

int mapCfaPattern(const QByteArray &pattern);
void unpackSamples(const QByteArray &encodedRow, int width, int bitsPerSample, quint16 *target);
int packedRowBytes(int width, int bitsPerSample);

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
           && photometric.first() == kPhotometricCfa;
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
    const TiffEntry *stripOffsetsEntry = findEntry(rawEntries, TIFFTAG_STRIPOFFSETS);
    const TiffEntry *stripByteCountsEntry = findEntry(rawEntries, TIFFTAG_STRIPBYTECOUNTS);

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
    if (const TiffEntry *rowsPerStripEntry = findEntry(rawEntries, TIFFTAG_ROWSPERSTRIP)) {
        const QVector<quint32> values = parseUnsignedArray(*rowsPerStripEntry, order);
        if (!values.isEmpty() && values.first() > 0) {
            rowsPerStrip = values.first();
        }
    }

    if (const TiffEntry *compressionEntry = findEntry(rawEntries, TIFFTAG_COMPRESSION)) {
        const QVector<quint32> values = parseUnsignedArray(*compressionEntry, order);
        if (!values.isEmpty() && values.first() != COMPRESSION_NONE) {
            return false;
        }
    }

    frame.pixels.resize(frame.width * frame.height);
    const int rowBytes = packedRowBytes(frame.width, frame.bitsPerSample);

    for (int stripIndex = 0; stripIndex < stripOffsets.size(); ++stripIndex) {
        const quint32 stripOffset = stripOffsets.at(stripIndex);
        const quint32 stripByteCount = stripByteCounts.at(qMin(stripIndex, stripByteCounts.size() - 1));
        const int startRow = stripIndex * static_cast<int>(rowsPerStrip);
        if (startRow >= frame.height) {
            break;
        }

        const int rowsInStrip = qMin<int>(static_cast<int>(rowsPerStrip), frame.height - startRow);
        const int requiredBytes = rowsInStrip * rowBytes;
        if (stripOffset > static_cast<quint32>(fileData.size())
            || stripByteCount > static_cast<quint32>(fileData.size())
            || stripOffset + stripByteCount > static_cast<quint32>(fileData.size())
            || stripByteCount < static_cast<quint32>(requiredBytes)) {
            return false;
        }

        const QByteArray stripData = fileData.mid(static_cast<int>(stripOffset), requiredBytes);
        for (int rowInStrip = 0; rowInStrip < rowsInStrip; ++rowInStrip) {
            const QByteArray rowSlice = stripData.mid(rowInStrip * rowBytes, rowBytes);
            unpackSamples(rowSlice,
                          frame.width,
                          frame.bitsPerSample,
                          frame.pixels.data() + ((startRow + rowInStrip) * frame.width));
        }
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

void unpackSamples(const QByteArray &encodedRow, int width, int bitsPerSample, quint16 *target)
{
    const unsigned char *data = reinterpret_cast<const unsigned char *>(encodedRow.constData());

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
        int src = 0;
        int dst = 0;
        while (dst + 3 < width && src + 4 < encodedRow.size()) {
            const unsigned char b0 = data[src++];
            const unsigned char b1 = data[src++];
            const unsigned char b2 = data[src++];
            const unsigned char b3 = data[src++];
            const unsigned char b4 = data[src++];

            target[dst++] = static_cast<quint16>(b0 | ((b4 & 0x03) << 8));
            target[dst++] = static_cast<quint16>(b1 | ((b4 & 0x0C) << 6));
            target[dst++] = static_cast<quint16>(b2 | ((b4 & 0x30) << 4));
            target[dst++] = static_cast<quint16>(b3 | ((b4 & 0xC0) << 2));
        }
        while (dst < width && src < encodedRow.size()) {
            target[dst++] = data[src++];
        }
        return;
    }

    if (bitsPerSample == 12) {
        int src = 0;
        int dst = 0;
        while (dst + 1 < width && src + 2 < encodedRow.size()) {
            const unsigned char b0 = data[src++];
            const unsigned char b1 = data[src++];
            const unsigned char b2 = data[src++];

            target[dst++] = static_cast<quint16>((static_cast<quint16>(b0) << 4) | (b1 >> 4));
            target[dst++] = static_cast<quint16>((static_cast<quint16>(b1 & 0x0F) << 8) | b2);
        }
        while (dst < width && src < encodedRow.size()) {
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

    return width > 0 && height > 0 && samplesPerPixel == 1 && photometric == kPhotometricCfa;
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

QByteArray readScanline(TIFF *tiff, uint32_t row, tsize_t scanlineSize)
{
    QByteArray buffer(scanlineSize, Qt::Uninitialized);
    if (TIFFReadScanline(tiff, buffer.data(), row, 0) < 0) {
        return {};
    }
    return buffer;
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

    for (uint32_t row = 0; row < static_cast<uint32_t>(frame.height); ++row) {
        const QByteArray rowBytes = readScanline(tiff, row, scanlineSize);
        if (rowBytes.isEmpty()) {
            frame.error = QStringLiteral("Failed to decode DNG scanline %1.").arg(row);
            return false;
        }

        unpackSamples(rowBytes, frame.width, frame.bitsPerSample, frame.pixels.data() + (row * frame.width));
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

        for (uint32_t rowInStrip = 0; rowInStrip < rowsInStrip; ++rowInStrip) {
            const QByteArray rowSlice = stripData.mid(rowInStrip * rowBytes, rowBytes);
            unpackSamples(rowSlice,
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

            for (uint32_t row = 0; row < copyRows; ++row) {
                const char *src = tileData.constData() + (row * tileRowBytes);
                QByteArray rowSlice(src, copyRowBytes);
                unpackSamples(rowSlice,
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
}

RawFrame DngDecoder::decodeFile(const QString &filePath)
{
    RawFrame frame;
    if (decodeManualRaw(filePath, frame)) {
        return frame;
    }

    TIFF *tiff = openTiffWithCustomTags(filePath, "r");
    if (!tiff) {
        frame.error = QStringLiteral("Could not open DNG file.");
        return frame;
    }

    if (!selectRawDirectory(tiff)) {
        frame.error = QStringLiteral("No CFA raw image directory found.");
        TIFFClose(tiff);
        return frame;
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
        return frame;
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
        return frame;
    }

    frame.valid = true;
    TIFFClose(tiff);
    return frame;
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
