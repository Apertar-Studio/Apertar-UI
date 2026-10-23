#pragma once

#include <cstdint>

#include <QString>

#include <tiffio.h>

constexpr uint32_t kTiffTagDateTimeOriginal = 0x9003;
constexpr uint32_t kTiffTagTiffEpStandardId = 0x9216;
constexpr uint32_t kTiffTagTimeCode = 0xC763;
constexpr uint32_t kTiffTagFrameRate = 0xC764;
constexpr uint32_t kTiffTagReelName = 0xC789;
constexpr uint32_t kTiffTagCameraLabel = 0xC7A1;
constexpr uint32_t kTiffTagForwardMatrix1 = 0xC714;
constexpr uint32_t kTiffTagForwardMatrix2 = 0xC715;

void ensureCustomTiffTagsRegistered();
TIFF *openTiffWithCustomTags(const QString &filePath, const char *mode);
