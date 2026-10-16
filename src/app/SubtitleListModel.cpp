#include "SubtitleListModel.h"

#include "TimeFormat.h"

#include <algorithm>

namespace {

// F-47: pauses from 5 s are shown, 60 s and longer fill the whole width.
constexpr qint64 minimumPauseMs = 5000;
constexpr qint64 fullPauseMs = 60000;

} // namespace

int SubtitleListModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : int(m_cues.size());
}

QVariant SubtitleListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_cues.size()) {
        return {};
    }

    const afvp::MergedCue &cue = m_cues[index.row()];

    switch (role) {
    case StartMsRole:
        return cue.startMs;
    case TimeTextRole:
        return afvp::formatTime(cue.startMs);
    case TextRole:
        return cue.text;
    case SecondaryTextRole:
        return cue.secondaryText;
    case PauseFractionRole: {
        if (index.row() == 0) {
            return 0.0;
        }

        const qint64 pauseMs = cue.startMs - m_cues[index.row() - 1].endMs;

        if (pauseMs < minimumPauseMs) {
            return 0.0;
        }

        return std::min(1.0, double(pauseMs) / double(fullPauseMs));
    }
    default:
        return {};
    }
}

QHash<int, QByteArray> SubtitleListModel::roleNames() const {
    return {
        {StartMsRole, "startMs"},
        {TimeTextRole, "timeText"},
        {TextRole, "text"},
        {SecondaryTextRole, "secondaryText"},
        {PauseFractionRole, "pauseFraction"},
    };
}

void SubtitleListModel::setCues(const afvp::MergedCues &cues) {
    beginResetModel();
    m_cues = cues;
    endResetModel();
    emit countChanged();
}

int SubtitleListModel::indexAt(double seconds) const {
    return int(afvp::findCurrentCueIndex(m_cues, qint64(seconds * 1000.0 + 0.5)));
}

double SubtitleListModel::startSeconds(int index) const {
    if (index < 0 || index >= m_cues.size()) {
        return 0;
    }

    return double(m_cues[index].startMs) / 1000.0;
}

QString SubtitleListModel::text(int index) const {
    if (index < 0 || index >= m_cues.size()) {
        return QString();
    }

    return m_cues[index].text;
}
