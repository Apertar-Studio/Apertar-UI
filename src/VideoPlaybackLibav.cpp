#include "VideoPlaybackApi.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>

namespace {
int fail(char *error, int size, const char *message, int code = 0)
{
    char detail[AV_ERROR_MAX_STRING_SIZE] = {};
    if (code < 0) av_strerror(code, detail, sizeof(detail));
    if (error && size > 0)
        std::snprintf(error, size, "%s%s%s", message, code < 0 ? ": " : "", detail);
    return 0;
}

struct Decoder {
    AVFormatContext *format = nullptr;
    AVCodecContext *codec = nullptr;
    AVPacket *packet = nullptr;
    AVFrame *frame = nullptr;
    SwsContext *scale = nullptr;
    int stream = -1;
    int lastFrame = -1;
    bool draining = false;
    bool independent = false;
    ApertarVideoInfo info {};
    ~Decoder()
    {
        sws_freeContext(scale);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codec);
        avformat_close_input(&format);
    }
};

bool container(Decoder &d, const char *path, char *error, int size)
{
    int ret = avformat_open_input(&d.format, path, nullptr, nullptr);
    if (ret < 0) return fail(error, size, "Cannot open movie", ret);
    // MOV/MP4 sample tables contain geometry, duration and frame rate.
    // Avoid decoding a frame just to populate the clip browser.
    d.stream = av_find_best_stream(d.format, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (d.stream < 0) return fail(error, size, "No video stream", d.stream);
    auto *stream = d.format->streams[d.stream];
    auto *parameters = stream->codecpar;
    if (parameters->codec_id != AV_CODEC_ID_PRORES && parameters->codec_id != AV_CODEC_ID_H264)
        return fail(error, size, "This movie is not ProRes or H.264");
    d.independent = parameters->codec_id == AV_CODEC_ID_PRORES;
    if (!d.independent) {
        // Parse the SPS and timing, including 8/10-bit pixel format. MP4 headers
        // alone may not provide these; cache the result on the UI side.
        ret = avformat_find_stream_info(d.format, nullptr);
        if (ret < 0) return fail(error, size, "Cannot read H.264 stream information", ret);
    }
    if (parameters->width <= 0 || parameters->height <= 0 ||
        parameters->width > 16384 || parameters->height > 16384)
        return fail(error, size, "Invalid movie dimensions");
    const AVRational rate = av_guess_frame_rate(d.format, stream, nullptr);
    d.info.fps = av_q2d(rate);
    if (!std::isfinite(d.info.fps) || d.info.fps <= 0 || d.info.fps > 1000)
        return fail(error, size, "Invalid movie frame rate");
    d.info.width = parameters->width;
    d.info.height = parameters->height;
    const auto *pixelFormat = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(parameters->format));
    d.info.bits = parameters->bits_per_raw_sample > 0 ? parameters->bits_per_raw_sample
        : pixelFormat ? pixelFormat->comp[0].depth : d.independent ? 10 : 8;
    d.info.durationMs = stream->duration != AV_NOPTS_VALUE
        ? av_rescale_q(stream->duration, stream->time_base, {1, 1000})
        : std::max<int64_t>(0, d.format->duration / 1000);
    const double count = stream->nb_frames > 0 ? double(stream->nb_frames)
        : std::round(d.info.durationMs * d.info.fps / 1000.0);
    if (count < 1 || count > INT32_MAX)
        return fail(error, size, "Movie has no completed frames");
    d.info.frames = int(count);
    d.info.pixelAspect = av_q2d(stream->sample_aspect_ratio);
    if (!std::isfinite(d.info.pixelAspect) || d.info.pixelAspect <= 0)
        d.info.pixelAspect = 1.0;
    std::snprintf(d.info.codec, sizeof(d.info.codec), "%s", d.independent ? "ProRes" : "H.264");
    const char *profile = avcodec_profile_name(parameters->codec_id, parameters->profile);
    if (!profile && d.independent) {
        switch (parameters->codec_tag) {
        case MKTAG('a','p','c','o'): profile = "Proxy"; break;
        case MKTAG('a','p','c','s'): profile = "LT"; break;
        case MKTAG('a','p','c','n'): profile = "422"; break;
        case MKTAG('a','p','c','h'): profile = "HQ"; break;
        default: break;
        }
    }
    std::snprintf(d.info.profile, sizeof(d.info.profile), "%s", profile ? profile : "");
    auto *date = av_dict_get(d.format->metadata, "creation_time", nullptr, 0);
    if (!date) date = av_dict_get(stream->metadata, "creation_time", nullptr, 0);
    if (date) std::snprintf(d.info.captured, sizeof(d.info.captured), "%s", date->value);
    return true;
}

int probe(const char *path, ApertarVideoInfo *info, char *error, int size)
{
    Decoder d;
    if (!container(d, path, error, size)) return 0;
    *info = d.info;
    return 1;
}

void *open(const char *path, char *error, int size)
{
    auto d = std::make_unique<Decoder>();
    if (!container(*d, path, error, size)) return nullptr;
    const AVCodec *codec = avcodec_find_decoder(d->format->streams[d->stream]->codecpar->codec_id);
    if (!codec) { fail(error, size, "Video decoder unavailable"); return nullptr; }
    d->codec = avcodec_alloc_context3(codec);
    d->packet = av_packet_alloc();
    d->frame = av_frame_alloc();
    if (!d->codec || !d->packet || !d->frame) {
        fail(error, size, "Not enough memory for playback"); return nullptr;
    }
    int ret = avcodec_parameters_to_context(d->codec, d->format->streams[d->stream]->codecpar);
    if (ret < 0) { fail(error, size, "Invalid video parameters", ret); return nullptr; }
    d->codec->thread_count = 4;
    d->codec->thread_type = d->independent ? FF_THREAD_SLICE : FF_THREAD_FRAME | FF_THREAD_SLICE;
    // Playback is display-sized; let decoders supporting reduced IDCT avoid 4K work.
    d->codec->lowres = std::min<int>(codec->max_lowres, 1);
    ret = avcodec_open2(d->codec, codec, nullptr);
    if (ret < 0) { fail(error, size, "Cannot start video decoder", ret); return nullptr; }
    return d.release();
}

int read(void *handle, int index, unsigned char *pixels, int width, int height,
         int stride, char *error, int size)
{
    auto &d = *static_cast<Decoder *>(handle);
    if (index < 0 || index >= d.info.frames || width <= 0 || height <= 0 ||
        width > 1920 || height > 1920 || stride < width * 4)
        return fail(error, size, "Invalid playback frame");
    auto *stream = d.format->streams[d.stream];
    const AVRational frameTime = av_inv_q(av_guess_frame_rate(d.format, stream, nullptr));
    const int64_t start = stream->start_time == AV_NOPTS_VALUE ? 0 : stream->start_time;
    const int64_t wanted = start + av_rescale_q(index, frameTime, stream->time_base);
    if (index != d.lastFrame) {
        if (d.lastFrame < 0 || index <= d.lastFrame || index > d.lastFrame + (d.independent ? 8 : 120)) {
            int ret = av_seek_frame(d.format, d.stream, wanted, AVSEEK_FLAG_BACKWARD);
            if (ret < 0) return fail(error, size, "Movie seek failed", ret);
            avcodec_flush_buffers(d.codec);
            d.draining = false;
        }
        d.lastFrame = -1;
        bool found = false;
        for (int attempts = 0; attempts < 16384; ++attempts) {
            int ret = avcodec_receive_frame(d.codec, d.frame);
            if (ret == 0) {
                const int64_t pts = d.frame->best_effort_timestamp;
                if (pts != AV_NOPTS_VALUE && pts < wanted) continue;
                d.lastFrame = index;
                found = true;
                break;
            }
            if (ret != AVERROR(EAGAIN))
                return fail(error, size, "Video frame unavailable", ret);
            if (d.draining) return fail(error, size, "Incomplete video stream");
            av_packet_unref(d.packet);
            ret = av_read_frame(d.format, d.packet);
            if (ret == AVERROR_EOF) {
                d.draining = true;
                ret = avcodec_send_packet(d.codec, nullptr);
                if (ret < 0 && ret != AVERROR_EOF)
                    return fail(error, size, "Cannot finish video decode", ret);
                continue;
            }
            if (ret < 0) return fail(error, size, "Cannot read movie frame", ret);
            if (d.packet->stream_index != d.stream) continue;
            const int64_t pts = d.packet->pts != AV_NOPTS_VALUE ? d.packet->pts : d.packet->dts;
            // Every ProRes frame is independent: skipped display frames need no decode.
            // H.264 must instead decode references and reordered B-frames.
            if (d.independent && pts != AV_NOPTS_VALUE && pts < wanted) continue;
            ret = avcodec_send_packet(d.codec, d.packet);
            av_packet_unref(d.packet);
            if (ret < 0) return fail(error, size, "Video decode failed", ret);
        }
        if (!found) return fail(error, size, "Movie frame not found");
    }
    auto pixelFormat = static_cast<AVPixelFormat>(d.frame->format);
    bool fullRange = d.frame->color_range == AVCOL_RANGE_JPEG;
    // Normalize deprecated JPEG-format aliases so swscale can reuse its context.
    switch (pixelFormat) {
    case AV_PIX_FMT_YUVJ420P: pixelFormat = AV_PIX_FMT_YUV420P; fullRange = true; break;
    case AV_PIX_FMT_YUVJ422P: pixelFormat = AV_PIX_FMT_YUV422P; fullRange = true; break;
    case AV_PIX_FMT_YUVJ444P: pixelFormat = AV_PIX_FMT_YUV444P; fullRange = true; break;
    case AV_PIX_FMT_YUVJ440P: pixelFormat = AV_PIX_FMT_YUV440P; fullRange = true; break;
    default: break;
    }
    d.scale = sws_getCachedContext(d.scale, d.frame->width, d.frame->height,
        pixelFormat, width, height, AV_PIX_FMT_BGRA,
        SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    if (!d.scale) return fail(error, size, "Cannot scale movie frame");
    int matrix = SWS_CS_ITU709;
    const auto space = d.frame->colorspace;
    if (space == AVCOL_SPC_BT470BG || space == AVCOL_SPC_SMPTE170M) matrix = SWS_CS_ITU601;
    else if (space == AVCOL_SPC_BT2020_NCL) matrix = SWS_CS_BT2020;
    sws_setColorspaceDetails(d.scale, sws_getCoefficients(matrix),
        fullRange, sws_getCoefficients(matrix),
        1, 0, 1 << 16, 1 << 16);
    unsigned char *planes[] = {pixels, nullptr, nullptr, nullptr};
    int strides[] = {stride, 0, 0, 0};
    if (sws_scale(d.scale, d.frame->data, d.frame->linesize, 0, d.frame->height,
                  planes, strides) != height)
        return fail(error, size, "Incomplete movie frame");
    return 1;
}

void close(void *handle) { delete static_cast<Decoder *>(handle); }
}

extern "C" const ApertarVideoApi *apertar_video_api()
{
    static const ApertarVideoApi api {2, probe, open, read, close};
    return &api;
}
