#pragma once

#include "rawframe.h"
#include <QImage>

using RawPreviewSample = quint16 (*)(const void *, int, int);

// The optional accessor lets packed RAW containers reuse the DNG renderer
// without allocating an unpacked, full-resolution image.
QImage renderRawFramePreview(const RawFrame &frame, const QSize &size,
                             RawPreviewSample sample = nullptr, const void *context = nullptr,
                             const float *cameraToLinearSrgb = nullptr, float digitalGain = 1.0f);
