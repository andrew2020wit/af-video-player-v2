#include "Languages.h"

#include <QTest>

using namespace afvp;

class LanguagesTest : public QObject {
    Q_OBJECT

private slots:
    void normalize_data();
    void normalize();
    void same();
    void label();
    void tableIsConsistent();
};

void LanguagesTest::normalize_data() {
    QTest::addColumn<QString>("tag");
    QTest::addColumn<QString>("code");

    QTest::newRow("639-1") << "en" << "en";
    QTest::newRow("639-2") << "eng" << "en";
    QTest::newRow("639-2/B") << "ger" << "de";
    QTest::newRow("639-2/T") << "deu" << "de";
    QTest::newRow("upper case") << "UKR" << "uk";
    QTest::newRow("region") << "pt-BR" << "pt";
    QTest::newRow("underscore region") << "zh_TW" << "zh";
    QTest::newRow("english name") << "Russian" << "ru";
    QTest::newRow("unknown") << "XYZ" << "xyz";
    QTest::newRow("undetermined") << "und" << "und";
    QTest::newRow("empty") << "" << "";
}

void LanguagesTest::normalize() {
    QFETCH(QString, tag);
    QFETCH(QString, code);

    QCOMPARE(normalizeLanguageCode(tag), code);
}

void LanguagesTest::same() {
    QVERIFY(sameLanguage("en", "eng"));
    QVERIFY(sameLanguage("fre", "fra"));
    QVERIFY(sameLanguage("fr", "FRE"));
    QVERIFY(!sameLanguage("en", "uk"));
    QVERIFY(!sameLanguage("", ""));
    QVERIFY(!sameLanguage("en", ""));
}

void LanguagesTest::label() {
    QCOMPARE(languageLabel("eng"), QStringLiteral("English — en"));
    QCOMPARE(languageLabel("uk"), QStringLiteral("українська — uk"));
    QCOMPARE(languageLabel("xyz"), QStringLiteral("xyz"));
}

void LanguagesTest::tableIsConsistent() {
    const QList<Language> &languages = allLanguages();

    QVERIFY(languages.size() > 180);

    for (const Language &language : languages) {
        QCOMPARE(language.code.size(), 2);
        QCOMPARE(language.bibliographic.size(), 3);
        QCOMPARE(language.terminological.size(), 3);
        QVERIFY(!language.englishName.isEmpty());
        QVERIFY(!language.nativeName.isEmpty());
        QCOMPARE(findLanguage(language.code), &language);
    }
}

QTEST_GUILESS_MAIN(LanguagesTest)

#include "tst_languages.moc"
