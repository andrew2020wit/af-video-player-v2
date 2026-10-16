#include "CueText.h"

#include <QRegularExpression>
#include <QStringList>

namespace afvp {

namespace {

bool startsWithDialogueDash(const QString &line) {
    if (line.isEmpty()) {
        return false;
    }

    const QChar first = line.front();

    return first == u'-' || first == u'–' || first == u'—';
}

// The character of a numeric entity; empty for zero, a surrogate or a value beyond Unicode.
QString codePointText(const QString &number, int base) {
    bool ok = false;
    const char32_t code = number.toUInt(&ok, base);

    if (!ok || code == 0 || (code >= 0xD800 && code <= 0xDFFF) || code > 0x10FFFF) {
        return QString();
    }

    return QString::fromUcs4(&code, 1);
}

} // namespace

QString removeMarkupTags(const QString &text) {
    // A tag starts with a letter, '/' or a digit (timestamp tags); "a < b" stays untouched.
    static const QRegularExpression tag(QStringLiteral("<(?:/?[A-Za-z][^<>]*|[0-9][0-9:.]*)>"));
    QString result = text;

    return result.remove(tag);
}

QString removeAssOverrideBlocks(const QString &text) {
    static const QRegularExpression overrideBlock(QStringLiteral("\\{\\\\[^}]*\\}"));
    QString result = text;

    return result.remove(overrideBlock);
}

QString decodeHtmlEntities(const QString &text) {
    static const QRegularExpression entity(QStringLiteral("&(#[0-9]+|#[xX][0-9A-Fa-f]+|[A-Za-z]+);"));

    if (!text.contains(u'&')) {
        return text;
    }

    QString result;
    qsizetype lastEnd = 0;
    auto matches = entity.globalMatch(text);

    while (matches.hasNext()) {
        const auto match = matches.next();
        const QString name = match.captured(1);
        QString replacement;

        if (name.startsWith(u"#x") || name.startsWith(u"#X")) {
            replacement = codePointText(name.mid(2), 16);
        } else if (name.startsWith(u'#')) {
            replacement = codePointText(name.mid(1), 10);
        } else if (name == u"amp") {
            replacement = QStringLiteral("&");
        } else if (name == u"lt") {
            replacement = QStringLiteral("<");
        } else if (name == u"gt") {
            replacement = QStringLiteral(">");
        } else if (name == u"nbsp") {
            replacement = QStringLiteral(" ");
        } else if (name == u"quot") {
            replacement = QStringLiteral("\"");
        } else if (name == u"apos") {
            replacement = QStringLiteral("'");
        } else if (name == u"lrm" || name == u"rlm") {
            replacement = QString();
        } else {
            continue;
        }

        result.append(QStringView(text).mid(lastEnd, match.capturedStart() - lastEnd));
        result.append(replacement);
        lastEnd = match.capturedEnd();
    }

    result.append(QStringView(text).mid(lastEnd));

    return result;
}

QString cleanCueLines(const QString &text) {
    QStringList lines;
    const auto rawLines = QStringView(text).split(u'\n');

    for (const auto rawLine : rawLines) {
        const QString line = rawLine.toString().simplified();

        if (!line.isEmpty()) {
            lines.append(line);
        }
    }

    return lines.join(u'\n');
}

QString normalizeCueText(const QString &text) {
    const QStringList lines = cleanCueLines(text).split(u'\n', Qt::SkipEmptyParts);
    QString result;

    for (const QString &line : lines) {
        if (!result.isEmpty()) {
            result.append(startsWithDialogueDash(line) ? u'\n' : u' ');
        }

        result.append(line);
    }

    return result;
}

} // namespace afvp
