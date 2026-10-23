#pragma once

#include "VideoPlaybackApi.h"
#include "rawframe.h"
#include <QFile>
#include <QImage>
#include <QVector>
#include <array>
#include <atomic>

// Apertar's single-file, uncompressed MLV RAW layout. The index contains only
// frame locations and per-frame WB; pixel data stays on disk until requested.
class MlvReader
{
public:
    bool open(const QString &path, QString *error = nullptr);
    const ApertarVideoInfo &info() const { return m_info; }
    QImage render(int frame, const QSize &size, QString *error = nullptr,
                  const std::atomic<bool> *active = nullptr);
private:
    struct Entry {
        qint64 payload;
        float neutralRed;
        float neutralBlue;
        int previewMatrix;
    };
    bool blockAt(qint64 offset, QByteArray &header, quint32 &bytes, QString *error);
    bool rawInfo(const QByteArray &block, QString *error);
    bool whiteBalance(const QByteArray &block, QString *error);
    bool previewColor(const QByteArray &block, QString *error);
    bool ensureFrame(int frame, QString *error, const std::atomic<bool> *active);
    QFile m_file;
    ApertarVideoInfo m_info {};
    RawFrame m_raw;
    QVector<Entry> m_frames;
    qint64 m_scanOffset = 0;
    qint64 m_payloadBytes = 0;
    int m_pitch = 0;
    float m_neutralRed = 1.0f;
    float m_neutralBlue = 1.0f;
    bool m_apertarColorMatrix = false;
    float m_digitalGain = 1.0f;
    QVector<std::array<float, 9>> m_previewMatrices;
    int m_previewMatrix = -1;
};
