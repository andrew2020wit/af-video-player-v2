#pragma once

#include "SubtitleMerge.h"

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

// Rows of the subtitle list shown on pause (F-42 - F-47).
class SubtitleListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by App.subtitles")

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Role {
        StartMsRole = Qt::UserRole + 1,
        TimeTextRole,
        TextRole,
        SecondaryTextRole,
        // Pause before the cue as a share of the 60 s scale, 0 when shorter than 5 s (F-47).
        PauseFractionRole,
    };

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setCues(const afvp::MergedCues &cues);

    // Index of the current cue for a playback time in seconds, -1 before the first cue (F-45).
    Q_INVOKABLE int indexAt(double seconds) const;
    // Index of the first cue starting at or after a time in seconds, count() when there is none (I-89).
    Q_INVOKABLE int indexFrom(double seconds) const;
    Q_INVOKABLE double startSeconds(int index) const;
    Q_INVOKABLE QString text(int index) const;

signals:
    void countChanged();

private:
    afvp::MergedCues m_cues;
};
