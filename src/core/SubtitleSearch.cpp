#include "SubtitleSearch.h"

#include "Languages.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>
#include <tuple>

namespace afvp {

namespace {

// "movie.en", "movie_en", "movie" start with the video name "movie"; "movie2" does not.
bool startsWithVideoName(const QString &baseName, const QString &videoBaseName) {
    if (videoBaseName.isEmpty() || !baseName.startsWith(videoBaseName, Qt::CaseInsensitive)) {
        return false;
    }

    if (baseName.size() == videoBaseName.size()) {
        return true;
    }

    const QChar next = baseName.at(videoBaseName.size());

    return next == u'.' || next == u'_' || next == u'-' || next == u' ';
}

bool isSubsFolderName(const QString &name) {
    return name.compare(QStringLiteral("subs"), Qt::CaseInsensitive) == 0 ||
           name.compare(QStringLiteral("subtitles"), Qt::CaseInsensitive) == 0;
}

int flagsRank(const SubtitleNameInfo &info) {
    if (info.forced) {
        return 2;
    }

    return info.sdh ? 1 : 0;
}

bool isPreferred(const FoundSubtitle &first, const FoundSubtitle &second) {
    const auto key = [](const FoundSubtitle &file) {
        return std::make_tuple(flagsRank(file.info), int(file.location));
    };

    if (key(first) != key(second)) {
        return key(first) < key(second);
    }

    return first.relativePath.compare(second.relativePath, Qt::CaseInsensitive) < 0;
}

template <typename Predicate>
const FoundSubtitle *bestFile(const QList<FoundSubtitle> &files, Predicate predicate) {
    const FoundSubtitle *best = nullptr;

    for (const FoundSubtitle &file : files) {
        if (predicate(file) && (!best || isPreferred(file, *best))) {
            best = &file;
        }
    }

    return best;
}

QFileInfoList folderEntries(const QString &folder) {
    return QDir(folder).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
}

} // namespace

SubtitleNameInfo parseSubtitleFileName(const QString &fileName, const QString &videoBaseName) {
    static const QRegularExpression separators(QStringLiteral("[._\\- ]+"));
    QString name = QFileInfo(fileName).completeBaseName();

    if (startsWithVideoName(name, videoBaseName)) {
        name = name.mid(videoBaseName.size());
    }

    SubtitleNameInfo info;
    const QStringList parts = name.split(separators, Qt::SkipEmptyParts);

    for (const QString &part : parts) {
        const QString lower = part.toLower();

        if (lower == u"forced") {
            info.forced = true;
        } else if (lower == u"sdh" || lower == u"hi" || lower == u"cc") {
            // "hi" is always "hearing impaired", not Hindi (I-06).
            info.sdh = true;
        } else if (info.language.isEmpty()) {
            if (const Language *language = findLanguage(part)) {
                info.language = language->code;
            }
        }
    }

    return info;
}

QList<FoundSubtitle> findSubtitles(const QString &videoPath) {
    const QFileInfo video(videoPath);
    const QString folder = video.absolutePath();
    const QString videoBaseName = video.completeBaseName();
    const QDir videoFolder(folder);
    const QFileInfoList entries = folderEntries(folder);
    QList<FoundSubtitle> found;

    const auto add = [&](const QFileInfo &file, SubtitleLocation location) {
        found.append({QDir::fromNativeSeparators(file.absoluteFilePath()),
                      videoFolder.relativeFilePath(file.absoluteFilePath()), location,
                      parseSubtitleFileName(file.fileName(), videoBaseName)});
    };

    const bool singleVideo = std::count_if(entries.cbegin(), entries.cend(), [](const QFileInfo &entry) {
                                 return entry.isFile() && isVideoFileName(entry.fileName());
                             }) == 1;

    for (const QFileInfo &entry : entries) {
        if (entry.isFile() && isSubtitleFileName(entry.fileName()) &&
            startsWithVideoName(entry.completeBaseName(), videoBaseName)) {
            add(entry, SubtitleLocation::VideoFolder);
        }
    }

    for (const QFileInfo &entry : entries) {
        if (!entry.isDir() || !isSubsFolderName(entry.fileName())) {
            continue;
        }

        for (const QFileInfo &subsEntry : folderEntries(entry.absoluteFilePath())) {
            if (subsEntry.isFile() && isSubtitleFileName(subsEntry.fileName()) &&
                (singleVideo || startsWithVideoName(subsEntry.completeBaseName(), videoBaseName))) {
                add(subsEntry, SubtitleLocation::SubsFolder);
            } else if (subsEntry.isDir() && subsEntry.fileName().compare(videoBaseName, Qt::CaseInsensitive) == 0) {
                for (const QFileInfo &file : folderEntries(subsEntry.absoluteFilePath())) {
                    if (file.isFile() && isSubtitleFileName(file.fileName())) {
                        add(file, SubtitleLocation::VideoSubsFolder);
                    }
                }
            }
        }
    }

    std::stable_sort(found.begin(), found.end(), [](const FoundSubtitle &first, const FoundSubtitle &second) {
        if (first.location != second.location) {
            return first.location < second.location;
        }

        return first.relativePath.compare(second.relativePath, Qt::CaseInsensitive) < 0;
    });

    return found;
}

SubtitleChoice chooseSubtitles(const QList<FoundSubtitle> &files, const QString &studiedLanguage,
                               const QString &nativeLanguage) {
    const FoundSubtitle *main =
        bestFile(files, [&](const FoundSubtitle &file) { return sameLanguage(file.info.language, studiedLanguage); });

    if (!main && files.size() == 1 && files.front().info.language.isEmpty()) {
        main = &files.front();
    }

    const FoundSubtitle *secondary = nullptr;

    if (!sameLanguage(studiedLanguage, nativeLanguage)) {
        secondary = bestFile(files, [&](const FoundSubtitle &file) {
            return &file != main && sameLanguage(file.info.language, nativeLanguage);
        });
    }

    // Without a file in the studied language the translation becomes the main one: an empty list would hide it.
    if (!main) {
        std::swap(main, secondary);
    }

    return {main ? main->path : QString(), secondary ? secondary->path : QString()};
}

} // namespace afvp
