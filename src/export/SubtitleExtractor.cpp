#include "SubtitleExtractor.h"

#include <QElapsedTimer>
#include <QFile>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
}

#include <cstring>
#include <memory>
#include <vector>

namespace afvp {

namespace {

constexpr AVRational millisecondTimeBase{1, 1000};

// Used by the SubRip encoder when the decoder provides no ASS header.
constexpr char defaultAssHeader[] =
    "[Script Info]\r\nScriptType: v4.00+\r\nPlayResX: 384\r\nPlayResY: 288\r\n\r\n"
    "[V4+ Styles]\r\nFormat: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, "
    "Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, "
    "MarginL, MarginR, MarginV, Encoding\r\n"
    "Style: Default,Arial,16,&Hffffff,&Hffffff,&H0,&H0,0,0,0,0,100,100,0,0,1,1,0,2,10,10,10,0\r\n\r\n"
    "[Events]\r\nFormat: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\r\n";

QString errorText(int code) {
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(code, buffer, sizeof buffer);

    return QString::fromUtf8(buffer);
}

void setError(QString *error, const QString &text) {
    if (error) {
        *error = text;
    }
}

int interruptCallback(void *opaque) {
    return static_cast<const std::atomic_bool *>(opaque)->load() ? 1 : 0;
}

struct InputDeleter {
    void operator()(AVFormatContext *context) const {
        avformat_close_input(&context);
    }
};

using InputContext = std::unique_ptr<AVFormatContext, InputDeleter>;

InputContext openInput(const QString &path, const std::atomic_bool *cancel, QString *error) {
    AVFormatContext *context = avformat_alloc_context();

    if (cancel) {
        context->interrupt_callback.callback = interruptCallback;
        context->interrupt_callback.opaque = const_cast<std::atomic_bool *>(cancel);
    }

    // FFmpeg expects UTF-8 paths on all platforms.
    const int result = avformat_open_input(&context, path.toUtf8().constData(), nullptr, nullptr);

    if (result < 0) {
        setError(error, errorText(result));

        return {};
    }

    // Matroska and MP4 describe all streams in the header; other containers may need probing.
    const QString formatName = QString::fromLatin1(context->iformat->name);

    if (!formatName.contains(u"matroska") && !formatName.contains(u"mov")) {
        avformat_find_stream_info(context, nullptr);
    }

    return InputContext(context);
}

QString metadataValue(const AVDictionary *metadata, const char *key) {
    const AVDictionaryEntry *entry = av_dict_get(metadata, key, nullptr, 0);

    return entry ? QString::fromUtf8(entry->value) : QString();
}

QString codecName(AVCodecID codecId) {
    const AVCodecDescriptor *descriptor = avcodec_descriptor_get(codecId);

    return QString::fromLatin1(descriptor ? descriptor->name : avcodec_get_name(codecId));
}

// One exported file. It is written to a temporary file next to it, which replaces the file only on success:
// a failed or cancelled export keeps an existing file of that name.
struct Output {
    QString path;
    QString temporaryPath;
    AVStream *inputStream = nullptr;
    AVStream *outputStream = nullptr;
    AVFormatContext *context = nullptr;
    AVCodecContext *decoder = nullptr;
    AVCodecContext *encoder = nullptr;
    // Encoded SubRip packets of a converted track.
    std::vector<uint8_t> encodeBuffer;
    bool headerWritten = false;

    Output() = default;
    Output(const Output &) = delete;
    Output &operator=(const Output &) = delete;

    ~Output() {
        if (context) {
            if (context->pb) {
                avio_closep(&context->pb);
            }

            avformat_free_context(context);
        }

        avcodec_free_context(&decoder);
        avcodec_free_context(&encoder);
    }
};

bool openConverter(Output &output, QString *error) {
    const AVCodec *decoderCodec = avcodec_find_decoder(output.inputStream->codecpar->codec_id);
    const AVCodec *encoderCodec = avcodec_find_encoder(AV_CODEC_ID_SUBRIP);

    if (!decoderCodec || !encoderCodec) {
        setError(error, QStringLiteral("No codec to convert %1").arg(codecName(output.inputStream->codecpar->codec_id)));

        return false;
    }

    output.decoder = avcodec_alloc_context3(decoderCodec);
    avcodec_parameters_to_context(output.decoder, output.inputStream->codecpar);
    output.decoder->pkt_timebase = output.inputStream->time_base;
    int result = avcodec_open2(output.decoder, decoderCodec, nullptr);

    if (result < 0) {
        setError(error, errorText(result));

        return false;
    }

    output.encoder = avcodec_alloc_context3(encoderCodec);
    output.encoder->time_base = millisecondTimeBase;
    const bool hasHeader = output.decoder->subtitle_header && output.decoder->subtitle_header_size > 0;
    const auto *header = hasHeader ? output.decoder->subtitle_header : reinterpret_cast<const uint8_t *>(defaultAssHeader);
    const int headerSize = hasHeader ? output.decoder->subtitle_header_size : int(sizeof defaultAssHeader - 1);
    output.encoder->subtitle_header = static_cast<uint8_t *>(av_mallocz(headerSize + 1));
    std::memcpy(output.encoder->subtitle_header, header, headerSize);
    output.encoder->subtitle_header_size = headerSize;
    result = avcodec_open2(output.encoder, encoderCodec, nullptr);

    if (result < 0) {
        setError(error, errorText(result));

        return false;
    }

    avcodec_parameters_from_context(output.outputStream->codecpar, output.encoder);
    output.outputStream->time_base = output.encoder->time_base;
    output.encodeBuffer.resize(1 << 20);

    return true;
}

bool openOutput(Output &output, QString *error) {
    // The format follows the extension of the final name.
    int result = avformat_alloc_output_context2(&output.context, nullptr, nullptr, output.path.toUtf8().constData());

    if (result < 0 || !output.context) {
        setError(error, errorText(result));

        return false;
    }

    output.outputStream = avformat_new_stream(output.context, nullptr);
    const QString name = codecName(output.inputStream->codecpar->codec_id);

    if (canCopySubtitleCodec(name)) {
        avcodec_parameters_copy(output.outputStream->codecpar, output.inputStream->codecpar);
        output.outputStream->codecpar->codec_tag = 0;
        output.outputStream->time_base = output.inputStream->time_base;
    } else if (!openConverter(output, error)) {
        return false;
    }

    output.temporaryPath = output.path + QStringLiteral(".part");
    result = avio_open(&output.context->pb, output.temporaryPath.toUtf8().constData(), AVIO_FLAG_WRITE);

    if (result < 0) {
        setError(error, QStringLiteral("%1: %2").arg(output.path, errorText(result)));

        return false;
    }

    result = avformat_write_header(output.context, nullptr);

    if (result < 0) {
        setError(error, errorText(result));

        return false;
    }

    output.headerWritten = true;

    return true;
}

int writeCopiedPacket(Output &output, AVPacket *packet) {
    if (packet->pts == AV_NOPTS_VALUE) {
        packet->pts = packet->dts;
    }

    av_packet_rescale_ts(packet, output.inputStream->time_base, output.outputStream->time_base);
    packet->stream_index = 0;
    packet->pos = -1;

    return av_interleaved_write_frame(output.context, packet);
}

int writeConvertedPacket(Output &output, AVPacket *packet) {
    AVSubtitle subtitle{};
    int gotSubtitle = 0;

    // A broken packet is skipped, as the parsers do with broken cues.
    if (avcodec_decode_subtitle2(output.decoder, &subtitle, &gotSubtitle, packet) < 0 || !gotSubtitle) {
        return 0;
    }

    if (subtitle.pts == AV_NOPTS_VALUE) {
        const int64_t packetTime = packet->pts != AV_NOPTS_VALUE ? packet->pts : packet->dts;

        // A cue without a time cannot be placed.
        if (packetTime == AV_NOPTS_VALUE) {
            avsubtitle_free(&subtitle);

            return 0;
        }

        subtitle.pts = av_rescale_q(packetTime, output.inputStream->time_base, AV_TIME_BASE_Q);
    }

    int64_t durationMs = int64_t(subtitle.end_display_time) - int64_t(subtitle.start_display_time);

    if (durationMs <= 0 && packet->duration > 0) {
        durationMs = av_rescale_q(packet->duration, output.inputStream->time_base, millisecondTimeBase);
    }

    subtitle.pts += av_rescale_q(subtitle.start_display_time, millisecondTimeBase, AV_TIME_BASE_Q);
    subtitle.start_display_time = 0;
    subtitle.end_display_time = uint32_t(std::max<int64_t>(0, durationMs));

    std::vector<uint8_t> &buffer = output.encodeBuffer;
    const int size = avcodec_encode_subtitle(output.encoder, buffer.data(), int(buffer.size()), &subtitle);
    const int64_t pts = subtitle.pts;
    avsubtitle_free(&subtitle);

    if (size <= 0) {
        return 0;
    }

    AVPacket *encoded = av_packet_alloc();
    int result = av_new_packet(encoded, size);

    if (result >= 0) {
        std::memcpy(encoded->data, buffer.data(), size);
        encoded->pts = av_rescale_q(pts, AV_TIME_BASE_Q, output.outputStream->time_base);
        encoded->dts = encoded->pts;
        encoded->duration = av_rescale_q(durationMs, millisecondTimeBase, output.outputStream->time_base);
        encoded->stream_index = 0;
        result = av_interleaved_write_frame(output.context, encoded);
    }

    av_packet_free(&encoded);

    return result;
}

// Closes the outputs and removes their temporary files.
void removeOutputs(std::vector<std::unique_ptr<Output>> &outputs) {
    QStringList paths;

    for (const auto &output : outputs) {
        if (!output->temporaryPath.isEmpty()) {
            paths.append(output->temporaryPath);
        }
    }

    outputs.clear();

    for (const QString &path : paths) {
        QFile::remove(path);
    }
}

// Closes the complete outputs and moves their temporary files to the final names.
bool commitOutputs(std::vector<std::unique_ptr<Output>> &outputs, QString *error) {
    QList<std::pair<QString, QString>> moves;

    for (const auto &output : outputs) {
        moves.append({output->temporaryPath, output->path});
    }

    outputs.clear();
    bool ok = true;

    for (const auto &[temporaryPath, path] : moves) {
        if (ok && QFile::exists(path) && !QFile::remove(path)) {
            setError(error, QStringLiteral("%1: cannot replace the file").arg(path));
            ok = false;
        }

        if (ok && !QFile::rename(temporaryPath, path)) {
            setError(error, QStringLiteral("%1: cannot rename the temporary file").arg(path));
            ok = false;
        }

        if (!ok) {
            QFile::remove(temporaryPath);
        }
    }

    return ok;
}

} // namespace

QList<EmbeddedSubtitleTrack> probeSubtitleTracks(const QString &videoPath, QString *error) {
    QList<EmbeddedSubtitleTrack> tracks;
    const InputContext input = openInput(videoPath, nullptr, error);

    if (!input) {
        return tracks;
    }

    for (unsigned int i = 0; i < input->nb_streams; ++i) {
        const AVStream *stream = input->streams[i];

        if (stream->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            continue;
        }

        const AVCodecDescriptor *descriptor = avcodec_descriptor_get(stream->codecpar->codec_id);
        EmbeddedSubtitleTrack track;
        track.streamIndex = int(i);
        track.codecName = codecName(stream->codecpar->codec_id);
        track.language = metadataValue(stream->metadata, "language");
        track.title = metadataValue(stream->metadata, "title");
        track.forced = (stream->disposition & AV_DISPOSITION_FORCED) != 0;
        track.hearingImpaired = (stream->disposition & AV_DISPOSITION_HEARING_IMPAIRED) != 0;
        track.isText = descriptor && (descriptor->props & AV_CODEC_PROP_TEXT_SUB) != 0;
        tracks.append(track);
    }

    return tracks;
}

bool extractSubtitleTracks(const QString &videoPath, const QList<ExtractionTarget> &targets,
                           const std::function<void(double)> &progress, const std::atomic_bool &cancel,
                           QString *error) {
    const InputContext input = openInput(videoPath, &cancel, error);

    if (!input) {
        return false;
    }

    std::vector<std::unique_ptr<Output>> outputs;
    // Input stream index -> output, at most one output per stream.
    std::vector<Output *> outputByStream(input->nb_streams, nullptr);

    for (const ExtractionTarget &target : targets) {
        if (target.streamIndex < 0 || target.streamIndex >= int(input->nb_streams)
            || outputByStream[target.streamIndex]) {
            setError(error, QStringLiteral("Invalid stream %1").arg(target.streamIndex));
            removeOutputs(outputs);

            return false;
        }

        auto output = std::make_unique<Output>();
        output->path = target.outputPath;
        output->inputStream = input->streams[target.streamIndex];
        outputByStream[target.streamIndex] = output.get();
        outputs.push_back(std::move(output));

        if (!openOutput(*outputs.back(), error)) {
            removeOutputs(outputs);

            return false;
        }
    }

    // The demuxer skips packets of other streams.
    for (unsigned int i = 0; i < input->nb_streams; ++i) {
        if (!outputByStream[i]) {
            input->streams[i]->discard = AVDISCARD_ALL;
        }
    }

    const int64_t inputSize = input->pb ? avio_size(input->pb) : -1;
    QElapsedTimer progressTimer;
    progressTimer.start();
    AVPacket *packet = av_packet_alloc();
    int result = 0;

    while ((result = av_read_frame(input.get(), packet)) >= 0) {
        Output *output = packet->stream_index < int(outputByStream.size()) ? outputByStream[packet->stream_index]
                                                                           : nullptr;

        if (output) {
            const int writeResult =
                output->encoder ? writeConvertedPacket(*output, packet) : writeCopiedPacket(*output, packet);

            if (writeResult < 0) {
                result = writeResult;
                av_packet_unref(packet);

                break;
            }
        }

        av_packet_unref(packet);

        if (progress && inputSize > 0 && progressTimer.elapsed() >= 100) {
            progressTimer.restart();
            progress(double(avio_tell(input->pb)) / double(inputSize));
        }
    }

    av_packet_free(&packet);

    if (cancel.load()) {
        setError(error, QStringLiteral("Cancelled"));
        removeOutputs(outputs);

        return false;
    }

    if (result != AVERROR_EOF) {
        setError(error, errorText(result));
        removeOutputs(outputs);

        return false;
    }

    for (const auto &output : outputs) {
        result = av_write_trailer(output->context);

        if (result < 0) {
            setError(error, errorText(result));
            removeOutputs(outputs);

            return false;
        }
    }

    if (!commitOutputs(outputs, error)) {
        return false;
    }

    if (progress) {
        progress(1.0);
    }

    return true;
}

QString ffmpegVersion() {
    return QString::fromLatin1(av_version_info());
}

} // namespace afvp
