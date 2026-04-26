#pragma once

#include <cstdint>

struct PreviewFrameInfo
{
    int procid = -1;
    int fdRaw = -1;
    int fdIsp = -1;
    int fallbackFdIsp = -1;
    int planeCount = 0;
    int planeFds[3] = { -1, -1, -1 };
    unsigned int planeOffsets[3] = { 0, 0, 0 };
    unsigned int planePitches[3] = { 0, 0, 0 };
    uint64_t frame = 0;
    unsigned int sequence = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int stride = 0;
    unsigned int captureWidth = 0;
    unsigned int captureHeight = 0;
    unsigned int fallbackWidth = 0;
    unsigned int fallbackHeight = 0;
    unsigned int fallbackStride = 0;
};
