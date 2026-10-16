#include "HistoryGroups.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace afvp;

class HistoryGroupsTest : public QObject {
    Q_OBJECT

private slots:
    void watched_data();
    void watched();
    void groupsByFolderInRecencyOrder();
    void missingFilesAreSkipped();
    void continuePath_data();
    void continuePath();
    void singleVideoIsNotSeries();
    void videoFilesOnDisk();
};

namespace {

// An absolute path on any OS: "C:/<relative>" on Windows, "/<relative>" elsewhere.
QString p(const char *relative) {
    return QDir::rootPath() + QLatin1String(relative);
}

const QString series = p("Series");
const QStringList seriesFiles = {p("Series/e10.mkv"), p("Series/e2.mkv"), p("Series/e1.mkv"), p("Series/e3.mkv")};

// percent < 0: never played to the end of the file (no duration).
HistoryEntry entry(const QString &path, int percent) {
    HistoryEntry result;
    result.path = path;
    result.durationMs = percent < 0 ? 0 : 100000;
    result.positionMs = percent < 0 ? 0 : percent * 1000;

    return result;
}

QList<HistoryGroup> group(const QList<HistoryEntry> &entries) {
    return groupHistory(
        entries, [](const QString &path) { return !path.contains("missing"); },
        [](const QString &folder) { return folder == series ? seriesFiles : QStringList{folder + "/movie.mkv"}; });
}

} // namespace

void HistoryGroupsTest::watched_data() {
    QTest::addColumn<int>("percent");
    QTest::addColumn<bool>("watched");

    QTest::newRow("no duration") << -1 << false;
    QTest::newRow("half") << 50 << false;
    QTest::newRow("94%") << 94 << false;
    QTest::newRow("95%") << 95 << true;
    QTest::newRow("end") << 100 << true;
}

void HistoryGroupsTest::watched() {
    QFETCH(int, percent);
    QFETCH(bool, watched);

    QCOMPARE(isWatched(entry(p("a.mkv"), percent)), watched);
}

void HistoryGroupsTest::groupsByFolderInRecencyOrder() {
    const QList<HistoryGroup> groups =
        group({entry(p("Series/e2.mkv"), 30), entry(p("Film/movie.mkv"), 10), entry(p("Series/e1.mkv"), 100)});

    QCOMPARE(groups.size(), 2);
    QCOMPARE(groups[0].folder, series);
    QCOMPARE(groups[0].entries.size(), 2);
    QCOMPARE(groups[0].entries[0].path, p("Series/e2.mkv"));
    QCOMPARE(groups[0].videoFiles,
             QStringList({p("Series/e1.mkv"), p("Series/e2.mkv"), p("Series/e3.mkv"), p("Series/e10.mkv")}));
    QCOMPARE(groups[0].watchedCount, 1);
    QVERIFY(groups[0].isSeries());
    QCOMPARE(groups[1].folder, p("Film"));
}

void HistoryGroupsTest::missingFilesAreSkipped() {
    const QList<HistoryGroup> groups = group({entry(p("Old/missing.mkv"), 10), entry(p("Series/e1.mkv"), 10)});

    QCOMPARE(groups.size(), 1);
    QCOMPARE(groups[0].folder, series);
}

void HistoryGroupsTest::continuePath_data() {
    QTest::addColumn<QList<HistoryEntry>>("entries");
    QTest::addColumn<QString>("continuePath");

    QTest::newRow("latest not watched") << QList<HistoryEntry>{entry(p("Series/e2.mkv"), 40), entry(p("Series/e1.mkv"), 100)}
                                        << p("Series/e2.mkv");
    QTest::newRow("latest watched: next") << QList<HistoryEntry>{entry(p("Series/e2.mkv"), 97)}
                                          << p("Series/e3.mkv");
    QTest::newRow("next watched is skipped")
        << QList<HistoryEntry>{entry(p("Series/e2.mkv"), 100), entry(p("Series/e3.mkv"), 100)}
        << p("Series/e10.mkv");
    QTest::newRow("last watched") << QList<HistoryEntry>{entry(p("Series/e10.mkv"), 100)} << QString();
    QTest::newRow("never played") << QList<HistoryEntry>{entry(p("Series/e3.mkv"), -1)}
                                  << p("Series/e3.mkv");
}

void HistoryGroupsTest::continuePath() {
    QFETCH(QList<HistoryEntry>, entries);
    QFETCH(QString, continuePath);

    QCOMPARE(group(entries).front().continuePath, continuePath);
}

void HistoryGroupsTest::singleVideoIsNotSeries() {
    QVERIFY(!group({entry(p("Film/movie.mkv"), 100)}).front().isSeries());
}

void HistoryGroupsTest::videoFilesOnDisk() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    for (const QString name : {"e1.mkv", "e2.MP4", "e1.srt", "notes.txt"}) {
        QFile file(dir.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
    }

    QStringList files = videoFilesInFolder(dir.path());
    files.sort();

    QCOMPARE(files, QStringList({dir.filePath("e1.mkv"), dir.filePath("e2.MP4")}));
}

QTEST_GUILESS_MAIN(HistoryGroupsTest)

#include "tst_historygroups.moc"
