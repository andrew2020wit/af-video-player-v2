#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <functional>

namespace afvp {

// An embedded subtitle track of a video file (I-08).
struct EmbeddedSubtitleTrack {
    int streamIndex = -1;
    QString codecName; // FFmpeg codec name: "subrip", "ass", "webvtt", "mov_text", ...
    QString language;
    QString title;
    bool forced = false;
    bool hearingImpaired = false;
    bool isText = true; // bitmap subtitles are ignored (P-11)
};

// Extension of the exported file: the source format is kept (subrip -> srt, ass -> ass, ssa -> ssa,
// webvtt -> vtt); other text formats (mov_text, ...) are converted to SRT.
QString exportExtension(const QString &codecName);

// true when packets can be copied as they are, false when the track is converted to SRT.
bool canCopySubtitleCodec(const QString &codecName);

enum class ConflictPolicy {
    Replace,
    KeepBoth,
};

// File names for the exported tracks: "<video name>.<language>[.forced][.sdh].<ext>" (I-08).
// Equal names within the batch get numbers: movie.en.srt, movie.en.2.srt, ...
// With KeepBoth, names taken in the folder (fileExists) also get the next free number.
QStringList exportFileNames(const QString &videoBaseName, const QList<EmbeddedSubtitleTrack> &tracks,
                            ConflictPolicy policy, const std::function<bool(const QString &)> &fileExists);

} // namespace afvp
