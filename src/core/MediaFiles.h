#pragma once

#include <QString>

namespace afvp {

// Video and subtitle files are recognised by the file name extension, case-insensitively.
bool isVideoFileName(const QString &fileName);
bool isSubtitleFileName(const QString &fileName);

// "Natural" order of file names (I-16): case-insensitive, numbers compared as numbers ("e9" < "e10").
// Returns a negative value, zero or a positive value, like QString::compare.
int naturalCompare(const QString &first, const QString &second);

// File names in the natural order of names without extensions, then of extensions: "Movie.mkv" < "Movie 2.mkv".
int compareFileNames(const QString &first, const QString &second);

// The next video file after `videoPath` in its folder in the natural order (I-16); empty for the last one.
QString nextVideoFile(const QString &videoPath);

// `path` itself if it is an existing folder, otherwise its nearest existing parent folder up to the root of the
// drive (I-86); empty when none exists (e.g. the drive is removed) or `path` is empty.
QString existingFolder(const QString &path);

} // namespace afvp
