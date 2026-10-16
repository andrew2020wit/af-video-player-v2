#pragma once

#include <QString>

namespace afvp {

// Word for a dictionary lookup at a cursor position in a cue text (I-59). The position is a cursor position
// between characters (as returned by TextEdit.positionAt): the word touching it is taken.
// Returns an empty string when the position is on a separator.
QString wordAt(const QString &text, int position);

// Selected text for a dictionary lookup (I-59): whitespace collapsed, punctuation and quotes trimmed at the ends.
QString cleanSelection(const QString &selection);

// Substitutes {text}, {src} and {dst} in a URL template; values are percent-encoded (I-40).
QString fillUrlTemplate(const QString &urlTemplate, const QString &text, const QString &sourceLanguage,
                        const QString &targetLanguage);

} // namespace afvp
