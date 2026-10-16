#pragma once

#include <QList>
#include <QString>

namespace afvp {

// An audio track as reported by mpv's track-list.
struct AudioTrack {
    int id = 0; // mpv "aid", starts at 1
    QString language;
    QString title;
    QString codec;
    int channelCount = 0;
    bool isDefault = false;
};

// Audio track selection on file open (I-54):
// 1) the track the user chose for this file; 2) the first track in the preferred language;
// 3) the file's default track (or the first one). Returns 0 when there are no audio tracks.
int chooseAudioTrack(const QList<AudioTrack> &tracks, int userTrackId, const QString &preferredLanguage);

// "2. English — en · Commentary · AC3 5.1" (I-02). Without a language the first part is
// unknownLanguageTemplate with %1 replaced by the track id, e.g. "Track 2 · AC3 5.1".
QString describeAudioTrack(const AudioTrack &track, const QString &unknownLanguageTemplate);

} // namespace afvp
