#pragma once

#include "ExportNaming.h"

#include <QList>
#include <QString>

#include <atomic>
#include <functional>

namespace afvp {

// Embedded subtitle tracks of a video file, read from the container headers. Bitmap tracks are included
// with isText == false so that the caller can skip them (P-11).
QList<EmbeddedSubtitleTrack> probeSubtitleTracks(const QString &videoPath, QString *error = nullptr);

struct ExtractionTarget {
    int streamIndex = -1;
    QString outputPath;
};

// Extracts several tracks in one pass over the file (SPEC 3.2): packets are copied as they are, formats
// without a file format of their own (mov_text, ...) are converted to SRT.
// progress receives 0..1 by the read position; cancel stops the extraction. The files are written under
// temporary names and replace existing files only on success: a failure or cancellation leaves the folder as it was.
bool extractSubtitleTracks(const QString &videoPath, const QList<ExtractionTarget> &targets,
                           const std::function<void(double)> &progress, const std::atomic_bool &cancel,
                           QString *error = nullptr);

// FFmpeg version for "About".
QString ffmpegVersion();

} // namespace afvp
