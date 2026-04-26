#pragma once

#include <cstdint>

#include <QString>

#include <tiffio.h>

constexpr uint32_t kTiffTagDateTimeOriginal = 0x9003;
constexpr uint32_t kTiffTagTimeCode = 0xC763;
constexpr uint32_t kTiffTagFrameRate = 0xC764;

void ensureCustomTiffTagsRegistered();
TIFF *openTiffWithCustomTags(const QString &filePath, const char *mode);
