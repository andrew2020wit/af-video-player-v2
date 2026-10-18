#include "SubtitleMerge.h"

#include <QTest>

using namespace afvp;

class SubtitleMergeTest : public QObject {
    Q_OBJECT

private slots:
    void noSecondary();
    void largestOverlapWins();
    void tieGoesToEarlierCue();
    void gapGoesToNearestCue();
    void gapTieGoesToEarlierCue();
    void beforeFirstAndAfterLast();
    void severalSecondaryCuesJoinedWithNewline();
    void secondaryAtZeroIsMerged();
    void longMainCueFarBehind();
    void noMainCues();
    void textIsNormalized();
    void shift();
    void currentCue_data();
    void currentCue();
    void cueFrom_data();
    void cueFrom();
    void activeText_data();
    void activeText();
};

namespace {

SubtitleCue cue(qint64 startMs, qint64 endMs, const QString &text) {
    return {startMs, endMs, text};
}

} // namespace

void SubtitleMergeTest::noSecondary() {
    const MergedCues merged = mergeSubtitles({cue(0, 1000, "A"), cue(2000, 3000, "B")}, {});

    QCOMPARE(merged.size(), 2);
    QCOMPARE(merged[0], (MergedCue{0, 1000, "A", ""}));
    QCOMPARE(merged[1], (MergedCue{2000, 3000, "B", ""}));
}

void SubtitleMergeTest::largestOverlapWins() {
    // The secondary cue overlaps A by 200 ms and B by 800 ms.
    const MergedCues merged =
        mergeSubtitles({cue(0, 1000, "A"), cue(1000, 3000, "B")}, {cue(800, 1800, "b")});

    QCOMPARE(merged[0].secondaryText, QString());
    QCOMPARE(merged[1].secondaryText, QStringLiteral("b"));
}

void SubtitleMergeTest::tieGoesToEarlierCue() {
    const MergedCues merged =
        mergeSubtitles({cue(0, 1000, "A"), cue(1000, 2000, "B")}, {cue(500, 1500, "ab")});

    QCOMPARE(merged[0].secondaryText, QStringLiteral("ab"));
    QCOMPARE(merged[1].secondaryText, QString());
}

void SubtitleMergeTest::gapGoesToNearestCue() {
    // The secondary cue is in the pause: 300 ms after A, 1000 ms before B.
    const MergedCues merged =
        mergeSubtitles({cue(0, 1000, "A"), cue(3000, 4000, "B")}, {cue(1300, 2000, "x")});

    QCOMPARE(merged[0].secondaryText, QStringLiteral("x"));
    QCOMPARE(merged[1].secondaryText, QString());
}

void SubtitleMergeTest::gapTieGoesToEarlierCue() {
    const MergedCues merged =
        mergeSubtitles({cue(0, 1000, "A"), cue(3000, 4000, "B")}, {cue(1500, 2500, "x")});

    QCOMPARE(merged[0].secondaryText, QStringLiteral("x"));
}

void SubtitleMergeTest::beforeFirstAndAfterLast() {
    const MergedCues merged = mergeSubtitles({cue(1000, 2000, "A"), cue(3000, 4000, "B")},
                                             {cue(0, 500, "before"), cue(5000, 6000, "after")});

    QCOMPARE(merged[0].secondaryText, QStringLiteral("before"));
    QCOMPARE(merged[1].secondaryText, QStringLiteral("after"));
}

void SubtitleMergeTest::severalSecondaryCuesJoinedWithNewline() {
    const MergedCues merged =
        mergeSubtitles({cue(0, 4000, "Long sentence")}, {cue(0, 2000, "Part one,"), cue(2000, 4000, "part two.")});

    QCOMPARE(merged[0].secondaryText, QStringLiteral("Part one,\npart two."));
}

void SubtitleMergeTest::secondaryAtZeroIsMerged() {
    // A secondary cue at 0 ms must not be skipped (F-44).
    const MergedCues merged =
        mergeSubtitles({cue(0, 1000, "A"), cue(1000, 2000, "B")}, {cue(0, 1000, "a"), cue(1000, 2000, "b")});

    QCOMPARE(merged[0].secondaryText, QStringLiteral("a"));
    QCOMPARE(merged[1].secondaryText, QStringLiteral("b"));
}

void SubtitleMergeTest::longMainCueFarBehind() {
    // A long main cue that starts long before the secondary one still gets it by overlap.
    const MergedCues merged = mergeSubtitles(
        {cue(0, 60000, "Song"), cue(10000, 11000, "A"), cue(20000, 21000, "B")}, {cue(40000, 41000, "lyrics")});

    QCOMPARE(merged[0].secondaryText, QStringLiteral("lyrics"));
    QCOMPARE(merged[2].secondaryText, QString());
}

void SubtitleMergeTest::noMainCues() {
    QVERIFY(mergeSubtitles({}, {cue(0, 1000, "a")}).isEmpty());
}

void SubtitleMergeTest::textIsNormalized() {
    const MergedCues merged =
        mergeSubtitles({cue(0, 1000, "Hello\nworld")}, {cue(0, 1000, "- Привет.\n- Мир.")});

    QCOMPARE(merged[0].text, QStringLiteral("Hello world"));
    QCOMPARE(merged[0].secondaryText, QStringLiteral("- Привет.\n- Мир."));
}

void SubtitleMergeTest::shift() {
    const SubtitleCues shifted = shiftCues({cue(1000, 2000, "A")}, -500);

    QCOMPARE(shifted[0], cue(500, 1500, "A"));
}

void SubtitleMergeTest::currentCue_data() {
    QTest::addColumn<qint64>("timeMs");
    QTest::addColumn<qsizetype>("index");

    QTest::newRow("before first") << qint64(500) << qsizetype(-1);
    QTest::newRow("at first start") << qint64(1000) << qsizetype(0);
    QTest::newRow("in the pause") << qint64(2500) << qsizetype(0);
    QTest::newRow("second") << qint64(3000) << qsizetype(1);
    QTest::newRow("after last") << qint64(99000) << qsizetype(2);
}

void SubtitleMergeTest::currentCue() {
    QFETCH(qint64, timeMs);
    QFETCH(qsizetype, index);
    const SubtitleCues cues = {cue(1000, 2000, "A"), cue(3000, 4000, "B"), cue(5000, 6000, "C")};

    QCOMPARE(findCurrentCueIndex(cues, timeMs), index);
}

void SubtitleMergeTest::cueFrom_data() {
    QTest::addColumn<qint64>("timeMs");
    QTest::addColumn<qsizetype>("index");

    QTest::newRow("before first") << qint64(0) << qsizetype(0);
    QTest::newRow("at first start") << qint64(1000) << qsizetype(0);
    QTest::newRow("inside first") << qint64(1500) << qsizetype(1);
    QTest::newRow("in the pause") << qint64(2500) << qsizetype(1);
    QTest::newRow("at last start") << qint64(5000) << qsizetype(2);
    QTest::newRow("after last start") << qint64(5001) << qsizetype(3);
}

void SubtitleMergeTest::cueFrom() {
    QFETCH(qint64, timeMs);
    QFETCH(qsizetype, index);
    const SubtitleCues cues = {cue(1000, 2000, "A"), cue(3000, 4000, "B"), cue(5000, 6000, "C")};

    QCOMPARE(findCueIndexFrom(cues, timeMs), index);
}

void SubtitleMergeTest::activeText_data() {
    QTest::addColumn<qint64>("timeMs");
    QTest::addColumn<QString>("text");

    QTest::newRow("before first") << qint64(500) << QString();
    QTest::newRow("lines of a cue joined") << qint64(1000) << QStringLiteral("A line 2");
    QTest::newRow("at first end") << qint64(2000) << QString();
    QTest::newRow("in the pause") << qint64(2500) << QString();
    QTest::newRow("overlap") << qint64(3500) << QStringLiteral("Long\nB");
    QTest::newRow("long cue after a short one") << qint64(4500) << QStringLiteral("Long");
    QTest::newRow("dialogue lines kept") << qint64(7000) << QStringLiteral("- Hi.\n- Hello.");
    QTest::newRow("after last") << qint64(99000) << QString();
}

void SubtitleMergeTest::activeText() {
    QFETCH(qint64, timeMs);
    QFETCH(QString, text);
    const SubtitleCues cues = {cue(1000, 2000, "A\nline 2"), cue(2900, 6000, "Long"), cue(3000, 4000, "B"),
                               cue(7000, 8000, "- Hi.\n- Hello.")};

    QCOMPARE(activeCueText(cues, timeMs), text);
}

QTEST_GUILESS_MAIN(SubtitleMergeTest)

#include "tst_subtitlemerge.moc"
