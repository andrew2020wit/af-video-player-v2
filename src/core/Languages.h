#pragma once

#include <QList>
#include <QString>

namespace afvp {

// A language of ISO 639-1 with its ISO 639-2 codes (bibliographic and terminological ones may differ).
struct Language {
    QString code;           // ISO 639-1, e.g. "en"
    QString bibliographic;  // ISO 639-2/B, e.g. "ger"
    QString terminological; // ISO 639-2/T, e.g. "deu"
    QString englishName;    // e.g. "German"
    QString nativeName;     // autonym, e.g. "Deutsch"
};

const QList<Language> &allLanguages();

// Finds a language by an ISO 639-1 / 639-2 code (case-insensitive, region suffix like "en-US" ignored)
// or by its English name. Returns nullptr when unknown.
const Language *findLanguage(const QString &tag);

// ISO 639-1 code for a known language, otherwise the lower-cased tag; empty for an empty tag.
QString normalizeLanguageCode(const QString &tag);

// ISO 639-1 and 639-2 codes are equal (I-54): "en" == "eng". Empty or unknown tags are compared as written.
bool sameLanguage(const QString &first, const QString &second);

// "English — en"; for an unknown tag - the tag itself.
QString languageLabel(const QString &tag);

} // namespace afvp
