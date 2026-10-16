#include "CueText.h"
#include "SubtitleParser.h"
#include "TextDecoding.h"

#include <QTest>

using namespace afvp;

class SubtitleParserTest : public QObject {
    Q_OBJECT

private slots:
    void srtBasic();
    void srtTolerant();
    void srtTags();
    void vttBasic();
    void vttServiceBlocksAndTags();
    void assBasic();
    void assDrawingAndFormat();
    void sortedByStart();
    void detectFormat_data();
    void detectFormat();
    void decodeUtf8Bom();
    void decodeUtf16();
    void decodeSingleByte_data();
    void decodeSingleByte();
    void loadFiles_data();
    void loadFiles();
    void normalizeText_data();
    void normalizeText();
    void htmlEntities_data();
    void htmlEntities();
};

void SubtitleParserTest::srtBasic() {
    const QString text = QStringLiteral("1\r\n00:00:01,000 --> 00:00:02,500\r\nHello\r\nworld\r\n\r\n"
                                        "2\r\n00:01:02,03 --> 01:00:00,000\r\nSecond\r\n");
    const SubtitleCues cues = parseSrt(text);

    QCOMPARE(cues.size(), 2);
    QCOMPARE(cues[0], (SubtitleCue{1000, 2500, QStringLiteral("Hello\nworld")}));
    QCOMPARE(cues[1].startMs, 62030);
    QCOMPARE(cues[1].endMs, 3600000);
}

void SubtitleParserTest::srtTolerant() {
    // Missing index, dot separator, a broken block and a cue that is empty after removing tags.
    const QString text = QStringLiteral("00:00:01.000 --> 00:00:02.000\nNo index\n\n"
                                        "3\n00:00:xx,000 --> 00:00:04,000\nBroken\n\n"
                                        "4\n00:00:05,000 --> 00:00:06,000\n<i></i>\n\n"
                                        "5\n00:00:07,000 --> 00:00:08,000\nLast");
    const SubtitleCues cues = parseSrt(text);

    QCOMPARE(cues.size(), 2);
    QCOMPARE(cues[0].text, QStringLiteral("No index"));
    QCOMPARE(cues[1].text, QStringLiteral("Last"));
}

void SubtitleParserTest::srtTags() {
    const QString text = QStringLiteral("1\n00:00:01,000 --> 00:00:02,000\n"
                                        "{\\an8}<i>Hi</i>, <font color=\"#fff\">you</font> &amp; <b>me</b> 1 < 2\n");
    const SubtitleCues cues = parseSrt(text);

    QCOMPARE(cues.size(), 1);
    QCOMPARE(cues[0].text, QStringLiteral("Hi, you & me 1 < 2"));
}

void SubtitleParserTest::vttBasic() {
    const QString text = QStringLiteral("WEBVTT\n\n00:01.000 --> 00:02.000 align:start line:0\nShort time\n\n"
                                        "cue-id\n01:00:00.500 --> 01:00:01.000\nLong time\n");
    const SubtitleCues cues = parseVtt(text);

    QCOMPARE(cues.size(), 2);
    QCOMPARE(cues[0], (SubtitleCue{1000, 2000, QStringLiteral("Short time")}));
    QCOMPARE(cues[1], (SubtitleCue{3600500, 3601000, QStringLiteral("Long time")}));
}

void SubtitleParserTest::vttServiceBlocksAndTags() {
    const QString text = QStringLiteral("WEBVTT - title\n\nNOTE a comment\nwith two lines\n\n"
                                        "STYLE\n::cue { color: red }\n\n"
                                        "00:00:01.000 --> 00:00:02.000\n"
                                        "<v Bob>Hello</v> <c.yellow>dear</c> <00:00:01.500>friend&nbsp;&lt;3\n");
    const SubtitleCues cues = parseVtt(text);

    QCOMPARE(cues.size(), 1);
    QCOMPARE(cues[0].text, QStringLiteral("Hello dear friend <3"));
}

void SubtitleParserTest::assBasic() {
    const QString text = QStringLiteral(
        "[Script Info]\nTitle: test\n\n[V4+ Styles]\nFormat: Name, Fontname\nStyle: Default,Arial\n\n"
        "[Events]\n"
        "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n"
        "Dialogue: 0,0:00:01.50,0:00:03.00,Default,,0,0,0,,{\\i1}Hello{\\i0}, world\\Nsecond\\hline\n"
        "Comment: 0,0:00:04.00,0:00:05.00,Default,,0,0,0,,Comment\n");
    const SubtitleCues cues = parseAss(text);

    QCOMPARE(cues.size(), 1);
    QCOMPARE(cues[0], (SubtitleCue{1500, 3000, QStringLiteral("Hello, world\nsecond line")}));
}

void SubtitleParserTest::assDrawingAndFormat() {
    // Non-standard field order and a drawing that must be dropped.
    const QString text = QStringLiteral(
        "[Events]\nFormat: Start, End, Layer, Style, Text\n"
        "Dialogue: 0:00:02.00,0:00:03.00,0,Default,{\\p1}m 0 0 l 100 0 100 100{\\p0}Visible\n"
        "Dialogue: 0:00:01.00,0:00:02.00,0,Default,{\\p1}m 0 0 l 10 10\n");
    const SubtitleCues cues = parseAss(text);

    QCOMPARE(cues.size(), 1);
    QCOMPARE(cues[0], (SubtitleCue{2000, 3000, QStringLiteral("Visible")}));
}

void SubtitleParserTest::sortedByStart() {
    const QString text = QStringLiteral("2\n00:00:05,000 --> 00:00:06,000\nB\n\n1\n00:00:01,000 --> 00:00:02,000\nA\n");
    const SubtitleCues cues = parseSrt(text);

    QCOMPARE(cues.size(), 2);
    QCOMPARE(cues[0].text, QStringLiteral("A"));
}

void SubtitleParserTest::detectFormat_data() {
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("format");

    QTest::newRow("srt extension") << "a.SRT" << "" << int(SubtitleFormat::Srt);
    QTest::newRow("vtt extension") << "a.vtt" << "" << int(SubtitleFormat::Vtt);
    QTest::newRow("ssa extension") << "a.ssa" << "" << int(SubtitleFormat::Ass);
    QTest::newRow("vtt content") << "a.txt" << "WEBVTT\n\n" << int(SubtitleFormat::Vtt);
    QTest::newRow("ass content") << "a.sub" << "[Script Info]\n" << int(SubtitleFormat::Ass);
    QTest::newRow("srt content") << "a" << "1\n00:00:01,000 --> 00:00:02,000\n" << int(SubtitleFormat::Srt);
    QTest::newRow("unknown") << "a.txt" << "hello" << int(SubtitleFormat::Unknown);
}

void SubtitleParserTest::detectFormat() {
    QFETCH(QString, fileName);
    QFETCH(QString, text);
    QFETCH(int, format);

    QCOMPARE(int(detectSubtitleFormat(fileName, text)), format);
}

void SubtitleParserTest::decodeUtf8Bom() {
    QCOMPARE(decodeSubtitleBytes(QByteArray("\xEF\xBB\xBF\xD0\xBF\xD1\x80\xD0\xB8")), QStringLiteral("при"));
}

void SubtitleParserTest::decodeUtf16() {
    QCOMPARE(decodeSubtitleBytes(QByteArray("\xFF\xFE\x3F\x04\x40\x04", 6)), QStringLiteral("пр"));
    QCOMPARE(decodeSubtitleBytes(QByteArray("\xFE\xFF\x04\x3F\x04\x40", 6)), QStringLiteral("пр"));
}

void SubtitleParserTest::decodeSingleByte_data() {
    QTest::addColumn<QByteArray>("bytes");
    QTest::addColumn<QString>("text");

    // Not valid UTF-8, so the encoding is detected by the decoded text.
    QTest::newRow("windows-1251") << QByteArray("\xCF\xF0\xE8\xE2\xE5\xF2, \xA8\xE6\xE8\xEA \xB9\x31 \x97 \xB4")
                                  << "Привет, Ёжик №1 — ґ";
    QTest::newRow("windows-1251 upper case") << QByteArray("\xCF\xD0\xC8\xC2\xC5\xD2!") << "ПРИВЕТ!";
    QTest::newRow("windows-1251 ukrainian") << QByteArray("\xB2\xE6\xE0\xEA \xBF\xE4\xE5, \xBA\xE2\xE3\xE5\xED")
                                            << "Іжак їде, євген";
    QTest::newRow("windows-1252") << QByteArray("caf\xE9 \x80\x93") << "café €“";
    QTest::newRow("windows-1252 french") << QByteArray("\xC0 l'\xE9t\xE9, na\xEFve") << "À l'été, naïve";
    QTest::newRow("windows-1252 german") << QByteArray("Gr\xF6\xDF\x65 Stra\xDF\x65") << "Größe Straße";
    // Quotes and dashes are the same in both encodings.
    QTest::newRow("punctuation only") << QByteArray("I\x92m \x93here\x94 \x96 now\x85") << "I’m “here” – now…";
    // Neither encoding gives text-like words: UTF-8 with the broken byte replaced.
    QTest::newRow("undetermined") << QByteArray("a\x81" "b") << "a�b";
    // A tie: "Рыба xйz" in windows-1251, "Ðûáà xéz" in windows-1252 - one plausible word in each.
    QTest::newRow("undetermined tie") << QByteArray("\xD0\xFB\xE1\xE0 x\xE9z") << "���� x�z";
}

void SubtitleParserTest::decodeSingleByte() {
    QFETCH(QByteArray, bytes);
    QFETCH(QString, text);

    QCOMPARE(decodeSubtitleBytes(bytes), text);
}

void SubtitleParserTest::loadFiles_data() {
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<int>("cueCount");
    QTest::addColumn<QString>("firstText");

    QTest::newRow("srt utf-8") << "sample.en.srt" << 3 << "Where are you going?";
    QTest::newRow("srt windows-1251") << "sample.ru.cp1251.srt" << 3 << "Куда ты идёшь?";
    QTest::newRow("vtt") << "sample.uk.vtt" << 3 << "Куди ти йдеш?";
    QTest::newRow("ass") << "sample.en.ass" << 2 << "Where are you going?";
}

void SubtitleParserTest::loadFiles() {
    QFETCH(QString, fileName);
    QFETCH(int, cueCount);
    QFETCH(QString, firstText);

    const SubtitleLoadResult result = loadSubtitleFile(QStringLiteral(AFVP_TEST_DATA_DIR "/") + fileName);

    QVERIFY2(result.ok, qPrintable(result.error));
    QCOMPARE(result.cues.size(), cueCount);
    QCOMPARE(result.cues.front().text, firstText);
}

void SubtitleParserTest::normalizeText_data() {
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("normalized");

    QTest::newRow("single line") << "Hello" << "Hello";
    QTest::newRow("line break to space") << "I don't know\nwhat to say" << "I don't know what to say";
    QTest::newRow("dialogue dash") << "- Hi.\n- Hello." << "- Hi.\n- Hello.";
    QTest::newRow("en dash dialogue") << "– Hi.\n– Hello." << "– Hi.\n– Hello.";
    QTest::newRow("mixed") << "- Where are\nyou going?\n- Home." << "- Where are you going?\n- Home.";
    QTest::newRow("spaces") << "  too   many \n\n spaces  " << "too many spaces";
}

void SubtitleParserTest::normalizeText() {
    QFETCH(QString, text);
    QFETCH(QString, normalized);

    QCOMPARE(normalizeCueText(text), normalized);
}

void SubtitleParserTest::htmlEntities_data() {
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("decoded");

    QTest::newRow("named") << QStringLiteral("&lt;a&gt; &amp; &quot;b&quot; &apos;c&apos;")
                           << QStringLiteral("<a> & \"b\" 'c'");
    QTest::newRow("decimal") << QStringLiteral("caf&#233;") << QStringLiteral("café");
    QTest::newRow("hexadecimal") << QStringLiteral("&#x1F600;!") << QStringLiteral("😀!");
    QTest::newRow("direction marks") << QStringLiteral("a&lrm;b&rlm;") << QStringLiteral("ab");
    QTest::newRow("unknown name") << QStringLiteral("&copy; &foo;") << QStringLiteral("&copy; &foo;");
    QTest::newRow("surrogate") << QStringLiteral("a&#xD800;b") << QStringLiteral("ab");
    QTest::newRow("beyond unicode") << QStringLiteral("a&#x110000;b") << QStringLiteral("ab");
    QTest::newRow("zero") << QStringLiteral("a&#0;b") << QStringLiteral("ab");
}

void SubtitleParserTest::htmlEntities() {
    QFETCH(QString, text);
    QFETCH(QString, decoded);

    QCOMPARE(decodeHtmlEntities(text), decoded);
}

QTEST_GUILESS_MAIN(SubtitleParserTest)

#include "tst_subtitleparser.moc"
