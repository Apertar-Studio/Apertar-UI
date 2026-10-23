#include "MlvReader.hpp"
#include "RawPreview.hpp"

#include <QtEndian>
#include <array>
#include <cmath>
#include <cstdio>

namespace {
quint16 u16(const QByteArray &data, int offset)
{
    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(data.constData() + offset));
}
quint32 u32(const QByteArray &data, int offset)
{
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(data.constData() + offset));
}
qint32 s32(const QByteArray &data, int offset) { return static_cast<qint32>(u32(data, offset)); }
bool fail(QString *error, const QString &message)
{
    if (error) *error = message;
    return false;
}

struct PackedFrame {
    QFile &file;
    qint64 payload;
    int pitch;
    int bits;
    int height;
    struct Rows {
        int window = -1;
        QByteArray bytes;
    };
    mutable std::array<Rows, 2> cache;
    mutable QString error;

    const uchar *row(int y) const
    {
        if (!error.isEmpty()) return nullptr;
        const int window = y / 4;
        auto &rows = cache[window % cache.size()];
        if (rows.window != window) {
            const int first = window * 4;
            const qint64 bytes = qint64(qMin(4, height - first)) * pitch;
            rows.bytes.resize(bytes);
            if (!file.seek(payload + qint64(first) * pitch) || file.read(rows.bytes.data(), bytes) != bytes) {
                error = QStringLiteral("Could not read MLV pixels; check the recording media.");
                return nullptr;
            }
            rows.window = window;
        }
        return reinterpret_cast<const uchar *>(rows.bytes.constData()) + (y % 4) * pitch;
    }
};
quint16 packedSample(const void *context, int x, int y)
{
    const auto &frame = *static_cast<const PackedFrame *>(context);
    const uchar *row = frame.row(y);
    if (!row) return 0;
    if (frame.bits == 8) return row[x];
    if (frame.bits == 16) return qFromLittleEndian<quint16>(row + x * 2);
    // MLV packs MSB-first samples into little-endian 16-bit words (not CSI-2).
    const int offset = x * frame.bits;
    const int byte = (offset / 16) * 2;
    const quint32 first = row[byte] | (byte + 1 < frame.pitch ? quint32(row[byte + 1]) << 8 : 0);
    const quint32 second = byte + 2 < frame.pitch
        ? row[byte + 2] | (byte + 3 < frame.pitch ? quint32(row[byte + 3]) << 8 : 0) : 0;
    return ((first << 16 | second) >> (32 - (offset % 16) - frame.bits)) & ((1u << frame.bits) - 1);
}
}

bool MlvReader::blockAt(qint64 offset, QByteArray &header, quint32 &bytes, QString *error)
{
    if (offset < 0 || offset > m_file.size() - 16 || !m_file.seek(offset))
        return fail(error, QStringLiteral("Truncated MLV block."));
    header = m_file.read(16);
    if (header.size() != 16) return fail(error, QStringLiteral("Could not read MLV block."));
    bytes = u32(header, 4);
    if (bytes < 16 || qint64(bytes) > m_file.size() - offset)
        return fail(error, QStringLiteral("Invalid or truncated MLV block size."));
    return true;
}

bool MlvReader::rawInfo(const QByteArray &block, QString *error)
{
    if (block.size() < 180) return fail(error, QStringLiteral("Incomplete MLV RAW metadata."));
    const quint32 width = u32(block, 32), height = u32(block, 28), bits = u32(block, 44);
    const quint32 pitch = u32(block, 36);
    if (width < 2 || height < 2 || width > 16384 || height > 16384 ||
        (bits != 8 && bits != 10 && bits != 12 && bits != 14 && bits != 16) ||
        pitch < (quint64(width) * bits + 7) / 8 || pitch > 128 * 1024 ||
        quint64(pitch) * height > 256ull * 1024 * 1024)
        return fail(error, QStringLiteral("Unsupported MLV RAW geometry or bit depth."));
    m_raw.width = int(width);
    m_raw.height = int(height);
    m_raw.bitsPerSample = int(bits);
    m_raw.blackLevel = s32(block, 48);
    m_raw.whiteLevel = s32(block, 52);
    if (m_raw.blackLevel < 0 || m_raw.whiteLevel <= m_raw.blackLevel || m_raw.whiteLevel > 65535)
        return fail(error, QStringLiteral("Invalid MLV black/white levels."));
    switch (u32(block, 96)) {
    case 0x02010100: m_raw.cfaPattern = RawFrame::RGGB; break;
    case 0x00010102: m_raw.cfaPattern = RawFrame::BGGR; break;
    case 0x01000201: m_raw.cfaPattern = RawFrame::GBRG; break;
    case 0x01020001: m_raw.cfaPattern = RawFrame::GRBG; break;
    default: return fail(error, QStringLiteral("Unsupported MLV Bayer pattern."));
    }
    m_raw.activeTop = s32(block, 72);
    m_raw.activeLeft = s32(block, 76);
    m_raw.activeBottom = s32(block, 80);
    m_raw.activeRight = s32(block, 84);
    m_raw.hasActiveArea = m_raw.activeTop >= 0 && m_raw.activeLeft >= 0 &&
        m_raw.activeBottom > m_raw.activeTop && m_raw.activeRight > m_raw.activeLeft &&
        m_raw.activeBottom <= int(height) && m_raw.activeRight <= int(width);
    m_raw.hasColorMatrix1 = true;
    for (int i = 0; i < 9; ++i) {
        const qint32 denominator = s32(block, 108 + i * 8);
        if (!denominator) { m_raw.hasColorMatrix1 = false; break; }
        m_raw.colorMatrix1[i] = float(s32(block, 104 + i * 8)) / denominator;
        if (!std::isfinite(m_raw.colorMatrix1[i])) m_raw.hasColorMatrix1 = false;
    }
    m_raw.hasAsShotNeutral = true;
    m_raw.valid = true;
    m_pitch = int(pitch);
    m_payloadBytes = qint64(pitch) * height;
    return true;
}

bool MlvReader::whiteBalance(const QByteArray &block, QString *error)
{
    if (block.size() < 44) return fail(error, QStringLiteral("Incomplete MLV white balance."));
    const quint32 red = u32(block, 24), green = u32(block, 28), blue = u32(block, 32);
    if (!red || !green || !blue) return fail(error, QStringLiteral("Invalid MLV white balance gains."));
    m_neutralRed = float(green) / red;
    m_neutralBlue = float(green) / blue;
    return true;
}

bool MlvReader::previewColor(const QByteArray &block, QString *error)
{
    if (block.size() < 64 || u32(block, 16) != 0x4d435041 || u32(block, 20) != 40 || u32(block, 24) != 1)
        return fail(error, QStringLiteral("Unsupported MLV preview colour metadata."));
    std::array<float, 9> matrix{};
    for (size_t i = 0; i < matrix.size(); ++i)
        matrix[i] = float(s32(block, 28 + int(i) * 4)) / 100000.0f;
    if (m_previewMatrices.isEmpty() || m_previewMatrices.last() != matrix)
        m_previewMatrices.append(matrix);
    m_previewMatrix = m_previewMatrices.size() - 1;
    return true;
}

bool MlvReader::open(const QString &path, QString *error)
{
    m_file.close(); m_frames.clear(); m_raw = {}; m_info = {};
    m_neutralRed = m_neutralBlue = 1.0f;
    m_apertarColorMatrix = false;
    m_digitalGain = 1.0f;
    m_previewMatrices.clear();
    m_previewMatrix = -1;
    m_payloadBytes = 0;
    m_file.setFileName(path);
    if (!m_file.open(QIODevice::ReadOnly)) return fail(error, m_file.errorString());
    const QByteArray header = m_file.read(52);
    if (header.size() != 52 || header.left(4) != "MLVI" || u32(header, 4) < 52 || u32(header, 4) > 4096)
        return fail(error, QStringLiteral("Invalid MLV file header."));
    if (u16(header, 24) != 0 || u16(header, 26) > 1)
        return fail(error, QStringLiteral("Split MLV recordings are not supported."));
    if (u16(header, 32) != 1)
        return fail(error, QStringLiteral("Only uncompressed MLV RAW is supported."));
    if (!u32(header, 48)) return fail(error, QStringLiteral("Invalid MLV frame rate."));
    m_info.fps = double(u32(header, 44)) / u32(header, 48);
    if (!std::isfinite(m_info.fps) || m_info.fps < 0.1 || m_info.fps > 1000)
        return fail(error, QStringLiteral("Invalid MLV frame rate."));
    const quint32 count = u32(header, 36);
    if (!count || count > 2000000)
        return fail(error, QStringLiteral("MLV is not finalized or its frame count is invalid."));
    m_info.frames = int(count);
    m_info.durationMs = qRound64(count * 1000.0 / m_info.fps);
    m_info.pixelAspect = 1.0;
    std::snprintf(m_info.codec, sizeof(m_info.codec), "MLV");
    std::snprintf(m_info.profile, sizeof(m_info.profile), "RAW");
    m_scanOffset = u32(header, 4);
    for (int blocks = 0; blocks < 4096; ++blocks) {
        QByteArray block;
        quint32 bytes;
        if (!blockAt(m_scanOffset, block, bytes, error)) return false;
        if (block.left(4) == "VIDF") {
            if (!m_raw.valid) return fail(error, QStringLiteral("MLV has no RAW metadata."));
            if (count > quint64(m_file.size() - m_scanOffset) / (32 + m_payloadBytes))
                return fail(error, QStringLiteral("MLV frame count exceeds its file size."));
            m_info.width = m_raw.width; m_info.height = m_raw.height;
            m_info.bits = m_raw.bitsPerSample;
            return true;
        }
        const auto tag = block.left(4);
        if (tag == "RAWI" || tag == "WBAL" || tag == "IDNT" || tag == "EXPO" || tag == "DEBG") {
            const int required = tag == "RAWI" ? 180 : tag == "WBAL" ? 44 : tag == "IDNT" ? 52 : tag == "DEBG" ? 24 : 32;
            if (bytes < quint32(required)) return fail(error, QStringLiteral("Truncated MLV metadata."));
            block += m_file.read(required - 16);
            if (tag == "RAWI" && !rawInfo(block, error)) return false;
            if (tag == "WBAL" && !whiteBalance(block, error)) return false;
            if (tag == "DEBG" && u32(block, 16) == 0x4d435041) {
                if (bytes < 64) return fail(error, QStringLiteral("Truncated MLV preview colour metadata."));
                block += m_file.read(40);
                if (!previewColor(block, error)) return false;
            }
            // Older Apertar files stored the ISP CCM directly in RAWI. Newer
            // files use standard RAWI plus APCM; retain playback of both.
            if (tag == "IDNT") m_apertarColorMatrix = u32(block, 48) == 0xA9000001u;
            if (tag == "EXPO" && u32(block, 28) > 0) m_digitalGain = float(u32(block, 28)) / 1024.0f;
        }
        m_scanOffset += bytes;
    }
    return fail(error, QStringLiteral("MLV has too many header blocks."));
}

bool MlvReader::ensureFrame(int frame, QString *error, const std::atomic<bool> *active)
{
    if (frame < 0 || frame >= m_info.frames) return fail(error, QStringLiteral("MLV frame is out of range."));
    while (m_frames.size() <= frame) {
        if (active && !active->load(std::memory_order_relaxed))
            return fail(error, QStringLiteral("MLV playback stopped."));
        QByteArray block;
        quint32 bytes;
        if (!blockAt(m_scanOffset, block, bytes, error)) return false;
        const auto tag = block.left(4);
        if (tag == "VIDF") {
            if (bytes < 32) return fail(error, QStringLiteral("Truncated MLV frame header."));
            block += m_file.read(16);
            if (block.size() != 32) return fail(error, QStringLiteral("Could not read MLV frame header."));
            const quint32 space = u32(block, 28);
            if (qint64(space) + m_payloadBytes > qint64(bytes) - 32)
                return fail(error, QStringLiteral("Truncated MLV RAW payload."));
            m_frames.append({m_scanOffset + 32 + space, m_neutralRed, m_neutralBlue, m_previewMatrix});
        } else if (tag == "DEBG") {
            if (bytes < 24) return fail(error, QStringLiteral("Truncated MLV debug metadata."));
            block += m_file.read(8);
            if (u32(block, 16) == 0x4d435041) {
                if (bytes < 64) return fail(error, QStringLiteral("Truncated MLV preview colour metadata."));
                block += m_file.read(40);
                if (!previewColor(block, error)) return false;
            }
        } else if (tag == "WBAL") {
            if (bytes < 44) return fail(error, QStringLiteral("Truncated MLV white balance."));
            block += m_file.read(28);
            if (!whiteBalance(block, error)) return false;
        } else if (tag == "RAWI") {
            return fail(error, QStringLiteral("MLV changes RAW geometry within the clip."));
        }
        m_scanOffset += bytes;
    }
    return true;
}

QImage MlvReader::render(int frame, const QSize &size, QString *error, const std::atomic<bool> *active)
{
    if (!ensureFrame(frame, error, active)) return {};
    if (active && !active->load(std::memory_order_relaxed)) return {};
    const Entry entry = m_frames.at(frame);
    if (entry.payload > m_file.size() - m_payloadBytes) {
        fail(error, QStringLiteral("MLV was truncated or its recording media is unavailable."));
        return {};
    }
    // A tiny row cache avoids a full RAW buffer and remains safe if removable
    // media disappears during playback (unlike faulting a file-backed mapping).
    const PackedFrame packed {m_file, entry.payload, m_pitch, m_raw.bitsPerSample, m_raw.height, {}, {}};
    m_raw.asShotNeutral[0] = entry.neutralRed;
    m_raw.asShotNeutral[2] = entry.neutralBlue;
    const float *previewMatrix = entry.previewMatrix >= 0
        ? m_previewMatrices.at(entry.previewMatrix).data()
        : (m_apertarColorMatrix && m_raw.hasColorMatrix1 ? m_raw.colorMatrix1 : nullptr);
    const QImage image = renderRawFramePreview(m_raw, size, packedSample, &packed,
        previewMatrix, m_digitalGain);
    if (!packed.error.isEmpty()) { fail(error, packed.error); return {}; }
    if (image.isNull()) fail(error, QStringLiteral("Could not render MLV RAW frame."));
    return image;
}
