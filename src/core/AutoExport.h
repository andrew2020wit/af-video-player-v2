#pragma once

#include "ExportNaming.h"

#include <QList>
#include <QString>

#include <functional>

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

// A track of the batch export (I-87) and the name of its file in the video folder.
struct BatchExportItem {
    EmbeddedSubtitleTrack track;
    QString fileName;
};

// Tracks of one video for the batch export (I-87): all text tracks except forced ones, with the file names of I-08.
// A track whose file already exists (fileExists) was exported earlier and is skipped.
QList<BatchExportItem> chooseBatchExportTracks(const QString &videoBaseName,
                                               const QList<EmbeddedSubtitleTrack> &tracks,
                                               const std::function<bool(const QString &)> &fileExists);

} // namespace afvp
