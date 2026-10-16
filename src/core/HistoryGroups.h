#pragma once

#include "History.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <functional>

namespace afvp {

// A file played to at least 95% is watched (I-76).
bool isWatched(const HistoryEntry &entry);

// History entries of one folder in the start window (I-76).
struct HistoryGroup {
    QString folder;
    // Entries of existing files, the most recent first.
    QList<HistoryEntry> entries;
    // All video files of the folder in the natural order (I-16). A folder with two or more is a series.
    QStringList videoFiles;
    // Video files of the folder with a watched entry.
    int watchedCount = 0;
    // The file to continue the series with: the most recent entry when it is not watched, otherwise the first
    // video after it that is not watched. Empty when there is none.
    QString continuePath;

    bool isSeries() const {
        return videoFiles.size() >= 2;
    }
};

// Groups the entries of existing files by folder; groups are ordered by their most recent entry.
// videoFilesIn lists the video files of a folder (absolute paths, any order).
QList<HistoryGroup> groupHistory(const QList<HistoryEntry> &entries,
                                 const std::function<bool(const QString &)> &fileExists,
                                 const std::function<QStringList(const QString &)> &videoFilesIn);

// Video files of a folder on disk, as absolute paths.
QStringList videoFilesInFolder(const QString &folder);

} // namespace afvp
