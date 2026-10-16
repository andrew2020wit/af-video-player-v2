#include "ExportNaming.h"

#include <QTest>

using namespace afvp;

class ExportNamingTest : public QObject {
    Q_OBJECT

private slots:
    void extensions();
    void basicNames();
    void duplicatesInBatch();
    void replaceIgnoresExistingFiles();
    void keepBothSkipsExistingFiles();
};

namespace {

EmbeddedSubtitleTrack track(const QString &codec, const QString &language, bool forced = false,
                            bool hearingImpaired = false) {
    EmbeddedSubtitleTrack result;
    result.codecName = codec;
    result.language = language;
    result.forced = forced;
    result.hearingImpaired = hearingImpaired;

    return result;
}

const auto noFiles = [](const QString &) { return false; };

} // namespace

void ExportNamingTest::extensions() {
    QCOMPARE(exportExtension("subrip"), QStringLiteral("srt"));
    QCOMPARE(exportExtension("ass"), QStringLiteral("ass"));
    QCOMPARE(exportExtension("ssa"), QStringLiteral("ssa"));
    QCOMPARE(exportExtension("webvtt"), QStringLiteral("vtt"));
    QCOMPARE(exportExtension("mov_text"), QStringLiteral("srt"));
    QVERIFY(canCopySubtitleCodec("subrip"));
    QVERIFY(!canCopySubtitleCodec("mov_text"));
}

void ExportNamingTest::basicNames() {
    const QStringList names = exportFileNames(
        "movie",
        {track("subrip", "eng"), track("ass", "rus"), track("subrip", "eng", true), track("webvtt", "ger", false, true),
         track("subrip", "")},
        ConflictPolicy::Replace, noFiles);

    QCOMPARE(names, QStringList({"movie.en.srt", "movie.ru.ass", "movie.en.forced.srt", "movie.de.sdh.vtt",
                                 "movie.und.srt"}));
}

void ExportNamingTest::duplicatesInBatch() {
    const QStringList names =
        exportFileNames("movie", {track("subrip", "eng"), track("subrip", "en"), track("subrip", "eng")},
                        ConflictPolicy::Replace, noFiles);

    QCOMPARE(names, QStringList({"movie.en.srt", "movie.en.2.srt", "movie.en.3.srt"}));
}

void ExportNamingTest::replaceIgnoresExistingFiles() {
    const QStringList names = exportFileNames("movie", {track("subrip", "eng")}, ConflictPolicy::Replace,
                                              [](const QString &) { return true; });

    QCOMPARE(names, QStringList({"movie.en.srt"}));
}

void ExportNamingTest::keepBothSkipsExistingFiles() {
    const QStringList existing = {"movie.en.srt", "movie.en.2.srt"};
    const QStringList names = exportFileNames(
        "movie", {track("subrip", "eng"), track("subrip", "eng"), track("subrip", "ukr")}, ConflictPolicy::KeepBoth,
        [&existing](const QString &name) { return existing.contains(name); });

    QCOMPARE(names, QStringList({"movie.en.3.srt", "movie.en.4.srt", "movie.uk.srt"}));
}

QTEST_GUILESS_MAIN(ExportNamingTest)

#include "tst_exportnaming.moc"
