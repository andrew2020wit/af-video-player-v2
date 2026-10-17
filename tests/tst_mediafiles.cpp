#include "MediaFiles.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace afvp;

class MediaFilesTest : public QObject {
    Q_OBJECT

private slots:
    void fileTypes();
    void naturalOrder_data();
    void naturalOrder();
    void fileNameOrder();
    void folderVideos();
    void nextVideo();
    void existingFolders();
};

void MediaFilesTest::fileTypes() {
    QVERIFY(isVideoFileName("movie.MKV"));
    QVERIFY(isVideoFileName("D:/a/movie.m2ts"));
    QVERIFY(!isVideoFileName("movie.srt"));
    QVERIFY(isSubtitleFileName("movie.en.SRT"));
    QVERIFY(isSubtitleFileName("movie.ass"));
    QVERIFY(!isSubtitleFileName("movie.sub"));
    QVERIFY(!isSubtitleFileName("srt"));
}

void MediaFilesTest::naturalOrder_data() {
    QTest::addColumn<QString>("first");
    QTest::addColumn<QString>("second");

    // `first` goes before `second`.
    QTest::newRow("numbers") << "e9.mkv" << "e10.mkv";
    QTest::newRow("episodes") << "s02e09.mkv" << "s02e10.mkv";
    QTest::newRow("seasons") << "s1e10.mkv" << "s2e1.mkv";
    QTest::newRow("case") << "a.mkv" << "B.mkv";
    QTest::newRow("leading zeros") << "e07.mkv" << "e8.mkv";
    QTest::newRow("big numbers") << "file99999999999999999999.mkv" << "file100000000000000000000.mkv";
    QTest::newRow("text") << "Episode 2 - Beta.mkv" << "Episode 10 - Alpha.mkv";
}

void MediaFilesTest::naturalOrder() {
    QFETCH(QString, first);
    QFETCH(QString, second);

    QVERIFY(naturalCompare(first, second) < 0);
    QVERIFY(naturalCompare(second, first) > 0);
    QCOMPARE(naturalCompare(first, first), 0);
}

void MediaFilesTest::fileNameOrder() {
    QVERIFY(compareFileNames("Movie.mkv", "Movie 2.mkv") < 0);
    QVERIFY(compareFileNames("Movie 2.mkv", "Movie.mkv") > 0);
    QVERIFY(compareFileNames("e9.mp4", "e10.avi") < 0);
    QVERIFY(compareFileNames("e1.avi", "e1.mkv") < 0);
    QCOMPARE(compareFileNames("e1.mkv", "e1.mkv"), 0);
}

void MediaFilesTest::folderVideos() {
    QTemporaryDir root;
    QVERIFY(root.isValid());
    QVERIFY(QDir(root.path()).mkpath("Extras"));

    for (const QString &name : {"s01e10.mkv", "s01e02.mp4", "s01e01.en.srt", "notes.txt", "S01E09.avi", "Extras/bonus.mkv"}) {
        QFile file(root.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
    }

    QStringList names;

    for (const QString &path : videoFilesInFolder(root.path())) {
        QVERIFY(QFileInfo(path).isAbsolute());
        names.append(QFileInfo(path).fileName());
    }

    // Subfolders are not searched.
    QCOMPARE(names, (QStringList{"s01e02.mp4", "S01E09.avi", "s01e10.mkv"}));
    QCOMPARE(videoFilesInFolder(root.filePath("missing")), QStringList());
}

void MediaFilesTest::nextVideo() {
    QTemporaryDir root;
    QVERIFY(root.isValid());

    for (const QString &name : {"s01e01.mkv", "s01e02.mp4", "s01e10.mkv", "s01e09.en.srt", "s01e03.txt", "S01E09.avi"}) {
        QFile file(root.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
    }

    const auto next = [&root](const QString &name) { return QFileInfo(nextVideoFile(root.filePath(name))).fileName(); };

    QCOMPARE(next("s01e01.mkv"), QStringLiteral("s01e02.mp4"));
    QCOMPARE(next("s01e02.mp4"), QStringLiteral("S01E09.avi"));
    QCOMPARE(next("S01E09.avi"), QStringLiteral("s01e10.mkv"));
    QCOMPARE(nextVideoFile(root.filePath("s01e10.mkv")), QString());
    // A video that is not in the folder listing (e.g. an unknown extension) still has a next file.
    QCOMPARE(next("s01e05.xyz"), QStringLiteral("S01E09.avi"));
    QCOMPARE(nextVideoFile(root.filePath("s01e02.mp4")), QDir::fromNativeSeparators(root.filePath("S01E09.avi")));
}

void MediaFilesTest::existingFolders() {
    QTemporaryDir root;
    QVERIFY(root.isValid());
    QVERIFY(QDir(root.path()).mkpath("Series/Season 1"));

    const QString path = QDir::cleanPath(root.path());
    QFile video(path + "/Series/Season 1/e01.mkv");
    QVERIFY(video.open(QIODevice::WriteOnly));

    QCOMPARE(existingFolder(path + "/Series/Season 1"), path + "/Series/Season 1");
    QCOMPARE(existingFolder(path + "/Series/Season 2"), path + "/Series");
    QCOMPARE(existingFolder(path + "/Films/Comedy/2024"), path);
    // A file is not a folder: the folder it is in.
    QCOMPARE(existingFolder(video.fileName()), path + "/Series/Season 1");
    QCOMPARE(existingFolder(QString()), QString());
}

QTEST_GUILESS_MAIN(MediaFilesTest)

#include "tst_mediafiles.moc"
