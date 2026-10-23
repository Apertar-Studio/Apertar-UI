#include "tiffhelper.h"

#include <QFile>

#include <mutex>

namespace {

TIFFExtendProc g_previousTagExtender = nullptr;

void mergeCustomTiffFields(TIFF *tiff)
{
    static const TIFFFieldInfo kFieldInfo[] = {
        {static_cast<ttag_t>(kTiffTagDateTimeOriginal), -1, -1, TIFF_ASCII, FIELD_CUSTOM, true, false,
         const_cast<char *>("DateTimeOriginal")},
        {static_cast<ttag_t>(kTiffTagTiffEpStandardId), 4, 4, TIFF_BYTE, FIELD_CUSTOM, true, false,
         const_cast<char *>("TIFFEPStandardID")},
        {static_cast<ttag_t>(kTiffTagTimeCode), 8, 8, TIFF_BYTE, FIELD_CUSTOM, true, false,
         const_cast<char *>("TimeCode")},
        {static_cast<ttag_t>(kTiffTagFrameRate), 1, 1, TIFF_SRATIONAL, FIELD_CUSTOM, true, false,
         const_cast<char *>("FrameRate")},
        {static_cast<ttag_t>(kTiffTagReelName), -1, -1, TIFF_ASCII, FIELD_CUSTOM, true, false,
         const_cast<char *>("ReelName")},
        {static_cast<ttag_t>(kTiffTagCameraLabel), -1, -1, TIFF_ASCII, FIELD_CUSTOM, true, false,
         const_cast<char *>("CameraLabel")},
        {static_cast<ttag_t>(kTiffTagForwardMatrix1), 9, 9, TIFF_SRATIONAL, FIELD_CUSTOM, true, false,
         const_cast<char *>("ForwardMatrix1")},
        {static_cast<ttag_t>(kTiffTagForwardMatrix2), 9, 9, TIFF_SRATIONAL, FIELD_CUSTOM, true, false,
         const_cast<char *>("ForwardMatrix2")},
    };

    TIFFMergeFieldInfo(tiff, kFieldInfo, sizeof(kFieldInfo) / sizeof(kFieldInfo[0]));
}

void customTiffTagExtender(TIFF *tiff)
{
    mergeCustomTiffFields(tiff);
    if (g_previousTagExtender) {
        g_previousTagExtender(tiff);
    }
}

uint16_t readTiffUInt16(const uchar *bytes, bool littleEndian)
{
    if (littleEndian)
        return static_cast<uint16_t>(bytes[0] | (static_cast<uint16_t>(bytes[1]) << 8));
    return static_cast<uint16_t>((static_cast<uint16_t>(bytes[0]) << 8) | bytes[1]);
}

uint32_t readTiffUInt32(const uchar *bytes, bool littleEndian)
{
    if (littleEndian) {
        return static_cast<uint32_t>(bytes[0]) |
               (static_cast<uint32_t>(bytes[1]) << 8) |
               (static_cast<uint32_t>(bytes[2]) << 16) |
               (static_cast<uint32_t>(bytes[3]) << 24);
    }

    return (static_cast<uint32_t>(bytes[0]) << 24) |
           (static_cast<uint32_t>(bytes[1]) << 16) |
           (static_cast<uint32_t>(bytes[2]) << 8) |
           static_cast<uint32_t>(bytes[3]);
}

bool hasZeroFirstDirectoryCount(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    uchar header[8]{};
    if (file.read(reinterpret_cast<char *>(header), sizeof(header)) != static_cast<qint64>(sizeof(header)))
        return false;

    const bool littleEndian = header[0] == 'I' && header[1] == 'I';
    const bool bigEndian = header[0] == 'M' && header[1] == 'M';
    if (!littleEndian && !bigEndian)
        return false;

    if (readTiffUInt16(header + 2, littleEndian) != 42)
        return false;

    const uint32_t firstIfdOffset = readTiffUInt32(header + 4, littleEndian);
    if (firstIfdOffset == 0 || firstIfdOffset > static_cast<uint64_t>(file.size() - 2))
        return false;

    if (!file.seek(firstIfdOffset))
        return false;

    uchar countBytes[2]{};
    if (file.read(reinterpret_cast<char *>(countBytes), sizeof(countBytes)) != static_cast<qint64>(sizeof(countBytes)))
        return false;

    return readTiffUInt16(countBytes, littleEndian) == 0;
}

} // namespace

void ensureCustomTiffTagsRegistered()
{
    static std::once_flag once;
    std::call_once(once, []() {
        g_previousTagExtender = TIFFSetTagExtender(customTiffTagExtender);
    });
}

TIFF *openTiffWithCustomTags(const QString &filePath, const char *mode)
{
    ensureCustomTiffTagsRegistered();
    if (hasZeroFirstDirectoryCount(filePath))
        return nullptr;
    return TIFFOpen(filePath.toLocal8Bit().constData(), mode);
}
