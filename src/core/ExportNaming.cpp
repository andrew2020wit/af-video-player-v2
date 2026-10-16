#include "ExportNaming.h"

#include "History.h"
#include "Languages.h"

namespace afvp {

namespace {

QString languagePart(const QString &language) {
    const QString code = normalizeLanguageCode(language);

    return code.isEmpty() ? QStringLiteral("und") : code;
}

QString numberedName(const QString &stem, int number, const QString &extension) {
    if (number <= 1) {
        return QStringLiteral("%1.%2").arg(stem, extension);
    }

    return QStringLiteral("%1.%2.%3").arg(stem).arg(number).arg(extension);
}

bool containsPath(const QStringList &names, const QString &name) {
    for (const QString &item : names) {
        if (History::samePath(item, name)) {
            return true;
        }
    }

    return false;
}

} // namespace

QString exportExtension(const QString &codecName) {
    if (codecName == u"subrip" || codecName == u"srt") {
        return QStringLiteral("srt");
    }

    if (codecName == u"ass") {
        return QStringLiteral("ass");
    }

    if (codecName == u"ssa") {
        return QStringLiteral("ssa");
    }

    if (codecName == u"webvtt") {
        return QStringLiteral("vtt");
    }

    return QStringLiteral("srt");
}

bool canCopySubtitleCodec(const QString &codecName) {
    return codecName == u"subrip" || codecName == u"srt" || codecName == u"ass" || codecName == u"ssa"
        || codecName == u"webvtt";
}

QStringList exportFileNames(const QString &videoBaseName, const QList<EmbeddedSubtitleTrack> &tracks,
                            ConflictPolicy policy, const std::function<bool(const QString &)> &fileExists) {
    QStringList names;

    for (const EmbeddedSubtitleTrack &track : tracks) {
        QString stem = videoBaseName + u'.' + languagePart(track.language);

        if (track.forced) {
            stem += QStringLiteral(".forced");
        }

        if (track.hearingImpaired) {
            stem += QStringLiteral(".sdh");
        }

        const QString extension = exportExtension(track.codecName);
        int number = 1;
        QString name = numberedName(stem, number, extension);

        while (containsPath(names, name) || (policy == ConflictPolicy::KeepBoth && fileExists(name))) {
            ++number;
            name = numberedName(stem, number, extension);
        }

        names.append(name);
    }

    return names;
}

} // namespace afvp
