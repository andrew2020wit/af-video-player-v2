#pragma once

#include <QString>

namespace afvp {

// "H:MM:SS" when the time is an hour or longer, otherwise "MM:SS" (F-12).
QString formatTime(qint64 timeMs);

} // namespace afvp
