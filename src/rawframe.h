#pragma once

#include <QVector>

struct RawFrame
{
    enum CfaPattern {
        RGGB = 0,
        BGGR = 1,
        GRBG = 2,
        GBRG = 3
    };

    bool valid = false;
    int width = 0;
    int height = 0;
    int bitsPerSample = 0;
    int cfaPattern = RGGB;
    float blackLevel = 0.0f;
    float whiteLevel = 1023.0f;
    bool hasAsShotNeutral = false;
    float asShotNeutral[3] = {1.0f, 1.0f, 1.0f};
    bool hasAnalogBalance = false;
    float analogBalance[3] = {1.0f, 1.0f, 1.0f};
    bool hasColorMatrix1 = false;
    float colorMatrix1[9] = {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };
    bool hasActiveArea = false;
    int activeTop = 0;
    int activeLeft = 0;
    int activeBottom = 0;
    int activeRight = 0;
    QVector<quint16> pixels;
    QString error;
};
