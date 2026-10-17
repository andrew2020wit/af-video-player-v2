#include "AutoExport.h"

#include <QTest>

using namespace afvp;

class AutoExportTest : public QObject {
    Q_OBJECT

private slots:
    void trackFlags_data();
    void trackFlags();
    void choice_data();
    void choice();
};

namespace {

// "<stream index>|<language>|<flags>|<title>"; flags: f - forced, s - hearing impaired, b - bitmap.
EmbeddedSubtitleTrack parseTrack(const QString &spec) {
    const QStringList parts = spec.split(u'|');
    EmbeddedSubtitleTrack track;
    track.streamIndex = parts.value(0).toInt();
    track.codecName = QStringLiteral("subrip");
    track.language = parts.value(1);
    track.forced = parts.value(2).contains(u'f');
    track.hearingImpaired = parts.value(2).contains(u's');
    track.isText = !parts.value(2).contains(u'b');
    track.title = parts.value(3);

    return track;
}

QList<EmbeddedSubtitleTrack> parseTracks(const QStringList &specs) {
    QList<EmbeddedSubtitleTrack> tracks;

    for (const QString &spec : specs) {
        tracks.append(parseTrack(spec));
    }

    return tracks;
}

} // namespace

void AutoExportTest::trackFlags_data() {
    QTest::addColumn<QString>("track");
    QTest::addColumn<bool>("forced");
    QTest::addColumn<bool>("sdh");

    QTest::newRow("plain") << "1|eng||English" << false << false;
    QTest::newRow("forced flag") << "1|eng|f|" << true << false;
    QTest::newRow("forced title") << "1|eng||English (Forced)" << true << false;
    QTest::newRow("sdh flag") << "1|eng|s|" << false << true;
    QTest::newRow("sdh title") << "1|eng||English SDH" << false << true;
    QTest::newRow("cc title") << "1|eng||English [CC]" << false << true;
    QTest::newRow("hi title") << "1|eng||English.HI" << false << true;
    QTest::newRow("words inside other words") << "1|eng||Unforced Hindi CCTV" << false << false;
}

void AutoExportTest::trackFlags() {
    QFETCH(QString, track);
    QFETCH(bool, forced);
    QFETCH(bool, sdh);

    QCOMPARE(isForcedTrack(parseTrack(track)), forced);
    QCOMPARE(isSdhTrack(parseTrack(track)), sdh);
}

void AutoExportTest::choice_data() {
    QTest::addColumn<QStringList>("tracks");
    QTest::addColumn<QString>("studied");
    QTest::addColumn<QString>("native");
    QTest::addColumn<int>("main");
    QTest::addColumn<int>("secondary");

    QTest::newRow("no tracks") << QStringList() << "en" << "uk" << -1 << -1;
    QTest::newRow("first of each language")
        << QStringList{"2|rus||", "3|eng||", "4|ukr||", "5|eng||", "6|ukr||"} << "en" << "uk" << 3 << 4;
    QTest::newRow("639-1 tags") << QStringList{"2|en||", "3|uk||"} << "en" << "uk" << 2 << 3;
    QTest::newRow("forced skipped")
        << QStringList{"2|eng|f|", "3|eng||Forced", "4|eng||", "5|ukr|f|"} << "en" << "uk" << 4 << -1;
    QTest::newRow("only forced") << QStringList{"2|eng|f|"} << "en" << "uk" << -1 << -1;
    QTest::newRow("ordinary before sdh") << QStringList{"2|eng|s|", "3|eng||SDH", "4|eng||"} << "en" << "uk" << 4
                                         << -1;
    QTest::newRow("sdh when no other") << QStringList{"2|eng|fs|", "3|eng|s|", "4|eng||CC"} << "en" << "uk" << 3
                                       << -1;
    QTest::newRow("bitmap skipped") << QStringList{"2|eng|b|", "3|eng||"} << "en" << "uk" << 3 << -1;
    QTest::newRow("no language skipped") << QStringList{"2|||", "3|und||"} << "en" << "uk" << -1 << -1;
    QTest::newRow("native becomes main") << QStringList{"2|rus||", "3|ukr||"} << "en" << "uk" << 3 << -1;
    QTest::newRow("same languages") << QStringList{"2|eng||", "3|eng||"} << "en" << "en" << 2 << -1;
}

void AutoExportTest::choice() {
    QFETCH(QStringList, tracks);
    QFETCH(QString, studied);
    QFETCH(QString, native);
    QFETCH(int, main);
    QFETCH(int, secondary);

    const AutoExportChoice result = chooseAutoExportTracks(parseTracks(tracks), studied, native);

    QCOMPARE(result.mainStreamIndex, main);
    QCOMPARE(result.secondaryStreamIndex, secondary);
}

QTEST_GUILESS_MAIN(AutoExportTest)

#include "tst_autoexport.moc"
