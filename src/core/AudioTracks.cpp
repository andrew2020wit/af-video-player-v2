#include "AudioTracks.h"

#include "Languages.h"

#include <QStringList>

namespace afvp {

namespace {

QString describeChannels(int channelCount) {
    switch (channelCount) {
    case 0:
        return QString();
    case 1:
        return QStringLiteral("mono");
    case 2:
        return QStringLiteral("2.0");
    case 6:
        return QStringLiteral("5.1");
    case 8:
        return QStringLiteral("7.1");
    default:
        return QStringLiteral("%1 ch").arg(channelCount);
    }
}

} // namespace

int chooseAudioTrack(const QList<AudioTrack> &tracks, int userTrackId, const QString &preferredLanguage) {
    if (tracks.isEmpty()) {
        return 0;
    }

    for (const AudioTrack &track : tracks) {
        if (userTrackId > 0 && track.id == userTrackId) {
            return track.id;
        }
    }

    for (const AudioTrack &track : tracks) {
        if (sameLanguage(track.language, preferredLanguage)) {
            return track.id;
        }
    }

    for (const AudioTrack &track : tracks) {
        if (track.isDefault) {
            return track.id;
        }
    }

    return tracks.front().id;
}

QString describeAudioTrack(const AudioTrack &track, const QString &unknownLanguageTemplate) {
    QStringList parts;

    if (track.language.isEmpty()) {
        parts.append(unknownLanguageTemplate.arg(track.id));
    } else {
        parts.append(QStringLiteral("%1. %2").arg(track.id).arg(languageLabel(track.language)));
    }

    if (!track.title.isEmpty()) {
        parts.append(track.title);
    }

    QStringList formatParts;

    if (!track.codec.isEmpty()) {
        formatParts.append(track.codec.toUpper());
    }

    const QString channels = describeChannels(track.channelCount);

    if (!channels.isEmpty()) {
        formatParts.append(channels);
    }

    if (!formatParts.isEmpty()) {
        parts.append(formatParts.join(u' '));
    }

    return parts.join(QStringLiteral(" · "));
}

} // namespace afvp
