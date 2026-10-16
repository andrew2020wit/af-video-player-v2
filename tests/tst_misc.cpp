#include "AudioTracks.h"
#include "KeyMapping.h"
#include "TimeFormat.h"

#include <QTest>

using namespace afvp;

class MiscTest : public QObject {
    Q_OBJECT

private slots:
    void timeFormat_data();
    void timeFormat();
    void keyMapping();
    void audioTrackChoice();
    void audioTrackDescription();
};

void MiscTest::timeFormat_data() {
    QTest::addColumn<qint64>("timeMs");
    QTest::addColumn<QString>("text");

    QTest::newRow("zero") << qint64(0) << "00:00";
    QTest::newRow("negative") << qint64(-5000) << "00:00";
    QTest::newRow("minutes") << qint64(65999) << "01:05";
    QTest::newRow("hour") << qint64(3600000) << "1:00:00";
    QTest::newRow("hours") << qint64(7384000) << "2:03:04";
}

void MiscTest::timeFormat() {
    QFETCH(qint64, timeMs);
    QFETCH(QString, text);

    QCOMPARE(formatTime(timeMs), text);
}

void MiscTest::keyMapping() {
    QCOMPARE(physicalKey(Qt::Key_M), int(Qt::Key_M));
    QCOMPARE(physicalKey(Qt::Key_Space), int(Qt::Key_Space));
    QCOMPARE(physicalKey(u'Ь'), int(Qt::Key_M));
    QCOMPARE(physicalKey(u'ь'), int(Qt::Key_M));
    QCOMPARE(physicalKey(u'А'), int(Qt::Key_F));
    QCOMPARE(physicalKey(u'Ф'), int(Qt::Key_A));
    QCOMPARE(physicalKey(u'І'), int(Qt::Key_S));
    QCOMPARE(physicalKey(u'Б'), int(Qt::Key_Comma));
    QCOMPARE(physicalKey(u'Щ'), int(Qt::Key_O));
    QCOMPARE(physicalKey(u'Р'), int(Qt::Key_H));
}

void MiscTest::audioTrackChoice() {
    const QList<AudioTrack> tracks = {
        {1, "rus", "", "ac3", 6, true},
        {2, "eng", "", "ac3", 6, false},
        {3, "eng", "Commentary", "aac", 2, false},
    };

    QCOMPARE(chooseAudioTrack(tracks, 3, "en"), 3);
    QCOMPARE(chooseAudioTrack(tracks, 0, "en"), 2);
    QCOMPARE(chooseAudioTrack(tracks, 9, "eng"), 2);
    QCOMPARE(chooseAudioTrack(tracks, 0, "de"), 1);
    QCOMPARE(chooseAudioTrack({{1, "", "", "aac", 2, false}, {2, "", "", "aac", 2, false}}, 0, "en"), 1);
    QCOMPARE(chooseAudioTrack({}, 0, "en"), 0);
}

void MiscTest::audioTrackDescription() {
    QCOMPARE(describeAudioTrack({2, "eng", "Commentary", "ac3", 6, false}, "Track %1"),
             QStringLiteral("2. English — en · Commentary · AC3 5.1"));
    QCOMPARE(describeAudioTrack({3, "", "", "aac", 2, false}, "Track %1"), QStringLiteral("Track 3 · AAC 2.0"));
}

QTEST_GUILESS_MAIN(MiscTest)

#include "tst_misc.moc"
