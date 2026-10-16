#pragma once

#include "MediaFiles.h"

#include <QList>
#include <QString>

namespace afvp {

// Language and flags read from a subtitle file name (I-06).
struct SubtitleNameInfo {
    QString language; // ISO 639-1 code for a known language, empty when not found
    bool forced = false;
    bool sdh = false;

    bool operator==(const SubtitleNameInfo &other) const = default;
};

// When the file name starts with the video name, only the parts after it are looked at.
SubtitleNameInfo parseSubtitleFileName(const QString &fileName, const QString &videoBaseName);

// Where a subtitle file was found, in the order of preference.
enum class SubtitleLocation {
    VideoFolder,       // "movie.en.srt" next to the video
    SubsFolder,        // "Subs/movie.en.srt" or "Subtitles/…"
    VideoSubsFolder,   // "Subs/<video name>/2_English.srt"
};

struct FoundSubtitle {
    QString path;         // absolute, with "/" separators
    QString relativePath; // relative to the video folder, e.g. "Subs/s02e05/2_English.srt"
    SubtitleLocation location = SubtitleLocation::VideoFolder;
    SubtitleNameInfo info;

    bool operator==(const FoundSubtitle &other) const = default;
};

// Subtitle files for a video in its folder and in Subs/ (I-06), ordered by location, then by relative path.
QList<FoundSubtitle> findSubtitles(const QString &videoPath);

struct SubtitleChoice {
    QString main;
    QString secondary;

    bool operator==(const SubtitleChoice &other) const = default;
};

// Main subtitles in the studied language, secondary in the native one (I-06, I-55). Among files of one language
// the preferred one is without flags, then sdh, then forced; then by location; then by relative path.
SubtitleChoice chooseSubtitles(const QList<FoundSubtitle> &files, const QString &studiedLanguage,
                               const QString &nativeLanguage);

} // namespace afvp
