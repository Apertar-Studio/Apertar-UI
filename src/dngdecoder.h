#pragma once

#include <QString>
#include <QVariantList>

#include "rawframe.h"

class DngDecoder
{
public:
    static RawFrame decodeFile(const QString &filePath);
    static bool decodeFileInto(const QString &filePath, RawFrame &frame);
    static double detectFrameRate(const QString &filePath);
    static QVariantList decodeHistogramBins(const QString &filePath, int binCount, int targetSamples);
};
