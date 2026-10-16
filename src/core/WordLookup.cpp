#include "WordLookup.h"

#include <QUrl>

namespace afvp {

namespace {

bool isApostrophe(QChar character) {
    return character == u'\'' || character == u'’';
}

bool isWordLetter(QChar character) {
    return character.isLetterOrNumber() || character.isMark();
}

// A word character: a letter, digit or combining mark, or an apostrophe between two letters.
bool isWordCharacter(const QString &text, qsizetype index) {
    if (index < 0 || index >= text.size()) {
        return false;
    }

    const QChar character = text[index];

    if (isWordLetter(character)) {
        return true;
    }

    return isApostrophe(character) && index > 0 && index + 1 < text.size() && text[index - 1].isLetter()
        && text[index + 1].isLetter();
}

QString removePossessiveSuffix(const QString &word) {
    if (word.size() > 2 && isApostrophe(word[word.size() - 2])
        && word.back().toLower() == u's') {
        return word.left(word.size() - 2);
    }

    return word;
}

} // namespace

QString wordAt(const QString &text, int position) {
    qsizetype anchor = -1;

    if (isWordCharacter(text, position)) {
        anchor = position;
    } else if (isWordCharacter(text, position - 1)) {
        anchor = position - 1;
    }

    if (anchor < 0) {
        return QString();
    }

    qsizetype start = anchor;
    qsizetype end = anchor + 1;

    while (isWordCharacter(text, start - 1)) {
        --start;
    }

    while (isWordCharacter(text, end)) {
        ++end;
    }

    return removePossessiveSuffix(text.mid(start, end - start));
}

QString cleanSelection(const QString &selection) {
    const QString text = selection.simplified();
    qsizetype start = 0;
    qsizetype end = text.size();

    while (start < end && !isWordLetter(text[start])) {
        ++start;
    }

    while (end > start && !isWordLetter(text[end - 1])) {
        --end;
    }

    return text.mid(start, end - start);
}

QString fillUrlTemplate(const QString &urlTemplate, const QString &text, const QString &sourceLanguage,
                        const QString &targetLanguage) {
    QString result = urlTemplate;
    result.replace(QStringLiteral("{text}"), QString::fromLatin1(QUrl::toPercentEncoding(text)));
    result.replace(QStringLiteral("{src}"), QString::fromLatin1(QUrl::toPercentEncoding(sourceLanguage)));
    result.replace(QStringLiteral("{dst}"), QString::fromLatin1(QUrl::toPercentEncoding(targetLanguage)));

    return result;
}

} // namespace afvp
