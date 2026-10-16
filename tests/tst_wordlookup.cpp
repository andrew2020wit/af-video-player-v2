#include "WordLookup.h"

#include <QTest>

using namespace afvp;

class WordLookupTest : public QObject {
    Q_OBJECT

private slots:
    void wordAtPosition_data();
    void wordAtPosition();
    void selection_data();
    void selection();
    void urlTemplate();
};

void WordLookupTest::wordAtPosition_data() {
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("position");
    QTest::addColumn<QString>("word");

    QTest::newRow("start of word") << "Hello world" << 0 << "Hello";
    QTest::newRow("middle of word") << "Hello world" << 8 << "world";
    QTest::newRow("end of word") << "Hello world" << 5 << "Hello";
    QTest::newRow("end of text") << "Hello world" << 11 << "world";
    QTest::newRow("on separator") << "Hello -- world" << 7 << "";
    QTest::newRow("empty") << "" << 0 << "";
    QTest::newRow("apostrophe inside") << "I don't know" << 4 << "don't";
    QTest::newRow("typographic apostrophe") << "I don’t know" << 6 << "don’t";
    QTest::newRow("possessive") << "John's car" << 1 << "John";
    QTest::newRow("possessive on s") << "the teacher's book" << 12 << "teacher";
    QTest::newRow("it's") << "it's late" << 0 << "it";
    QTest::newRow("o'clock") << "at five o'clock." << 9 << "o'clock";
    QTest::newRow("quote is not apostrophe") << "'Hello' he said" << 1 << "Hello";
    QTest::newRow("hyphen splits") << "a well-known fact" << 3 << "well";
    QTest::newRow("hyphen splits right") << "a well-known fact" << 9 << "known";
    QTest::newRow("cyrillic") << "«Привет», — сказал он…" << 3 << "Привет";
    QTest::newRow("closing quote") << "“Yes” and no" << 2 << "Yes";
    QTest::newRow("ellipsis") << "Wait…what" << 6 << "what";
    QTest::newRow("en dash") << "now–then" << 1 << "now";
    QTest::newRow("digits") << "in 1984 we" << 4 << "1984";
    QTest::newRow("combining mark") << "café au lait" << 2 << "café";
}

void WordLookupTest::wordAtPosition() {
    QFETCH(QString, text);
    QFETCH(int, position);
    QFETCH(QString, word);

    QCOMPARE(wordAt(text, position), word);
}

void WordLookupTest::selection_data() {
    QTest::addColumn<QString>("selection");
    QTest::addColumn<QString>("cleaned");

    QTest::newRow("plain") << "look up" << "look up";
    QTest::newRow("punctuation around") << " (give up!) " << "give up";
    QTest::newRow("line break") << "turn\nout" << "turn out";
    QTest::newRow("quotes") << "“well-known”," << "well-known";
    QTest::newRow("only punctuation") << " -- " << "";
}

void WordLookupTest::selection() {
    QFETCH(QString, selection);
    QFETCH(QString, cleaned);

    QCOMPARE(cleanSelection(selection), cleaned);
}

void WordLookupTest::urlTemplate() {
    QCOMPARE(fillUrlTemplate("https://www.ldoceonline.com/dictionary/{text}", "give up", "en", "uk"),
             QStringLiteral("https://www.ldoceonline.com/dictionary/give%20up"));
    QCOMPARE(fillUrlTemplate("https://t.example/?sl={src}&tl={dst}&text={text}", "a&b=c/d?", "en", "uk"),
             QStringLiteral("https://t.example/?sl=en&tl=uk&text=a%26b%3Dc%2Fd%3F"));
    QCOMPARE(fillUrlTemplate("https://d.example/{text}", "ёж", "ru", "uk"),
             QStringLiteral("https://d.example/%D1%91%D0%B6"));
}

QTEST_GUILESS_MAIN(WordLookupTest)

#include "tst_wordlookup.moc"
