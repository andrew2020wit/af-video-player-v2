#pragma once

#include <QByteArray>
#include <QString>

namespace afvp {

// Decodes a subtitle file (SPEC 3.4): BOM -> UTF-8 / UTF-16; valid UTF-8 -> UTF-8;
// otherwise windows-1251 or windows-1252, detected by which result looks more like text;
// when neither is clearly better, UTF-8 with broken sequences replaced.
QString decodeSubtitleBytes(const QByteArray &bytes);

} // namespace afvp
