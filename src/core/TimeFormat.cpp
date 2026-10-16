#include "TimeFormat.h"

#include <algorithm>

namespace afvp {

QString formatTime(qint64 timeMs) {
    const qint64 totalSeconds = std::max<qint64>(0, timeMs) / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = totalSeconds / 60 % 60;
    const qint64 seconds = totalSeconds % 60;

    if (hours > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    }

    return QStringLiteral("%1:%2").arg(minutes, 2, 10, QLatin1Char('0')).arg(seconds, 2, 10, QLatin1Char('0'));
}

} // namespace afvp
