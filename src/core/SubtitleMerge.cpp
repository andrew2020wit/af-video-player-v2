#include "SubtitleMerge.h"

#include "CueText.h"

#include <QStringList>

#include <algorithm>
#include <limits>

namespace afvp {

namespace {

qint64 overlap(const SubtitleCue &first, const SubtitleCue &second) {
    return std::min(first.endMs, second.endMs) - std::max(first.startMs, second.startMs);
}

// Index of the main cue a secondary cue belongs to, or -1 when there are no main cues.
qsizetype findOwner(const SubtitleCues &mainCues, qint64 maximumDurationMs, const SubtitleCue &secondary) {
    // Main cues that start before the secondary cue ends: [0, firstAfter).
    const auto firstAfterIterator = std::lower_bound(
        mainCues.cbegin(), mainCues.cend(), secondary.endMs,
        [](const SubtitleCue &cue, qint64 timeMs) { return cue.startMs < timeMs; });
    const qsizetype firstAfter = firstAfterIterator - mainCues.cbegin();

    qsizetype bestIndex = -1;
    qint64 bestOverlap = 0;
    qsizetype previousIndex = -1;
    qint64 previousEnd = std::numeric_limits<qint64>::min();

    // A cue ends no later than its start + the longest main cue duration. Scanning stops when earlier cues
    // can neither overlap the secondary cue nor end later than the best "previous" cue found so far.
    for (qsizetype i = firstAfter - 1; i >= 0; --i) {
        const SubtitleCue &cue = mainCues[i];
        const qint64 latestPossibleEnd = cue.startMs + maximumDurationMs;

        if (latestPossibleEnd <= secondary.startMs && latestPossibleEnd < previousEnd) {
            break;
        }

        const qint64 cueOverlap = overlap(cue, secondary);

        // Iterating backwards: ">=" keeps the earlier cue on a tie.
        if (cueOverlap > 0 && cueOverlap >= bestOverlap) {
            bestOverlap = cueOverlap;
            bestIndex = i;
        }

        if (cue.endMs >= previousEnd) {
            previousEnd = cue.endMs;
            previousIndex = i;
        }
    }

    if (bestIndex >= 0) {
        return bestIndex;
    }

    // No overlap: the nearest cue before or after.
    const qint64 previousDistance =
        previousIndex >= 0 ? secondary.startMs - previousEnd : std::numeric_limits<qint64>::max();
    const qint64 nextDistance = firstAfter < mainCues.size() ? mainCues[firstAfter].startMs - secondary.endMs
                                                             : std::numeric_limits<qint64>::max();

    if (previousIndex < 0 && firstAfter >= mainCues.size()) {
        return -1;
    }

    return previousDistance <= nextDistance ? previousIndex : firstAfter;
}

} // namespace

SubtitleCues shiftCues(const SubtitleCues &cues, qint64 offsetMs) {
    if (offsetMs == 0) {
        return cues;
    }

    SubtitleCues result = cues;

    for (SubtitleCue &cue : result) {
        cue.startMs += offsetMs;
        cue.endMs += offsetMs;
    }

    return result;
}

MergedCues mergeSubtitles(const SubtitleCues &mainCues, const SubtitleCues &secondaryCues) {
    MergedCues result;
    result.reserve(mainCues.size());

    for (const SubtitleCue &cue : mainCues) {
        result.append({cue.startMs, cue.endMs, normalizeCueText(cue.text), QString()});
    }

    qint64 maximumDurationMs = 0;

    for (const SubtitleCue &cue : mainCues) {
        maximumDurationMs = std::max(maximumDurationMs, cue.endMs - cue.startMs);
    }

    for (const SubtitleCue &secondary : secondaryCues) {
        const qsizetype owner = findOwner(mainCues, maximumDurationMs, secondary);

        if (owner < 0) {
            continue;
        }

        const QString text = normalizeCueText(secondary.text);

        if (text.isEmpty()) {
            continue;
        }

        QString &secondaryText = result[owner].secondaryText;

        if (!secondaryText.isEmpty()) {
            secondaryText.append(u'\n');
        }

        secondaryText.append(text);
    }

    return result;
}

QString activeCueText(const SubtitleCues &cues, qint64 timeMs) {
    QStringList texts;

    // Earlier cues may still be shown when they overlap later ones, so all cues that started are checked.
    for (qsizetype i = findCurrentCueIndex(cues, timeMs); i >= 0; --i) {
        if (cues[i].endMs > timeMs) {
            const QString text = normalizeCueText(cues[i].text);

            if (!text.isEmpty()) {
                texts.prepend(text);
            }
        }
    }

    return texts.join(u'\n');
}

} // namespace afvp
