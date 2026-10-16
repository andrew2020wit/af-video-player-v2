#include "FrameGrabber.h"
#include "SubtitleExtractor.h"
#include "SubtitleParser.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace afvp;

class ExportTest : public QObject {
    Q_OBJECT

private slots:
    void probeMkv();
    void extractMkvInOnePass();
    void convertMp4MovText();
    void cancelRemovesFiles();
    void failureKeepsExistingFile();
    void successReplacesExistingFile();
    void grabFrame_data();
    void grabFrame();
    void grabFrameWithoutVideo();
};

namespace {

QString dataPath(const QString &fileName) {
    return QStringLiteral(AFVP_TEST_DATA_DIR "/") + fileName;
}

} // namespace

void ExportTest::probeMkv() {
    QString error;
    const QList<EmbeddedSubtitleTrack> tracks = probeSubtitleTracks(dataPath("sample.mkv"), &error);

    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(tracks.size(), 3);
    QCOMPARE(tracks[0].codecName, QStringLiteral("subrip"));
    QCOMPARE(tracks[0].language, QStringLiteral("eng"));
    QCOMPARE(tracks[1].codecName, QStringLiteral("ass"));
    QCOMPARE(tracks[1].title, QStringLiteral("Styled"));
    QVERIFY(tracks[1].forced);
    QCOMPARE(tracks[2].codecName, QStringLiteral("webvtt"));
    QCOMPARE(tracks[2].language, QStringLiteral("ukr"));

    for (const EmbeddedSubtitleTrack &track : tracks) {
        QVERIFY(track.isText);
    }
}

void ExportTest::extractMkvInOnePass() {
    QTemporaryDir directory;
    const QList<ExtractionTarget> targets = {
        {1, directory.filePath("movie.en.srt")},
        {2, directory.filePath("movie.en.forced.ass")},
        {3, directory.filePath("movie.uk.vtt")},
    };
    std::atomic_bool cancel = false;
    double lastProgress = 0;
    QString error;

    QVERIFY2(extractSubtitleTracks(
                 dataPath("sample.mkv"), targets, [&lastProgress](double value) { lastProgress = value; }, cancel,
                 &error),
             qPrintable(error));
    QCOMPARE(lastProgress, 1.0);

    const SubtitleLoadResult srt = loadSubtitleFile(targets[0].outputPath);
    QVERIFY2(srt.ok, qPrintable(srt.error));
    QCOMPARE(srt.cues.size(), 3);
    QCOMPARE(srt.cues[0], (SubtitleCue{1000, 2500, QStringLiteral("Where are you going?")}));

    const SubtitleLoadResult ass = loadSubtitleFile(targets[1].outputPath);
    QVERIFY2(ass.ok, qPrintable(ass.error));
    QCOMPARE(ass.cues.size(), 2);
    QCOMPARE(ass.cues[1].text, QStringLiteral("- Home.\n- Already?"));

    const SubtitleLoadResult vtt = loadSubtitleFile(targets[2].outputPath);
    QVERIFY2(vtt.ok, qPrintable(vtt.error));
    QCOMPARE(vtt.cues.size(), 3);
    QCOMPARE(vtt.cues[0].text, QStringLiteral("Куди ти йдеш?"));
}

void ExportTest::convertMp4MovText() {
    QTemporaryDir directory;
    const QList<EmbeddedSubtitleTrack> tracks = probeSubtitleTracks(dataPath("sample.mp4"));

    QCOMPARE(tracks.size(), 1);
    QCOMPARE(tracks[0].codecName, QStringLiteral("mov_text"));

    const QString outputPath = directory.filePath("movie.en.srt");
    std::atomic_bool cancel = false;
    QString error;

    QVERIFY2(extractSubtitleTracks(dataPath("sample.mp4"), {{tracks[0].streamIndex, outputPath}}, {}, cancel, &error),
             qPrintable(error));

    const SubtitleLoadResult srt = loadSubtitleFile(outputPath);
    QVERIFY2(srt.ok, qPrintable(srt.error));
    QCOMPARE(srt.cues.size(), 3);
    QCOMPARE(srt.cues[0], (SubtitleCue{1000, 2500, QStringLiteral("Where are you going?")}));
    QCOMPARE(srt.cues[1].text, QStringLiteral("- Home.\n- Already?"));
}

void ExportTest::cancelRemovesFiles() {
    QTemporaryDir directory;
    const QString outputPath = directory.filePath("movie.en.srt");
    std::atomic_bool cancel = true;
    QString error;

    QVERIFY(!extractSubtitleTracks(dataPath("sample.mkv"), {{1, outputPath}}, {}, cancel, &error));
    QVERIFY(!QFile::exists(outputPath));
}

void ExportTest::failureKeepsExistingFile() {
    QTemporaryDir directory;
    const QString outputPath = directory.filePath("movie.en.srt");
    QFile existing(outputPath);
    QVERIFY(existing.open(QIODevice::WriteOnly));
    existing.write("old");
    existing.close();
    std::atomic_bool cancel = false;
    QString error;

    // The first output is opened before the invalid second target fails the export.
    QVERIFY(!extractSubtitleTracks(dataPath("sample.mkv"), {{1, outputPath}, {99, directory.filePath("other.srt")}},
                                   {}, cancel, &error));
    QVERIFY(existing.open(QIODevice::ReadOnly));
    QCOMPARE(existing.readAll(), QByteArray("old"));
    QCOMPARE(QDir(directory.path()).entryList(QDir::Files), QStringList{"movie.en.srt"});
}

void ExportTest::successReplacesExistingFile() {
    QTemporaryDir directory;
    const QString outputPath = directory.filePath("movie.en.srt");
    QFile existing(outputPath);
    QVERIFY(existing.open(QIODevice::WriteOnly));
    existing.write("old");
    existing.close();
    std::atomic_bool cancel = false;
    QString error;

    QVERIFY2(extractSubtitleTracks(dataPath("sample.mkv"), {{1, outputPath}}, {}, cancel, &error), qPrintable(error));

    const SubtitleLoadResult srt = loadSubtitleFile(outputPath);
    QVERIFY2(srt.ok, qPrintable(srt.error));
    QCOMPARE(srt.cues.size(), 3);
    QCOMPARE(QDir(directory.path()).entryList(QDir::Files), QStringList{"movie.en.srt"});
}

void ExportTest::grabFrame_data() {
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<double>("seconds");

    QTest::newRow("mkv start") << QStringLiteral("sample.mkv") << 0.0;
    QTest::newRow("mkv middle") << QStringLiteral("sample.mkv") << 5.5;
    QTest::newRow("mkv after end") << QStringLiteral("sample.mkv") << 100.0;
    QTest::newRow("mp4 middle") << QStringLiteral("sample.mp4") << 7.0;
}

void ExportTest::grabFrame() {
    QFETCH(QString, fileName);
    QFETCH(double, seconds);
    FrameGrabber grabber(dataPath(fileName));

    // The samples are 64x48: the height follows the aspect ratio. A second grab reuses the open file.
    for (int i = 0; i < 2; ++i) {
        const VideoFrame frame = grabber.grab(seconds, 160);

        QVERIFY(!frame.isNull());
        QCOMPARE(frame.width, 160);
        QCOMPARE(frame.height, 120);
        QCOMPARE(frame.pixels.size(), 160 * 120 * 4);
    }
}

void ExportTest::grabFrameWithoutVideo() {
    QVERIFY(FrameGrabber(dataPath("sample.en.srt")).grab(1.0, 160).isNull());
    QVERIFY(FrameGrabber(dataPath("missing.mkv")).grab(1.0, 160).isNull());
}

QTEST_GUILESS_MAIN(ExportTest)

#include "tst_export.moc"
