#include "TextDecoding.h"

#include <QStringDecoder>

#include <array>

namespace afvp {

namespace {

constexpr char16_t undefined = 0xFFFD;

// Upper halves (0x80-0xFF) of the Windows code pages. Own tables keep decoding identical on all platforms,
// independent of ICU availability in the Qt build.
constexpr std::array<char16_t, 128> windows1251 = {
    0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
    0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
    0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    undefined, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
    0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
    0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
    0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
    0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
    0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
    0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
    0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
    0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
};

// Only 0x80-0x9F differ from ISO-8859-1.
constexpr std::array<char16_t, 32> windows1252Control = {
    0x20AC, undefined, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, undefined, 0x017D, undefined,
    undefined, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, undefined, 0x017E, 0x0178,
};

QString decodeWithTable(const QByteArray &bytes, const char16_t *upperHalf, int tableSize) {
    QString result;
    result.reserve(bytes.size());

    for (const char byte : bytes) {
        const auto code = static_cast<unsigned char>(byte);

        if (code >= 0x80 && code - 0x80 < tableSize) {
            result.append(QChar(upperHalf[code - 0x80]));
        } else {
            result.append(QChar(code));
        }
    }

    return result;
}

// How much a decoded text looks like real text; words with non-ASCII letters are judged one by one.
// A Cyrillic word has no Latin letters ("Привет", not "cafй"); a Latin word has only a few non-ASCII
// letters ("café", "été", not "Ïðèâåò"). Undefined characters and C1 controls count against.
int plausibility(const QString &text) {
    int score = 0;
    int asciiLetters = 0;
    int latinLetters = 0;
    int cyrillicLetters = 0;
    int otherLetters = 0;

    const auto finishWord = [&] {
        const int nonAsciiLetters = latinLetters + cyrillicLetters + otherLetters;

        if (nonAsciiLetters > 0) {
            const bool cyrillicWord = cyrillicLetters == nonAsciiLetters && asciiLetters == 0;
            const bool latinWord = latinLetters == nonAsciiLetters && nonAsciiLetters <= asciiLetters + 1;
            score += cyrillicWord || latinWord ? 1 : -1;
        }

        asciiLetters = latinLetters = cyrillicLetters = otherLetters = 0;
    };

    for (const QChar character : text) {
        if (character.isLetter()) {
            if (character.unicode() < 0x80) {
                ++asciiLetters;
            } else if (character.script() == QChar::Script_Cyrillic) {
                ++cyrillicLetters;
            } else if (character.script() == QChar::Script_Latin) {
                ++latinLetters;
            } else {
                ++otherLetters;
            }

            continue;
        }

        finishWord();

        if (character.unicode() == undefined || (character.unicode() >= 0x80 && character.unicode() < 0xA0)) {
            score -= 2;
        }
    }

    finishWord();

    return score;
}

} // namespace

QString decodeSubtitleBytes(const QByteArray &bytes) {
    if (bytes.startsWith("\xEF\xBB\xBF")) {
        return QString::fromUtf8(bytes.mid(3));
    }

    if (bytes.startsWith("\xFF\xFE")) {
        QStringDecoder decoder(QStringConverter::Utf16LE);

        return decoder.decode(bytes.mid(2));
    }

    if (bytes.startsWith("\xFE\xFF")) {
        QStringDecoder decoder(QStringConverter::Utf16BE);

        return decoder.decode(bytes.mid(2));
    }

    QStringDecoder utf8Decoder(QStringConverter::Utf8);
    QString text = utf8Decoder.decode(bytes);

    if (!utf8Decoder.hasError()) {
        return text;
    }

    // Not valid UTF-8: the single-byte encoding whose result looks like text and more so than the other one.
    QString cyrillic = decodeWithTable(bytes, windows1251.data(), int(windows1251.size()));
    QString western = decodeWithTable(bytes, windows1252Control.data(), int(windows1252Control.size()));

    // Only shared characters (quotes, dashes): both encodings give the same text.
    if (cyrillic == western) {
        return cyrillic;
    }

    const int cyrillicScore = plausibility(cyrillic);
    const int westernScore = plausibility(western);

    if (cyrillicScore > 0 && cyrillicScore > westernScore) {
        return cyrillic;
    }

    if (westernScore > 0 && westernScore > cyrillicScore) {
        return western;
    }

    // Undetermined: UTF-8 with broken sequences replaced.
    return text;
}

} // namespace afvp
