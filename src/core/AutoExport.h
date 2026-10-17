#pragma once

#include "ExportNaming.h"

#include <QList>
#include <QString>

namespace afvp {

// A forced track holds only part of the text: by the container flag or the word "forced" in the title (I-85).
bool isForcedTrack(const EmbeddedSubtitleTrack &track);

// An SDH track: by the "hearing impaired" flag or the words "SDH", "CC", "HI" in the title (I-85).
bool isSdhTrack(const EmbeddedSubtitleTrack &track);

// Tracks of the automatic export of a new video without subtitle files (I-85); -1 when there is none.
struct AutoExportChoice {
    int mainStreamIndex = -1;
    int secondaryStreamIndex = -1;

    bool isEmpty() const { return mainStreamIndex < 0 && secondaryStreamIndex < 0; }
    bool operator==(const AutoExportChoice &other) const = default;
};

// Main - the first text track in the studied language, secondary - in the native one. Forced tracks and tracks
// without a language are skipped, an SDH track is taken only when its language has no other one. Without a track in
// the studied language the native one becomes main; equal languages give no secondary track.
AutoExportChoice chooseAutoExportTracks(const QList<EmbeddedSubtitleTrack> &tracks, const QString &studiedLanguage,
                                        const QString &nativeLanguage);

} // namespace afvp
