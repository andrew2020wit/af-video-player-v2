#pragma once

#include "SubtitleCue.h"

#include <QString>

namespace afvp {

enum class SubtitleFormat {
    Unknown,
    Srt,
    Vtt,
    Ass,
};

// Parsers are tolerant (SPEC 3.4): a broken cue is skipped. Cues that are empty after removing tags are dropped.
// The result is sorted by start time.
SubtitleCues parseSrt(const QString &text);
SubtitleCues parseVtt(const QString &text);
SubtitleCues parseAss(const QString &text);

// Format by file extension; for an unknown extension - by content.
SubtitleFormat detectSubtitleFormat(const QString &fileName, const QString &text);

SubtitleCues parseSubtitles(const QString &text, SubtitleFormat format);

struct SubtitleLoadResult {
    bool ok = false;
    QString error;
    SubtitleCues cues;
};

SubtitleLoadResult loadSubtitleFile(const QString &path);

} // namespace afvp
