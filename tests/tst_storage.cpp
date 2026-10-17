#include "History.h"
#include "JsonFile.h"
#include "Settings.h"

#include <QFile>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QTest>

using namespace afvp;

class StorageTest : public QObject {
    Q_OBJECT

private slots:
    void settingsDefaults();
    void settingsRoundTrip();
    void settingsInvalidValues();
    void historyTouchOrdersByRecency();
    void historyKeepsDataOnTouch();
    void historyTrim();
    void historyNewFileAtSamePath();
    void historyFindsMovedFile();
    void historyMovedFileSubtitles();
    void historyMovedFileLosesSubtitles();
    void historyIgnoresCopiesAndOtherSizes();
    void historyRoundTrip();
    void historyIgnoresBrokenEntries();
    void jsonFileRoundTrip();
    void jsonFileBroken();
};

void StorageTest::settingsDefaults() {
    const Settings settings = Settings::fromJson({});

    QCOMPARE(settings, Settings());
    QCOMPARE(settings.historySize, 100);
    QCOMPARE(settings.effectiveAudioLanguage(), QStringLiteral("en"));
    QVERIFY(settings.dictionaryUrl.contains("{text}"));
    QVERIFY(settings.translatorUrl.contains("{text}"));
}

void StorageTest::settingsRoundTrip() {
    Settings settings;
    settings.studiedLanguage = "de";
    settings.nativeLanguage = "ru";
    settings.preferredAudioLanguage = "en";
    settings.dictionaryUrl = "https://a/{text}";
    settings.translatorUrl = "https://t/{src}/{dst}/{text}";
    settings.mainSubtitleFontSize = 32;
    settings.secondarySubtitleFontSize = 18;
    settings.volume = 42;
    settings.muted = true;
    settings.showSubtitleOverlay = true;
    settings.mainOverlayFontSize = 40;
    settings.secondaryOverlayFontSize = 24;
    settings.historySize = 7;
    settings.restartWatched = false;
    settings.lastOpenFolder = "D:/Films";
    settings.window = {-1200, 40, 1000, 700, true, false};

    const QJsonObject json = settings.toJson();

    QCOMPARE(json.value("version").toInt(), Settings::currentVersion);
    QCOMPARE(Settings::fromJson(json), settings);
    QCOMPARE(Settings::fromJson(Settings().toJson()).preferredAudioLanguage, QString());
    QVERIFY(Settings().toJson().value("preferredAudioLanguage").isNull());
    QVERIFY(Settings().toJson().value("window").isNull());
}

void StorageTest::settingsInvalidValues() {
    const QJsonObject json{
        {"volume", 250},
        {"historySize", 0},
        {"mainSubtitleFontSize", 500},
        {"secondarySubtitleFontSize", "big"},
        {"mainOverlayFontSize", 1},
        {"window", QJsonObject{{"x", 10}, {"y", 10}, {"width", 100}, {"height", 700}, {"maximized", true}}},
        {"studiedLanguage", ""},
        {"dictionaryUrl", "https://x/"},
        {"translatorUrl", 42},
        {"unknownField", true},
    };
    const Settings settings = Settings::fromJson(json);

    QCOMPARE(settings.volume, 100);
    QCOMPARE(settings.historySize, Settings::minimumHistorySize);
    QCOMPARE(settings.mainSubtitleFontSize, Settings::maximumSubtitleFontSize);
    QCOMPARE(settings.secondarySubtitleFontSize, Settings().secondarySubtitleFontSize);
    QCOMPARE(settings.mainOverlayFontSize, Settings::minimumSubtitleFontSize);
    QCOMPARE(settings.secondaryOverlayFontSize, Settings().secondaryOverlayFontSize);
    QVERIFY(!settings.window.isValid());
    QCOMPARE(settings.window, WindowGeometry());
    QCOMPARE(settings.studiedLanguage, QStringLiteral("en"));
    QCOMPARE(settings.dictionaryUrl, Settings::defaultDictionaryUrl());
    QCOMPARE(settings.translatorUrl, Settings::defaultTranslatorUrl());
}

void StorageTest::historyTouchOrdersByRecency() {
    History history;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    history.touch("D:/a.mkv", 1, now);
    history.touch("D:/b.mkv", 2, now.addSecs(1));
    history.touch("D:/a.mkv", 1, now.addSecs(2));

    QCOMPARE(history.entries().size(), 2);
    QCOMPARE(history.entries()[0].path, QStringLiteral("D:/a.mkv"));
    QCOMPARE(history.entries()[1].path, QStringLiteral("D:/b.mkv"));
}

void StorageTest::historyKeepsDataOnTouch() {
    History history;
    bool isNew = false;
    HistoryEntry &entry = history.touch("D:/a.mkv", 1, QDateTime::currentDateTimeUtc(), {}, &isNew);
    QVERIFY(isNew);
    entry.positionMs = 5000;
    entry.mainSubtitles = "D:/a.en.srt";
    history.touch("D:/b.mkv", 2, QDateTime::currentDateTimeUtc());
    const HistoryEntry &touched = history.touch("D:/a.mkv", 1, QDateTime::currentDateTimeUtc(), {}, &isNew);

    QVERIFY(!isNew);
    QCOMPARE(touched.positionMs, 5000);
    QCOMPARE(touched.mainSubtitles, QStringLiteral("D:/a.en.srt"));
    QCOMPARE(touched.size, 1);
}

void StorageTest::historyTrim() {
    History history;

    for (int i = 0; i < 5; ++i) {
        history.touch(QStringLiteral("D:/%1.mkv").arg(i), i, QDateTime::currentDateTimeUtc());
    }

    history.trim(2);

    QCOMPARE(history.entries().size(), 2);
    QCOMPARE(history.entries()[0].path, QStringLiteral("D:/4.mkv"));
}

void StorageTest::historyNewFileAtSamePath() {
    History history;
    HistoryEntry &entry = history.touch("D:/a.mkv", 1, QDateTime::currentDateTimeUtc());
    entry.positionMs = 5000;
    entry.audioTrackId = 2;
    bool isNew = false;
    const HistoryEntry &touched = history.touch("D:/a.mkv", 7, QDateTime::currentDateTimeUtc(), {}, &isNew);

    QVERIFY(isNew);
    QCOMPARE(history.entries().size(), 1);
    QCOMPARE(touched.positionMs, 0);
    QCOMPARE(touched.audioTrackId, 0);
    QCOMPARE(touched.size, 7);
}

namespace {

// Files that exist for the "moved file" tests.
History::FileExists existing(const QStringList &files) {
    return [files](const QString &path) { return files.contains(path); };
}

} // namespace

void StorageTest::historyFindsMovedFile() {
    History history;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    HistoryEntry &older = history.touch("D:/Old1/movie.mkv", 5, now.addSecs(-20));
    older.positionMs = 1000;
    HistoryEntry &newer = history.touch("D:/Old2/movie.mkv", 5, now.addSecs(-10));
    newer.positionMs = 2000;
    history.touch("D:/Other/other.mkv", 5, now.addSecs(-5));
    bool isNew = true;
    const HistoryEntry &touched =
        history.touch("D:/New/movie.mkv", 5, now, existing({"D:/Other/other.mkv"}), &isNew);

    QVERIFY(!isNew);
    QCOMPARE(touched.path, QStringLiteral("D:/New/movie.mkv"));
    QCOMPARE(touched.positionMs, 2000);
    QCOMPARE(history.entries().size(), 3);
    QVERIFY(!history.find("D:/Old2/movie.mkv"));
    QVERIFY(history.find("D:/Old1/movie.mkv"));
}

void StorageTest::historyMovedFileSubtitles() {
    History history;
    HistoryEntry &entry = history.touch("D:/Old/movie.mkv", 5, QDateTime::currentDateTimeUtc());
    entry.mainSubtitles = "D:/Old/Subs/movie.en.srt";
    entry.secondarySubtitles = "E:/Shared/movie.uk.srt";
    entry.subtitlesChosen = true;
    const HistoryEntry &touched = history.touch("D:/New/movie.mkv", 5, QDateTime::currentDateTimeUtc(),
                                                existing({"D:/New/Subs/movie.en.srt", "E:/Shared/movie.uk.srt"}));

    QCOMPARE(touched.mainSubtitles, QStringLiteral("D:/New/Subs/movie.en.srt"));
    QCOMPARE(touched.secondarySubtitles, QStringLiteral("E:/Shared/movie.uk.srt"));
    QVERIFY(touched.subtitlesChosen);
}

void StorageTest::historyMovedFileLosesSubtitles() {
    History history;
    HistoryEntry &entry = history.touch("D:/Old/movie.mkv", 5, QDateTime::currentDateTimeUtc());
    entry.mainSubtitles = "D:/Old/movie.en.srt";
    entry.secondarySubtitles = "D:/Old/movie.uk.srt";
    entry.subtitlesChosen = true;
    entry.positionMs = 3000;
    const HistoryEntry &touched = history.touch("D:/New/movie.mkv", 5, QDateTime::currentDateTimeUtc(),
                                                existing({"D:/New/movie.en.srt"}));

    QCOMPARE(touched.positionMs, 3000);
    QCOMPARE(touched.mainSubtitles, QString());
    QCOMPARE(touched.secondarySubtitles, QString());
    QVERIFY(!touched.subtitlesChosen);
}

void StorageTest::historyIgnoresCopiesAndOtherSizes() {
    History history;
    HistoryEntry &copied = history.touch("D:/Old/movie.mkv", 5, QDateTime::currentDateTimeUtc());
    copied.positionMs = 1000;
    HistoryEntry &resized = history.touch("D:/Old/film.mkv", 5, QDateTime::currentDateTimeUtc());
    resized.positionMs = 1000;

    QCOMPARE(history.touch("D:/New/movie.mkv", 5, QDateTime::currentDateTimeUtc(), existing({"D:/Old/movie.mkv"}))
                 .positionMs,
             0);
    QCOMPARE(history.touch("D:/New/film.mkv", 6, QDateTime::currentDateTimeUtc(), existing({})).positionMs, 0);
    // Without a file check moved files are not looked for.
    QCOMPARE(history.touch("D:/Other/film.mkv", 5, QDateTime::currentDateTimeUtc()).positionMs, 0);
    QCOMPARE(history.entries().size(), 5);
}

void StorageTest::historyRoundTrip() {
    History history;
    const QDateTime time = QDateTime(QDate(2026, 10, 14), QTime(9, 30), QTimeZone::UTC);
    history.touch("D:/Films/other.mkv", 10, time.addSecs(-60));
    HistoryEntry &entry = history.touch("D:/Films/movie.mkv", 2361393152, time);
    entry.durationMs = 7200000;
    entry.positionMs = 1234567;
    entry.audioTrackId = 2;
    entry.mainSubtitles = "D:/Films/movie.en.srt";
    entry.secondarySubtitles = "D:/Films/movie.uk.srt";
    entry.subtitlesChosen = true;
    entry.mainOffsetMs = -1500;

    const QJsonObject json = history.toJson();
    const History restored = History::fromJson(json);

    QCOMPARE(json.value("version").toInt(), History::currentVersion);
    QCOMPARE(restored.entries(), history.entries());
    QVERIFY(json.value("files").toArray()[1].toObject().value("audioTrackId").isNull());
}

void StorageTest::historyIgnoresBrokenEntries() {
    const QJsonObject json{{"files", QJsonArray{QJsonObject{{"path", ""}}, QJsonObject{{"path", "D:/a.mkv"}},
                                                QJsonObject{{"path", "D:/a.mkv"}},
                                                QJsonObject{{"path", "D:/films/../a.mkv"}}, 42}}};
    const History history = History::fromJson(json);

    QCOMPARE(history.entries().size(), 1);
}

void StorageTest::jsonFileRoundTrip() {
    QTemporaryDir directory;
    const QString path = directory.filePath("nested/settings.json");
    const QJsonObject object{{"a", 1}};

    QVERIFY(writeJsonObject(path, object));
    QCOMPARE(readJsonObject(path), std::optional<QJsonObject>(object));
}

void StorageTest::jsonFileBroken() {
    QTemporaryDir directory;
    const QString path = directory.filePath("broken.json");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("{ not json");
    file.close();

    QVERIFY(!readJsonObject(path).has_value());
    QVERIFY(!readJsonObject(directory.filePath("missing.json")).has_value());
}

QTEST_GUILESS_MAIN(StorageTest)

#include "tst_storage.moc"
