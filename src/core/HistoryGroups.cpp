#include "HistoryGroups.h"

#include "MediaFiles.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>

namespace afvp {

namespace {

constexpr double watchedShare = 0.95;

const HistoryEntry *findEntry(const QList<HistoryEntry> &entries, const QString &path) {
    const auto found = std::find_if(entries.cbegin(), entries.cend(),
                                    [&path](const HistoryEntry &entry) { return History::samePath(entry.path, path); });

    return found != entries.cend() ? &*found : nullptr;
}

QString findContinuePath(const HistoryGroup &group) {
    if (group.entries.isEmpty()) {
        return QString();
    }

    const HistoryEntry &latest = group.entries.front();

    if (!isWatched(latest)) {
        return latest.path;
    }

    const auto current = std::find_if(group.videoFiles.cbegin(), group.videoFiles.cend(),
                                      [&latest](const QString &file) { return History::samePath(file, latest.path); });

    if (current == group.videoFiles.cend()) {
        return QString();
    }

    for (auto file = current + 1; file != group.videoFiles.cend(); ++file) {
        const HistoryEntry *entry = findEntry(group.entries, *file);

        if (!entry || !isWatched(*entry)) {
            return *file;
        }
    }

    return QString();
}

} // namespace

bool isWatched(const HistoryEntry &entry) {
    return entry.durationMs > 0 && double(entry.positionMs) >= watchedShare * double(entry.durationMs);
}

QList<HistoryGroup> groupHistory(const QList<HistoryEntry> &entries,
                                 const std::function<bool(const QString &)> &fileExists,
                                 const std::function<QStringList(const QString &)> &videoFilesIn) {
    QList<HistoryGroup> groups;

    for (const HistoryEntry &entry : entries) {
        if (!fileExists(entry.path)) {
            continue;
        }

        const QString folder = QFileInfo(entry.path).absolutePath();
        auto group = std::find_if(groups.begin(), groups.end(), [&folder](const HistoryGroup &existing) {
            return History::samePath(existing.folder, folder);
        });

        if (group == groups.end()) {
            groups.append({folder, {}, {}, 0, {}});
            group = groups.end() - 1;
        }

        group->entries.append(entry);
    }

    for (HistoryGroup &group : groups) {
        group.videoFiles = videoFilesIn(group.folder);
        std::sort(group.videoFiles.begin(), group.videoFiles.end(),
                  [](const QString &first, const QString &second) { return compareFileNames(first, second) < 0; });
        group.watchedCount = int(std::count_if(group.videoFiles.cbegin(), group.videoFiles.cend(),
                                               [&group](const QString &file) {
                                                   const HistoryEntry *entry = findEntry(group.entries, file);

                                                   return entry && isWatched(*entry);
                                               }));
        group.continuePath = findContinuePath(group);
    }

    return groups;
}

QStringList videoFilesInFolder(const QString &folder) {
    const QDir dir(folder);
    QStringList files;

    for (const QString &name : dir.entryList(QDir::Files)) {
        if (isVideoFileName(name)) {
            files.append(dir.filePath(name));
        }
    }

    return files;
}

} // namespace afvp
