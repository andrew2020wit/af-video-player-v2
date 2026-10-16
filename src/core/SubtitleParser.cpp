#include "SubtitleParser.h"

#include "CueText.h"
#include "TextDecoding.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>

#include <algorithm>
#include <optional>

namespace afvp {

namespace {

QString normalizeLineEndings(const QString &text) {
    QString result = text;
    result.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    result.replace(u'\r', u'\n');

    if (result.startsWith(QChar(0xFEFF))) {
        result.remove(0, 1);
    }

    return result;
}

// Splits text into blocks separated by blank lines.
QList<QStringList> splitBlocks(const QString &text) {
    QList<QStringList> blocks;
    QStringList current;
    const QStringList lines = text.split(u'\n');

    for (const QString &line : lines) {
        if (line.trimmed().isEmpty()) {
            if (!current.isEmpty()) {
                blocks.append(current);
                current.clear();
            }

            continue;
        }

        current.append(line);
    }

    if (!current.isEmpty()) {
        blocks.append(current);
    }

    return blocks;
}

// "HH:MM:SS,mmm", "H:MM:SS.mmm", "MM:SS.mmm" (WebVTT); fractional part of 1-3 digits.
std::optional<qint64> parseTimestamp(QStringView value) {
    static const QRegularExpression pattern(
        QStringLiteral("^\\s*(?:(\\d+):)?(\\d{1,2}):(\\d{1,2})(?:[.,](\\d{1,3}))?\\s*$"));
    const auto match = pattern.matchView(value);

    if (!match.hasMatch()) {
        return std::nullopt;
    }

    const qint64 hours = match.capturedView(1).isEmpty() ? 0 : match.capturedView(1).toLongLong();
    const qint64 minutes = match.capturedView(2).toLongLong();
    const qint64 seconds = match.capturedView(3).toLongLong();
    QString fraction = match.captured(4);

    while (fraction.size() < 3) {
        fraction.append(u'0');
    }

    return ((hours * 60 + minutes) * 60 + seconds) * 1000 + fraction.toLongLong();
}

// "start --> end [settings]"
std::optional<std::pair<qint64, qint64>> parseTimingLine(const QString &line) {
    const qsizetype arrow = line.indexOf(QStringLiteral("-->"));

    if (arrow < 0) {
        return std::nullopt;
    }

    const QStringView startPart = QStringView(line).left(arrow);
    QStringView endPart = QStringView(line).mid(arrow + 3).trimmed();
    const qsizetype space = endPart.indexOf(u' ');

    if (space >= 0) {
        endPart = endPart.left(space);
    }

    const auto start = parseTimestamp(startPart);
    const auto end = parseTimestamp(endPart);

    if (!start || !end) {
        return std::nullopt;
    }

    return std::pair{*start, *end};
}

void sortCues(SubtitleCues &cues) {
    std::stable_sort(cues.begin(), cues.end(), [](const SubtitleCue &left, const SubtitleCue &right) {
        return left.startMs < right.startMs;
    });
}

void appendCue(SubtitleCues &cues, qint64 startMs, qint64 endMs, const QString &text) {
    const QString cleanText = cleanCueLines(text);

    if (cleanText.isEmpty()) {
        return;
    }

    cues.append({startMs, std::max(startMs, endMs), cleanText});
}

// ASS time "H:MM:SS.cc"
std::optional<qint64> parseAssTime(QStringView value) {
    return parseTimestamp(value.trimmed());
}

// Removes override blocks and drawings (text between \p1 and \p0), converts \N, \n and \h.
QString cleanAssText(const QString &text) {
    static const QRegularExpression drawingTag(QStringLiteral("\\\\p(\\d+)"));
    QString result;
    bool drawing = false;
    qsizetype position = 0;

    while (position < text.size()) {
        const qsizetype blockStart = text.indexOf(u'{', position);
        const qsizetype textEnd = blockStart < 0 ? text.size() : blockStart;

        if (!drawing) {
            result.append(QStringView(text).mid(position, textEnd - position));
        }

        if (blockStart < 0) {
            break;
        }

        const qsizetype blockEnd = text.indexOf(u'}', blockStart);

        if (blockEnd < 0) {
            break;
        }

        const QString block = text.mid(blockStart + 1, blockEnd - blockStart - 1);
        auto matches = drawingTag.globalMatch(block);

        while (matches.hasNext()) {
            drawing = matches.next().captured(1).toInt() > 0;
        }

        position = blockEnd + 1;
    }

    result.replace(QStringLiteral("\\N"), QStringLiteral("\n"));
    result.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
    result.replace(QStringLiteral("\\h"), QStringLiteral(" "));

    return result;
}

} // namespace

SubtitleCues parseSrt(const QString &text) {
    SubtitleCues cues;
    const auto blocks = splitBlocks(normalizeLineEndings(text));

    for (const QStringList &block : blocks) {
        for (qsizetype i = 0; i < block.size(); ++i) {
            const auto timing = parseTimingLine(block[i]);

            if (!timing) {
                continue;
            }

            const QString body = block.mid(i + 1).join(u'\n');
            appendCue(cues, timing->first, timing->second,
                      decodeHtmlEntities(removeMarkupTags(removeAssOverrideBlocks(body))));

            break;
        }
    }

    sortCues(cues);

    return cues;
}

SubtitleCues parseVtt(const QString &text) {
    SubtitleCues cues;
    const auto blocks = splitBlocks(normalizeLineEndings(text));

    for (const QStringList &block : blocks) {
        const QString firstLine = block.front().trimmed();
        const bool isServiceBlock = firstLine.startsWith(u"WEBVTT") || firstLine == u"NOTE"
            || firstLine.startsWith(u"NOTE ") || firstLine.startsWith(u"NOTE\t") || firstLine == u"STYLE"
            || firstLine == u"REGION";

        if (isServiceBlock && !firstLine.contains(u"-->")) {
            continue;
        }

        for (qsizetype i = 0; i < block.size() && i < 2; ++i) {
            const auto timing = parseTimingLine(block[i]);

            if (!timing) {
                continue;
            }

            const QString body = block.mid(i + 1).join(u'\n');
            appendCue(cues, timing->first, timing->second, decodeHtmlEntities(removeMarkupTags(body)));

            break;
        }
    }

    sortCues(cues);

    return cues;
}

SubtitleCues parseAss(const QString &text) {
    // Default field order of ASS ("Layer") and SSA ("Marked") files without a Format line.
    QStringList format = {QStringLiteral("layer"), QStringLiteral("start"), QStringLiteral("end"),
                          QStringLiteral("style"), QStringLiteral("name"), QStringLiteral("marginl"),
                          QStringLiteral("marginr"), QStringLiteral("marginv"), QStringLiteral("effect"),
                          QStringLiteral("text")};
    SubtitleCues cues;
    bool inEvents = false;
    const QStringList lines = normalizeLineEndings(text).split(u'\n');

    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();

        if (line.startsWith(u'[')) {
            inEvents = line.compare(u"[Events]", Qt::CaseInsensitive) == 0;

            continue;
        }

        if (!inEvents) {
            continue;
        }

        const qsizetype colon = line.indexOf(u':');

        if (colon < 0) {
            continue;
        }

        const QString key = line.left(colon).trimmed().toLower();
        const QString value = line.mid(colon + 1);

        if (key == u"format") {
            format.clear();
            const QStringList fields = value.split(u',');

            for (const QString &field : fields) {
                format.append(field.trimmed().toLower());
            }

            continue;
        }

        if (key != u"dialogue") {
            continue;
        }

        const qsizetype startIndex = format.indexOf(QStringLiteral("start"));
        const qsizetype endIndex = format.indexOf(QStringLiteral("end"));
        const qsizetype textIndex = format.indexOf(QStringLiteral("text"));

        if (startIndex < 0 || endIndex < 0 || textIndex != format.size() - 1) {
            continue;
        }

        // The text is the last field and may contain commas.
        QStringList fields;
        qsizetype position = 0;

        for (qsizetype i = 0; i < format.size() - 1; ++i) {
            const qsizetype comma = value.indexOf(u',', position);

            if (comma < 0) {
                break;
            }

            fields.append(value.mid(position, comma - position));
            position = comma + 1;
        }

        if (fields.size() != format.size() - 1) {
            continue;
        }

        fields.append(value.mid(position));
        const auto start = parseAssTime(fields[startIndex]);
        const auto end = parseAssTime(fields[endIndex]);

        if (!start || !end) {
            continue;
        }

        appendCue(cues, *start, *end, cleanAssText(fields[textIndex]));
    }

    sortCues(cues);

    return cues;
}

SubtitleFormat detectSubtitleFormat(const QString &fileName, const QString &text) {
    const QString suffix = QFileInfo(fileName).suffix().toLower();

    if (suffix == u"srt") {
        return SubtitleFormat::Srt;
    }

    if (suffix == u"vtt") {
        return SubtitleFormat::Vtt;
    }

    if (suffix == u"ass" || suffix == u"ssa") {
        return SubtitleFormat::Ass;
    }

    const QString head = text.left(4096).trimmed();

    if (head.startsWith(u"WEBVTT") || head.startsWith(QStringLiteral("﻿WEBVTT"))) {
        return SubtitleFormat::Vtt;
    }

    if (head.contains(QStringLiteral("[Script Info]"), Qt::CaseInsensitive)
        || head.contains(QStringLiteral("[Events]"), Qt::CaseInsensitive)) {
        return SubtitleFormat::Ass;
    }

    if (head.contains(QStringLiteral("-->"))) {
        return SubtitleFormat::Srt;
    }

    return SubtitleFormat::Unknown;
}

SubtitleCues parseSubtitles(const QString &text, SubtitleFormat format) {
    switch (format) {
    case SubtitleFormat::Srt:
        return parseSrt(text);
    case SubtitleFormat::Vtt:
        return parseVtt(text);
    case SubtitleFormat::Ass:
        return parseAss(text);
    case SubtitleFormat::Unknown:
        break;
    }

    return {};
}

SubtitleLoadResult loadSubtitleFile(const QString &path) {
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        return {false, file.errorString(), {}};
    }

    // Subtitle files are small; refuse anything unreasonably large.
    constexpr qint64 maximumSize = 64 * 1024 * 1024;

    if (file.size() > maximumSize) {
        return {false, QStringLiteral("File is too large"), {}};
    }

    const QString text = decodeSubtitleBytes(file.readAll());
    const SubtitleFormat format = detectSubtitleFormat(path, text);

    if (format == SubtitleFormat::Unknown) {
        return {false, QStringLiteral("Unknown subtitle format"), {}};
    }

    return {true, QString(), parseSubtitles(text, format)};
}

} // namespace afvp
