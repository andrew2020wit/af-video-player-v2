#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>

#include <functional>

namespace afvp {

// A file in history.json (SPEC 3.4, I-13).
struct HistoryEntry {
    QString path;
    qint64 size = 0;
    QDateTime lastOpened;
    qint64 durationMs = 0;
    qint64 positionMs = 0;
    // mpv "aid" chosen by the user (I-02); 0 - automatic choice (I-54).
    int audioTrackId = 0;
    QString mainSubtitles;
    QString secondarySubtitles;
    // The user chose the subtitles (I-57, I-08); the automatic search (I-06) is not applied then.
    bool subtitlesChosen = false;
    qint64 mainOffsetMs = 0;
    qint64 secondaryOffsetMs = 0;

    bool operator==(const HistoryEntry &other) const = default;
};

// Files ordered by lastOpened, the most recent first.
class History {
public:
    using FileExists = std::function<bool(const QString &)>;

    static constexpr int currentVersion = 1;

    const QList<HistoryEntry> &entries() const;

    // nullptr when the file is not in the history.
    const HistoryEntry *find(const QString &path) const;
    HistoryEntry *find(const QString &path);

    // Finds the entry of an opened file (I-12), moves it to the top and updates path, size and lastOpened:
    // 1) the entry with this path - unless its size differs, then the file is another one and gets a new entry;
    // 2) the most recent entry with this file name and size whose file is missing (the file was moved), with
    //    its subtitle paths moved along; skipped without `fileExists`;
    // 3) a new entry.
    HistoryEntry &touch(const QString &path, qint64 size, const QDateTime &now, const FileExists &fileExists = {});

    void remove(const QString &path);
    void clear();
    // Keeps at most maximumSize most recent entries.
    void trim(int maximumSize);

    QJsonObject toJson() const;
    static History fromJson(const QJsonObject &object);

    // Paths are compared case-insensitively on Windows.
    static bool samePath(const QString &first, const QString &second);

private:
    QList<HistoryEntry> m_entries;
};

} // namespace afvp
