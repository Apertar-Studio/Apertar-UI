#pragma once

#include <stdint.h>

// Private, versioned ABI: the UI does not link FFmpeg into its startup path.
struct ApertarVideoInfo {
    int width;
    int height;
    int frames;
    int bits;
    double fps;
    int64_t durationMs;
    double pixelAspect;
    char profile[64];
    char captured[64];
    char codec[16];
};

struct ApertarVideoApi {
    int version;
    int (*probe)(const char *, ApertarVideoInfo *, char *, int);
    void *(*open)(const char *, char *, int);
    int (*read)(void *, int, unsigned char *, int, int, int, char *, int);
    void (*close)(void *);
};

extern "C" const ApertarVideoApi *apertar_video_api();
