#include "MediaFiles.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include <algorithm>

namespace afvp {

namespace {

bool hasExtension(const QString &fileName, const QStringList &extensions) {
    return extensions.contains(QFileInfo(fileName).suffix(), Qt::CaseInsensitive);
}

// Length of the run of digits (or of non-digits) starting at `position`.
qsizetype runLength(const QString &text, qsizetype position) {
    const bool digits = text.at(position).isDigit();
    qsizetype end = position;

    while (end < text.size() && text.at(end).isDigit() == digits) {
        ++end;
    }

    return end - position;
}

// Compares runs of digits by value: "9" < "10", "007" == "7".
int compareNumbers(QStringView first, QStringView second) {
    while (first.size() > 1 && first.front() == u'0') {
        first = first.mid(1);
    }

    while (second.size() > 1 && second.front() == u'0') {
        second = second.mid(1);
    }

    if (first.size() != second.size()) {
        return first.size() < second.size() ? -1 : 1;
    }

    return first.compare(second);
}

} // namespace

bool isVideoFileName(const QString &fileName) {
    static const QStringList extensions{
        QStringLiteral("mkv"),  QStringLiteral("mp4"), QStringLiteral("m4v"),  QStringLiteral("avi"),
        QStringLiteral("mov"),  QStringLiteral("webm"), QStringLiteral("wmv"), QStringLiteral("ts"),
        QStringLiteral("m2ts"), QStringLiteral("mts"), QStringLiteral("mpg"),  QStringLiteral("mpeg"),
        QStringLiteral("flv"),  QStringLiteral("ogv"), QStringLiteral("3gp"),
    };

    return hasExtension(fileName, extensions);
}

bool isSubtitleFileName(const QString &fileName) {
    static const QStringList extensions{QStringLiteral("srt"), QStringLiteral("vtt"), QStringLiteral("ass"),
                                        QStringLiteral("ssa")};

    return hasExtension(fileName, extensions);
}

int naturalCompare(const QString &first, const QString &second) {
    qsizetype i = 0;
    qsizetype j = 0;

    while (i < first.size() && j < second.size()) {
        const qsizetype firstLength = runLength(first, i);
        const qsizetype secondLength = runLength(second, j);
        const QStringView firstRun = QStringView(first).mid(i, firstLength);
        const QStringView secondRun = QStringView(second).mid(j, secondLength);
        int result = 0;

        if (first.at(i).isDigit() && second.at(j).isDigit()) {
            result = compareNumbers(firstRun, secondRun);
        } else {
            result = firstRun.compare(secondRun, Qt::CaseInsensitive);
        }

        if (result != 0) {
            return result;
        }

        i += firstLength;
        j += secondLength;
    }

    if (i < first.size() || j < second.size()) {
        return i < first.size() ? 1 : -1;
    }

    // Equal in the natural order ("e07" and "E7"): a stable order for different names.
    return first.compare(second);
}

int compareFileNames(const QString &first, const QString &second) {
    const QFileInfo firstInfo(first);
    const QFileInfo secondInfo(second);
    const int result = naturalCompare(firstInfo.completeBaseName(), secondInfo.completeBaseName());

    return result != 0 ? result : naturalCompare(firstInfo.suffix(), secondInfo.suffix());
}

QStringList videoFilesInFolder(const QString &folder) {
    const QDir dir(folder);
    QStringList names;

    for (const QString &file : dir.entryList(QDir::Files)) {
        if (isVideoFileName(file)) {
            names.append(file);
        }
    }

    std::sort(names.begin(), names.end(),
              [](const QString &first, const QString &second) { return compareFileNames(first, second) < 0; });
    QStringList paths;

    for (const QString &name : names) {
        paths.append(dir.absoluteFilePath(name));
    }

    return paths;
}

QString nextVideoFile(const QString &videoPath) {
    const QFileInfo video(videoPath);
    const QString name = video.fileName();
    const QStringList files = video.absoluteDir().entryList(QDir::Files);
    QString next;

    for (const QString &file : files) {
        if (isVideoFileName(file) && compareFileNames(file, name) > 0 &&
            (next.isEmpty() || compareFileNames(file, next) < 0)) {
            next = file;
        }
    }

    return next.isEmpty() ? QString() : video.absoluteDir().filePath(next);
}

QString existingFolder(const QString &path) {
    if (path.isEmpty()) {
        return {};
    }

    QString folder = QDir::cleanPath(QFileInfo(path).absoluteFilePath());

    while (!QFileInfo(folder).isDir()) {
        const QString parent = QFileInfo(folder).absolutePath();

        if (parent == folder) {
            return {};
        }

        folder = parent;
    }

    return folder;
}

} // namespace afvp
