#pragma once

#include <QString>

#include "rawframe.h"

class DngDecoder
{
public:
    static RawFrame decodeFile(const QString &filePath);
    static double detectFrameRate(const QString &filePath);
};
