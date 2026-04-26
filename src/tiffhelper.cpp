#include "tiffhelper.h"

#include <mutex>

namespace {

TIFFExtendProc g_previousTagExtender = nullptr;

void mergeCustomTiffFields(TIFF *tiff)
{
    static const TIFFFieldInfo kFieldInfo[] = {
        {static_cast<ttag_t>(kTiffTagDateTimeOriginal), -1, -1, TIFF_ASCII, FIELD_CUSTOM, true, false,
         const_cast<char *>("DateTimeOriginal")},
        {static_cast<ttag_t>(kTiffTagTimeCode), 8, 8, TIFF_BYTE, FIELD_CUSTOM, true, false,
         const_cast<char *>("TimeCode")},
        {static_cast<ttag_t>(kTiffTagFrameRate), 1, 1, TIFF_SRATIONAL, FIELD_CUSTOM, true, false,
         const_cast<char *>("FrameRate")},
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
    return TIFFOpen(filePath.toLocal8Bit().constData(), mode);
}
