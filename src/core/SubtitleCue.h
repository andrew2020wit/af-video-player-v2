#pragma once

#include <QList>
#include <QString>

namespace afvp {

// One subtitle cue. Times are in milliseconds; the interval is [startMs, endMs).
struct SubtitleCue {
    qint64 startMs = 0;
    qint64 endMs = 0;
    QString text;

    bool operator==(const SubtitleCue &other) const = default;
};

using SubtitleCues = QList<SubtitleCue>;

} // namespace afvp
