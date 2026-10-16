#include "SubtitleSearch.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace afvp;

class SubtitleSearchTest : public QObject {
    Q_OBJECT

private slots:
    void parseName_data();
    void parseName();
    void findInFolders();
    void findAllInSubsForSingleVideo();
    void choose_data();
    void choose();
};

namespace {

// Creates an empty file (and its folders) in `root`.
void touch(const QTemporaryDir &root, const QString &relativePath) {
    const QString path = root.filePath(relativePath);
    QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
}

QStringList relativePaths(const QList<FoundSubtitle> &files) {
    QStringList paths;

    for (const FoundSubtitle &file : files) {
        paths.append(file.relativePath);
    }

    return paths;
}

// A found file of the video "movie" in "D:/v": the location comes from the relative path.
FoundSubtitle found(const QString &relativePath) {
    const qsizetype depth = relativePath.count(u'/');
    const SubtitleLocation location = depth == 0   ? SubtitleLocation::VideoFolder
                                      : depth == 1 ? SubtitleLocation::SubsFolder
                                                   : SubtitleLocation::VideoSubsFolder;

    return {QStringLiteral("D:/v/") + relativePath, relativePath, location,
            parseSubtitleFileName(QFileInfo(relativePath).fileName(), QStringLiteral("movie"))};
}

} // namespace

void SubtitleSearchTest::parseName_data() {
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("videoBaseName");
    QTest::addColumn<QString>("language");
    QTest::addColumn<bool>("forced");
    QTest::addColumn<bool>("sdh");

    QTest::newRow("iso 639-1") << "movie.en.srt" << "movie" << "en" << false << false;
    QTest::newRow("iso 639-2") << "movie.eng.srt" << "movie" << "en" << false << false;
    QTest::newRow("upper case") << "MOVIE.EN.SRT" << "movie" << "en" << false << false;
    QTest::newRow("forced") << "movie.en.forced.srt" << "movie" << "en" << true << false;
    QTest::newRow("sdh") << "movie.en.sdh.srt" << "movie" << "en" << false << true;
    QTest::newRow("hi is a flag") << "movie.en.hi.srt" << "movie" << "en" << false << true;
    QTest::newRow("hi alone is not Hindi") << "movie.hi.srt" << "movie" << "" << false << true;
    QTest::newRow("cc") << "movie.cc.srt" << "movie" << "" << false << true;
    QTest::newRow("export number") << "movie.en.2.srt" << "movie" << "en" << false << false;
    QTest::newRow("region") << "movie.pt-BR.srt" << "movie" << "pt" << false << false;
    QTest::newRow("no language") << "movie.srt" << "movie" << "" << false << false;
    QTest::newRow("english name") << "2_English.srt" << "movie" << "en" << false << false;
    QTest::newRow("name and flag") << "Russian Forced.srt" << "movie" << "ru" << true << false;
    QTest::newRow("short code word") << "it.srt" << "movie" << "it" << false << false;
    QTest::newRow("first language wins") << "movie.en.ru.srt" << "movie" << "en" << false << false;
    // Parts of the video name are not looked at: "Italian" is in the name.
    QTest::newRow("language in video name") << "Italian Job.ru.srt" << "Italian Job" << "ru" << false << false;
    QTest::newRow("dotted video name") << "The.Movie.2020.uk.vtt" << "The.Movie.2020" << "uk" << false << false;
    // "movie2" does not start with the video name "movie", so all parts are looked at.
    QTest::newRow("other video") << "movie2.de.srt" << "movie" << "de" << false << false;
}

void SubtitleSearchTest::parseName() {
    QFETCH(QString, fileName);
    QFETCH(QString, videoBaseName);
    QFETCH(QString, language);
    QFETCH(bool, forced);
    QFETCH(bool, sdh);

    QCOMPARE(parseSubtitleFileName(fileName, videoBaseName), (SubtitleNameInfo{language, forced, sdh}));
}

void SubtitleSearchTest::findInFolders() {
    QTemporaryDir root;
    QVERIFY(root.isValid());

    for (const QString &path : {"movie.mkv", "other.mkv", "movie.en.srt", "MOVIE.uk.VTT", "movie_ru.ass", "movie.srt",
                                "movie2.en.srt", "other.en.srt", "movie.en.txt", "Subs/movie.de.srt",
                                "Subs/other.de.srt", "Subs/movie/2_English.srt", "Subs/movie/notes.txt",
                                "Subs/other/1_English.srt", "Extras/movie.fr.srt"}) {
        touch(root, path);
    }

    const QList<FoundSubtitle> files = findSubtitles(root.filePath("movie.mkv"));

    QCOMPARE(relativePaths(files), (QStringList{"movie.en.srt", "movie.srt", "MOVIE.uk.VTT", "movie_ru.ass",
                                                "Subs/movie.de.srt", "Subs/movie/2_English.srt"}));
    QCOMPARE(files[0].path, QDir::fromNativeSeparators(root.filePath("movie.en.srt")));
    QCOMPARE(files[0].location, SubtitleLocation::VideoFolder);
    QCOMPARE(files[2].info.language, QStringLiteral("uk"));
    QCOMPARE(files[4].location, SubtitleLocation::SubsFolder);
    QCOMPARE(files[5].location, SubtitleLocation::VideoSubsFolder);
    QCOMPARE(files[5].info.language, QStringLiteral("en"));
}

void SubtitleSearchTest::findAllInSubsForSingleVideo() {
    QTemporaryDir root;
    QVERIFY(root.isValid());

    for (const QString &path : {"film.mp4", "subtitles/English.srt", "subtitles/Ukrainian.srt", "subtitles/readme.txt"}) {
        touch(root, path);
    }

    const QList<FoundSubtitle> files = findSubtitles(root.filePath("film.mp4"));

    QCOMPARE(relativePaths(files), (QStringList{"subtitles/English.srt", "subtitles/Ukrainian.srt"}));
    QCOMPARE(files[1].info.language, QStringLiteral("uk"));
}

void SubtitleSearchTest::choose_data() {
    QTest::addColumn<QStringList>("files");
    QTest::addColumn<QString>("studied");
    QTest::addColumn<QString>("native");
    QTest::addColumn<QString>("main");
    QTest::addColumn<QString>("secondary");

    QTest::newRow("both languages") << QStringList{"movie.uk.srt", "movie.en.srt", "movie.de.srt"} << "en" << "uk"
                                    << "movie.en.srt" << "movie.uk.srt";
    QTest::newRow("no flags first") << QStringList{"movie.en.forced.srt", "movie.en.sdh.srt", "Subs/movie.en.srt"}
                                    << "en" << "uk" << "Subs/movie.en.srt" << "";
    QTest::newRow("sdh before forced") << QStringList{"movie.en.forced.srt", "movie.en.sdh.srt"} << "en" << "uk"
                                       << "movie.en.sdh.srt" << "";
    QTest::newRow("video folder first") << QStringList{"Subs/movie/English.srt", "Subs/movie.en.srt", "movie.en.srt"}
                                        << "en" << "uk" << "movie.en.srt" << "";
    QTest::newRow("then by name") << QStringList{"movie.en.b.srt", "movie.en.a.srt"} << "en" << "uk"
                                  << "movie.en.a.srt" << "";
    QTest::newRow("only translation") << QStringList{"movie.uk.srt", "movie.de.srt"} << "en" << "uk"
                                      << "movie.uk.srt" << "";
    QTest::newRow("single without language") << QStringList{"movie.srt"} << "en" << "uk" << "movie.srt" << "";
    QTest::newRow("two without language") << QStringList{"movie.srt", "movie.x.srt"} << "en" << "uk" << "" << "";
    QTest::newRow("without language and translation")
        << QStringList{"movie.srt", "movie.uk.srt"} << "en" << "uk" << "movie.uk.srt" << "";
    QTest::newRow("same languages") << QStringList{"movie.en.srt", "movie.en.sdh.srt"} << "en" << "en"
                                    << "movie.en.srt" << "";
    QTest::newRow("nothing") << QStringList{} << "en" << "uk" << "" << "";
}

void SubtitleSearchTest::choose() {
    QFETCH(QStringList, files);
    QFETCH(QString, studied);
    QFETCH(QString, native);
    QFETCH(QString, main);
    QFETCH(QString, secondary);

    QList<FoundSubtitle> foundFiles;

    for (const QString &file : files) {
        foundFiles.append(found(file));
    }

    const auto fullPath = [](const QString &relativePath) {
        return relativePath.isEmpty() ? QString() : QStringLiteral("D:/v/") + relativePath;
    };

    QCOMPARE(chooseSubtitles(foundFiles, studied, native), (SubtitleChoice{fullPath(main), fullPath(secondary)}));
}

QTEST_GUILESS_MAIN(SubtitleSearchTest)

#include "tst_subtitlesearch.moc"
