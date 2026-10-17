#include "AutoExport.h"

#include "Languages.h"

#include <QRegularExpression>
#include <QStringList>

#include <initializer_list>

namespace afvp {

namespace {

bool titleHasWord(const QString &title, std::initializer_list<QStringView> words) {
    static const QRegularExpression separators(QStringLiteral("[^\\p{L}\\p{N}]+"));
    const QStringList titleWords = title.toCaseFolded().split(separators, Qt::SkipEmptyParts);

    for (const QString &titleWord : titleWords) {
        for (QStringView word : words) {
            if (titleWord == word) {
                return true;
            }
        }
    }

    return false;
}

// The first fitting track in the language: an ordinary one, otherwise an SDH one; -1 when there is none.
int findTrack(const QList<EmbeddedSubtitleTrack> &tracks, const QString &language) {
    if (language.isEmpty()) {
        return -1;
    }

    int sdhStreamIndex = -1;

    for (const EmbeddedSubtitleTrack &track : tracks) {
        if (!track.isText || track.language.isEmpty() || isForcedTrack(track) ||
            !sameLanguage(track.language, language)) {
            continue;
        }

        if (!isSdhTrack(track)) {
            return track.streamIndex;
        }

        if (sdhStreamIndex < 0) {
            sdhStreamIndex = track.streamIndex;
        }
    }

    return sdhStreamIndex;
}

} // namespace

bool isForcedTrack(const EmbeddedSubtitleTrack &track) {
    return track.forced || titleHasWord(track.title, {u"forced"});
}

bool isSdhTrack(const EmbeddedSubtitleTrack &track) {
    return track.hearingImpaired || titleHasWord(track.title, {u"sdh", u"cc", u"hi"});
}

AutoExportChoice chooseAutoExportTracks(const QList<EmbeddedSubtitleTrack> &tracks, const QString &studiedLanguage,
                                        const QString &nativeLanguage) {
    const int studied = findTrack(tracks, studiedLanguage);
    const int native = sameLanguage(studiedLanguage, nativeLanguage) ? -1 : findTrack(tracks, nativeLanguage);

    if (studied < 0) {
        return {native, -1};
    }

    return {studied, native};
}

} // namespace afvp
