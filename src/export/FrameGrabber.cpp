#include "FrameGrabber.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <cmath>

namespace afvp {

struct FrameGrabber::Private {
    QString path;
    bool opened = false;
    AVFormatContext *format = nullptr;
    AVCodecContext *codec = nullptr;
    SwsContext *scaler = nullptr;
    AVPacket *packet = nullptr;
    AVFrame *frame = nullptr;
    int streamIndex = -1;

    ~Private() {
        sws_freeContext(scaler);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codec);
        avformat_close_input(&format);
    }

    // Opens the file and the decoder of its main video stream on first use.
    bool open() {
        if (opened) {
            return codec != nullptr;
        }

        opened = true;

        if (avformat_open_input(&format, path.toUtf8().constData(), nullptr, nullptr) < 0 ||
            avformat_find_stream_info(format, nullptr) < 0) {
            return false;
        }

        // The decoder is found separately: the type of av_find_best_stream's decoder argument differs between
        // FFmpeg versions (const since FFmpeg 5).
        streamIndex = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);

        if (streamIndex < 0 || (format->streams[streamIndex]->disposition & AV_DISPOSITION_ATTACHED_PIC) != 0) {
            return false;
        }

        const AVCodec *decoder = avcodec_find_decoder(format->streams[streamIndex]->codecpar->codec_id);

        if (!decoder) {
            return false;
        }

        // Only the decoded stream is read.
        for (unsigned i = 0; i < format->nb_streams; ++i) {
            format->streams[i]->discard = int(i) == streamIndex ? AVDISCARD_DEFAULT : AVDISCARD_ALL;
        }

        codec = avcodec_alloc_context3(decoder);
        packet = av_packet_alloc();
        frame = av_frame_alloc();

        if (!codec || !packet || !frame ||
            avcodec_parameters_to_context(codec, format->streams[streamIndex]->codecpar) < 0) {
            avcodec_free_context(&codec);

            return false;
        }

        // Frame threading would hold the first frame back until several packets are decoded.
        codec->thread_count = 0;
        codec->thread_type = FF_THREAD_SLICE;

        if (avcodec_open2(codec, decoder, nullptr) < 0) {
            avcodec_free_context(&codec);

            return false;
        }

        return true;
    }

    // The first frame decoded after a seek to the keyframe at or before the time.
    bool decodeAt(double seconds) {
        const AVStream *stream = format->streams[streamIndex];
        const int64_t start = stream->start_time != AV_NOPTS_VALUE ? stream->start_time : 0;
        const int64_t timestamp = start + av_rescale_q(int64_t(std::max(0.0, seconds) * AV_TIME_BASE),
                                                       AV_TIME_BASE_Q, stream->time_base);

        if (av_seek_frame(format, streamIndex, timestamp, AVSEEK_FLAG_BACKWARD) < 0) {
            return false;
        }

        avcodec_flush_buffers(codec);
        bool flushed = false;

        while (true) {
            const int received = avcodec_receive_frame(codec, frame);

            if (received == 0) {
                return true;
            }

            if (received != AVERROR(EAGAIN) || flushed) {
                return false;
            }

            if (av_read_frame(format, packet) < 0) {
                // End of file: take the frames still in the decoder.
                avcodec_send_packet(codec, nullptr);
                flushed = true;

                continue;
            }

            if (packet->stream_index == streamIndex) {
                avcodec_send_packet(codec, packet);
            }

            av_packet_unref(packet);
        }
    }

    VideoFrame scale(int width) {
        if (frame->width <= 0 || frame->height <= 0) {
            return {};
        }

        const AVRational sampleAspect = av_guess_sample_aspect_ratio(format, format->streams[streamIndex], frame);
        const double pixelAspect = sampleAspect.num > 0 && sampleAspect.den > 0 ? av_q2d(sampleAspect) : 1.0;
        const int height =
            std::max(1, int(std::lround(double(width) * frame->height / (frame->width * pixelAspect))));

        scaler = sws_getCachedContext(scaler, frame->width, frame->height, AVPixelFormat(frame->format), width,
                                      height, AV_PIX_FMT_RGB32, SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!scaler) {
            return {};
        }

        VideoFrame result;
        result.width = width;
        result.height = height;
        result.pixels.resize(qsizetype(width) * height * 4);
        uint8_t *destination[4] = {reinterpret_cast<uint8_t *>(result.pixels.data()), nullptr, nullptr, nullptr};
        const int destinationStride[4] = {width * 4, 0, 0, 0};

        if (sws_scale(scaler, frame->data, frame->linesize, 0, frame->height, destination, destinationStride) <= 0) {
            return {};
        }

        return result;
    }
};

FrameGrabber::FrameGrabber(const QString &videoPath) : d(std::make_unique<Private>()) {
    d->path = videoPath;
}

FrameGrabber::~FrameGrabber() = default;

VideoFrame FrameGrabber::grab(double seconds, int width) {
    if (width <= 0 || !d->open() || !d->decodeAt(seconds)) {
        return {};
    }

    VideoFrame result = d->scale(width);
    av_frame_unref(d->frame);

    return result;
}

} // namespace afvp
