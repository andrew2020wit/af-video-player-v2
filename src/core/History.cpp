#include "History.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QSet>

#include <algorithm>

namespace afvp {

namespace {

QJsonValue stringOrNull(const QString &value) {
    return value.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(value);
}

qint64 toInt64(const QJsonValue &value) {
    return value.isDouble() ? qint64(value.toDouble()) : 0;
}

QJsonObject entryToJson(const HistoryEntry &entry) {
    return {
        {QStringLiteral("path"), entry.path},
        {QStringLiteral("size"), entry.size},
        {QStringLiteral("lastOpened"), entry.lastOpened.toUTC().toString(Qt::ISODate)},
        {QStringLiteral("durationMs"), entry.durationMs},
        {QStringLiteral("positionMs"), entry.positionMs},
        {QStringLiteral("audioTrackId"),
         entry.audioTrackId > 0 ? QJsonValue(entry.audioTrackId) : QJsonValue(QJsonValue::Null)},
        {QStringLiteral("mainSubtitles"), stringOrNull(entry.mainSubtitles)},
        {QStringLiteral("secondarySubtitles"), stringOrNull(entry.secondarySubtitles)},
        {QStringLiteral("subtitlesChosen"), entry.subtitlesChosen},
        {QStringLiteral("mainOffsetMs"), entry.mainOffsetMs},
        {QStringLiteral("secondaryOffsetMs"), entry.secondaryOffsetMs},
    };
}

HistoryEntry entryFromJson(const QJsonObject &object) {
    HistoryEntry entry;
    entry.path = object.value(QStringLiteral("path")).toString();
    entry.size = toInt64(object.value(QStringLiteral("size")));
    entry.lastOpened = QDateTime::fromString(object.value(QStringLiteral("lastOpened")).toString(), Qt::ISODate);
    entry.durationMs = std::max<qint64>(0, toInt64(object.value(QStringLiteral("durationMs"))));
    entry.positionMs = std::max<qint64>(0, toInt64(object.value(QStringLiteral("positionMs"))));
    entry.audioTrackId = std::max(0, object.value(QStringLiteral("audioTrackId")).toInt(0));
    entry.mainSubtitles = object.value(QStringLiteral("mainSubtitles")).toString();
    entry.secondarySubtitles = object.value(QStringLiteral("secondarySubtitles")).toString();
    entry.subtitlesChosen = object.value(QStringLiteral("subtitlesChosen")).toBool(false);
    entry.mainOffsetMs = toInt64(object.value(QStringLiteral("mainOffsetMs")));
    entry.secondaryOffsetMs = toInt64(object.value(QStringLiteral("secondaryOffsetMs")));
    return entry;
}

// Paths are compared case-insensitively on Windows.
Qt::CaseSensitivity pathCaseSensitivity() {
#ifdef Q_OS_WIN
    return Qt::CaseInsensitive;
#else
    return Qt::CaseSensitive;
#endif
}

// Equal for paths that samePath treats as equal.
QString pathKey(const QString &path) {
    const QString cleanPath = QDir::cleanPath(path);

    return pathCaseSensitivity() == Qt::CaseInsensitive ? cleanPath.toCaseFolded() : cleanPath;
}

// "D:/Films/movie.mkv" -> "D:/Films". History paths are absolute, with "/" separators.
QString folderOf(const QString &path) {
    const QString cleanPath = QDir::cleanPath(path);

    return cleanPath.left(cleanPath.lastIndexOf(u'/'));
}

// Subtitles of a moved video (I-12): a file from the old video folder or its subfolders is looked for at the
// same relative path in the new folder, a file outside it stays. Empty when the file is not found.
QString movedSubtitlePath(const QString &subtitlePath, const QString &oldFolder, const QString &newFolder,
                          const History::FileExists &fileExists) {
    const QString cleanPath = QDir::cleanPath(subtitlePath);
    const QString oldPrefix = oldFolder + u'/';
    const QString candidate = cleanPath.startsWith(oldPrefix, pathCaseSensitivity())
                                  ? newFolder + u'/' + cleanPath.mid(oldPrefix.size())
                                  : cleanPath;

    return fileExists(candidate) ? candidate : QString();
}

// When a subtitle file is not found, the choice is reset and the automatic search (I-06) applies.
void moveSubtitles(HistoryEntry &entry, const QString &newFolder, const History::FileExists &fileExists) {
    const QString oldFolder = folderOf(entry.path);
    bool lost = false;

    for (QString *subtitles : {&entry.mainSubtitles, &entry.secondarySubtitles}) {
        if (!subtitles->isEmpty()) {
            *subtitles = movedSubtitlePath(*subtitles, oldFolder, newFolder, fileExists);
            lost = lost || subtitles->isEmpty();
        }
    }

    if (lost) {
        entry.mainSubtitles.clear();
        entry.secondarySubtitles.clear();
        entry.subtitlesChosen = false;
    }
}

} // namespace

const QList<HistoryEntry> &History::entries() const {
    return m_entries;
}

const HistoryEntry *History::find(const QString &path) const {
    for (const HistoryEntry &entry : m_entries) {
        if (samePath(entry.path, path)) {
            return &entry;
        }
    }

    return nullptr;
}

HistoryEntry *History::find(const QString &path) {
    return const_cast<HistoryEntry *>(std::as_const(*this).find(path));
}

HistoryEntry &History::touch(const QString &path, qint64 size, const QDateTime &now, const FileExists &fileExists) {
    HistoryEntry entry;
    const auto existing = std::find_if(m_entries.begin(), m_entries.end(),
                                       [&path](const HistoryEntry &item) { return samePath(item.path, path); });

    if (existing != m_entries.end()) {
        // Another file of another size at the same path starts from scratch.
        if (existing->size == size) {
            entry = *existing;
        }

        m_entries.erase(existing);
    } else if (fileExists) {
        const QString name = QFileInfo(path).fileName();
        // Entries are ordered by lastOpened: the first match is the most recent one.
        const auto moved = std::find_if(m_entries.begin(), m_entries.end(), [&](const HistoryEntry &item) {
            return item.size == size && QFileInfo(item.path).fileName().compare(name, pathCaseSensitivity()) == 0 &&
                   !fileExists(item.path);
        });

        if (moved != m_entries.end()) {
            entry = *moved;
            m_entries.erase(moved);
            moveSubtitles(entry, folderOf(path), fileExists);
        }
    }

    entry.path = path;
    entry.size = size;
    entry.lastOpened = now;
    m_entries.prepend(entry);

    return m_entries.front();
}

void History::remove(const QString &path) {
    m_entries.removeIf([&path](const HistoryEntry &entry) { return samePath(entry.path, path); });
}

void History::clear() {
    m_entries.clear();
}

void History::trim(int maximumSize) {
    if (m_entries.size() > maximumSize) {
        m_entries.resize(std::max(0, maximumSize));
    }
}

QJsonObject History::toJson() const {
    QJsonArray files;

    for (const HistoryEntry &entry : m_entries) {
        files.append(entryToJson(entry));
    }

    return {{QStringLiteral("version"), currentVersion}, {QStringLiteral("files"), files}};
}

History History::fromJson(const QJsonObject &object) {
    History history;
    const QJsonArray files = object.value(QStringLiteral("files")).toArray();
    // Paths already read, as compared by samePath: a set keeps the reading of a long history linear.
    QSet<QString> paths;

    for (const QJsonValue &file : files) {
        HistoryEntry entry = entryFromJson(file.toObject());
        const QString key = pathKey(entry.path);

        if (!entry.path.isEmpty() && !paths.contains(key)) {
            paths.insert(key);
            history.m_entries.append(entry);
        }
    }

    std::stable_sort(history.m_entries.begin(), history.m_entries.end(),
                     [](const HistoryEntry &left, const HistoryEntry &right) {
                         return left.lastOpened > right.lastOpened;
                     });

    return history;
}

bool History::samePath(const QString &first, const QString &second) {
    return QDir::cleanPath(first).compare(QDir::cleanPath(second), pathCaseSensitivity()) == 0;
}

} // namespace afvp
