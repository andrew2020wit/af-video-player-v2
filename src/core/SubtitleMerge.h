#pragma once

#include "SubtitleCue.h"

namespace afvp {

// A row of the subtitle list: a main cue with the secondary cues attached to it (F-44, I-20).
struct MergedCue {
    qint64 startMs = 0;
    qint64 endMs = 0;
    QString text;
    QString secondaryText;

    bool operator==(const MergedCue &other) const = default;
};

using MergedCues = QList<MergedCue>;

// Applies a subtitle offset stored in the history (SPEC 3.4) to all cues.
SubtitleCues shiftCues(const SubtitleCues &cues, qint64 offsetMs);

// Merges secondary cues into main cues (I-20). Both lists must be sorted by start time.
// Every secondary cue goes to exactly one main cue: the one with the largest overlap (the earlier one on a tie);
// without any overlap - to the nearest main cue by time (the earlier one on a tie).
// Texts are normalized for the list (normalizeCueText); several secondary cues are joined with '\n'.
MergedCues mergeSubtitles(const SubtitleCues &mainCues, const SubtitleCues &secondaryCues);

// Text shown over the video at a time (I-22): the cues with startMs <= timeMs < endMs in start order, joined
// with '\n'. The lines of a cue are joined as in the list (normalizeCueText): the width of the window wraps them.
// Empty between cues. The cues must be sorted by start time.
QString activeCueText(const SubtitleCues &cues, qint64 timeMs);

// Index of the current cue: the last one with startMs <= timeMs (F-45), or -1.
template <typename Cue>
qsizetype findCurrentCueIndex(const QList<Cue> &cues, qint64 timeMs) {
    qsizetype low = 0;
    qsizetype high = cues.size();

    while (low < high) {
        const qsizetype middle = (low + high) / 2;

        if (cues[middle].startMs <= timeMs) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }

    return low - 1;
}

// Index of the first cue with startMs >= timeMs, or cues.size() when there is none: the row a chapter heading is
// shown above (I-89).
template <typename Cue>
qsizetype findCueIndexFrom(const QList<Cue> &cues, qint64 timeMs) {
    return findCurrentCueIndex(cues, timeMs - 1) + 1;
}

} // namespace afvp
