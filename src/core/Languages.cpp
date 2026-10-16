#include "Languages.h"

#include <QHash>
#include <QRegularExpression>

#include <iterator>

namespace afvp {

namespace {

struct LanguageRow {
    const char16_t *code;
    const char16_t *bibliographic;
    const char16_t *terminological;
    const char16_t *englishName;
    const char16_t *nativeName;
};

// ISO 639-1 languages.
constexpr LanguageRow languageRows[] = {
    {u"aa", u"aar", u"aar", u"Afar", u"Qafaraf"},
    {u"ab", u"abk", u"abk", u"Abkhazian", u"аԥсшәа"},
    {u"ae", u"ave", u"ave", u"Avestan", u"avesta"},
    {u"af", u"afr", u"afr", u"Afrikaans", u"Afrikaans"},
    {u"ak", u"aka", u"aka", u"Akan", u"Akan"},
    {u"am", u"amh", u"amh", u"Amharic", u"አማርኛ"},
    {u"an", u"arg", u"arg", u"Aragonese", u"aragonés"},
    {u"ar", u"ara", u"ara", u"Arabic", u"العربية"},
    {u"as", u"asm", u"asm", u"Assamese", u"অসমীয়া"},
    {u"av", u"ava", u"ava", u"Avaric", u"авар мацӀ"},
    {u"ay", u"aym", u"aym", u"Aymara", u"aymar aru"},
    {u"az", u"aze", u"aze", u"Azerbaijani", u"azərbaycan dili"},
    {u"ba", u"bak", u"bak", u"Bashkir", u"башҡорт теле"},
    {u"be", u"bel", u"bel", u"Belarusian", u"беларуская"},
    {u"bg", u"bul", u"bul", u"Bulgarian", u"български"},
    {u"bi", u"bis", u"bis", u"Bislama", u"Bislama"},
    {u"bm", u"bam", u"bam", u"Bambara", u"bamanankan"},
    {u"bn", u"ben", u"ben", u"Bengali", u"বাংলা"},
    {u"bo", u"tib", u"bod", u"Tibetan", u"བོད་ཡིག"},
    {u"br", u"bre", u"bre", u"Breton", u"brezhoneg"},
    {u"bs", u"bos", u"bos", u"Bosnian", u"bosanski"},
    {u"ca", u"cat", u"cat", u"Catalan", u"català"},
    {u"ce", u"che", u"che", u"Chechen", u"нохчийн мотт"},
    {u"ch", u"cha", u"cha", u"Chamorro", u"Chamoru"},
    {u"co", u"cos", u"cos", u"Corsican", u"corsu"},
    {u"cr", u"cre", u"cre", u"Cree", u"ᓀᐦᐃᔭᐍᐏᐣ"},
    {u"cs", u"cze", u"ces", u"Czech", u"čeština"},
    {u"cu", u"chu", u"chu", u"Church Slavic", u"ѩзыкъ словѣньскъ"},
    {u"cv", u"chv", u"chv", u"Chuvash", u"чӑваш чӗлхи"},
    {u"cy", u"wel", u"cym", u"Welsh", u"Cymraeg"},
    {u"da", u"dan", u"dan", u"Danish", u"dansk"},
    {u"de", u"ger", u"deu", u"German", u"Deutsch"},
    {u"dv", u"div", u"div", u"Divehi", u"ދިވެހި"},
    {u"dz", u"dzo", u"dzo", u"Dzongkha", u"རྫོང་ཁ"},
    {u"ee", u"ewe", u"ewe", u"Ewe", u"Eʋegbe"},
    {u"el", u"gre", u"ell", u"Greek", u"Ελληνικά"},
    {u"en", u"eng", u"eng", u"English", u"English"},
    {u"eo", u"epo", u"epo", u"Esperanto", u"Esperanto"},
    {u"es", u"spa", u"spa", u"Spanish", u"español"},
    {u"et", u"est", u"est", u"Estonian", u"eesti"},
    {u"eu", u"baq", u"eus", u"Basque", u"euskara"},
    {u"fa", u"per", u"fas", u"Persian", u"فارسی"},
    {u"ff", u"ful", u"ful", u"Fulah", u"Fulfulde"},
    {u"fi", u"fin", u"fin", u"Finnish", u"suomi"},
    {u"fj", u"fij", u"fij", u"Fijian", u"vosa Vakaviti"},
    {u"fo", u"fao", u"fao", u"Faroese", u"føroyskt"},
    {u"fr", u"fre", u"fra", u"French", u"français"},
    {u"fy", u"fry", u"fry", u"Western Frisian", u"Frysk"},
    {u"ga", u"gle", u"gle", u"Irish", u"Gaeilge"},
    {u"gd", u"gla", u"gla", u"Scottish Gaelic", u"Gàidhlig"},
    {u"gl", u"glg", u"glg", u"Galician", u"galego"},
    {u"gn", u"grn", u"grn", u"Guarani", u"Avañe'ẽ"},
    {u"gu", u"guj", u"guj", u"Gujarati", u"ગુજરાતી"},
    {u"gv", u"glv", u"glv", u"Manx", u"Gaelg"},
    {u"ha", u"hau", u"hau", u"Hausa", u"Hausa"},
    {u"he", u"heb", u"heb", u"Hebrew", u"עברית"},
    {u"hi", u"hin", u"hin", u"Hindi", u"हिन्दी"},
    {u"ho", u"hmo", u"hmo", u"Hiri Motu", u"Hiri Motu"},
    {u"hr", u"hrv", u"hrv", u"Croatian", u"hrvatski"},
    {u"ht", u"hat", u"hat", u"Haitian", u"Kreyòl ayisyen"},
    {u"hu", u"hun", u"hun", u"Hungarian", u"magyar"},
    {u"hy", u"arm", u"hye", u"Armenian", u"Հայերեն"},
    {u"hz", u"her", u"her", u"Herero", u"Otjiherero"},
    {u"ia", u"ina", u"ina", u"Interlingua", u"Interlingua"},
    {u"id", u"ind", u"ind", u"Indonesian", u"Bahasa Indonesia"},
    {u"ie", u"ile", u"ile", u"Interlingue", u"Interlingue"},
    {u"ig", u"ibo", u"ibo", u"Igbo", u"Asụsụ Igbo"},
    {u"ii", u"iii", u"iii", u"Sichuan Yi", u"ꆈꌠꉙ"},
    {u"ik", u"ipk", u"ipk", u"Inupiaq", u"Iñupiaq"},
    {u"io", u"ido", u"ido", u"Ido", u"Ido"},
    {u"is", u"ice", u"isl", u"Icelandic", u"íslenska"},
    {u"it", u"ita", u"ita", u"Italian", u"italiano"},
    {u"iu", u"iku", u"iku", u"Inuktitut", u"ᐃᓄᒃᑎᑐᑦ"},
    {u"ja", u"jpn", u"jpn", u"Japanese", u"日本語"},
    {u"jv", u"jav", u"jav", u"Javanese", u"basa Jawa"},
    {u"ka", u"geo", u"kat", u"Georgian", u"ქართული"},
    {u"kg", u"kon", u"kon", u"Kongo", u"Kikongo"},
    {u"ki", u"kik", u"kik", u"Kikuyu", u"Gĩkũyũ"},
    {u"kj", u"kua", u"kua", u"Kuanyama", u"Kuanyama"},
    {u"kk", u"kaz", u"kaz", u"Kazakh", u"қазақ тілі"},
    {u"kl", u"kal", u"kal", u"Kalaallisut", u"kalaallisut"},
    {u"km", u"khm", u"khm", u"Khmer", u"ខ្មែរ"},
    {u"kn", u"kan", u"kan", u"Kannada", u"ಕನ್ನಡ"},
    {u"ko", u"kor", u"kor", u"Korean", u"한국어"},
    {u"kr", u"kau", u"kau", u"Kanuri", u"Kanuri"},
    {u"ks", u"kas", u"kas", u"Kashmiri", u"कश्मीरी"},
    {u"ku", u"kur", u"kur", u"Kurdish", u"Kurdî"},
    {u"kv", u"kom", u"kom", u"Komi", u"коми кыв"},
    {u"kw", u"cor", u"cor", u"Cornish", u"Kernewek"},
    {u"ky", u"kir", u"kir", u"Kyrgyz", u"кыргызча"},
    {u"la", u"lat", u"lat", u"Latin", u"latine"},
    {u"lb", u"ltz", u"ltz", u"Luxembourgish", u"Lëtzebuergesch"},
    {u"lg", u"lug", u"lug", u"Ganda", u"Luganda"},
    {u"li", u"lim", u"lim", u"Limburgish", u"Limburgs"},
    {u"ln", u"lin", u"lin", u"Lingala", u"Lingála"},
    {u"lo", u"lao", u"lao", u"Lao", u"ລາວ"},
    {u"lt", u"lit", u"lit", u"Lithuanian", u"lietuvių"},
    {u"lu", u"lub", u"lub", u"Luba-Katanga", u"Kiluba"},
    {u"lv", u"lav", u"lav", u"Latvian", u"latviešu"},
    {u"mg", u"mlg", u"mlg", u"Malagasy", u"Malagasy"},
    {u"mh", u"mah", u"mah", u"Marshallese", u"Kajin M̧ajeļ"},
    {u"mi", u"mao", u"mri", u"Maori", u"te reo Māori"},
    {u"mk", u"mac", u"mkd", u"Macedonian", u"македонски"},
    {u"ml", u"mal", u"mal", u"Malayalam", u"മലയാളം"},
    {u"mn", u"mon", u"mon", u"Mongolian", u"монгол"},
    {u"mr", u"mar", u"mar", u"Marathi", u"मराठी"},
    {u"ms", u"may", u"msa", u"Malay", u"Bahasa Melayu"},
    {u"mt", u"mlt", u"mlt", u"Maltese", u"Malti"},
    {u"my", u"bur", u"mya", u"Burmese", u"မြန်မာ"},
    {u"na", u"nau", u"nau", u"Nauru", u"Dorerin Naoero"},
    {u"nb", u"nob", u"nob", u"Norwegian Bokmål", u"norsk bokmål"},
    {u"nd", u"nde", u"nde", u"North Ndebele", u"isiNdebele"},
    {u"ne", u"nep", u"nep", u"Nepali", u"नेपाली"},
    {u"ng", u"ndo", u"ndo", u"Ndonga", u"Owambo"},
    {u"nl", u"dut", u"nld", u"Dutch", u"Nederlands"},
    {u"nn", u"nno", u"nno", u"Norwegian Nynorsk", u"norsk nynorsk"},
    {u"no", u"nor", u"nor", u"Norwegian", u"norsk"},
    {u"nr", u"nbl", u"nbl", u"South Ndebele", u"isiNdebele"},
    {u"nv", u"nav", u"nav", u"Navajo", u"Diné bizaad"},
    {u"ny", u"nya", u"nya", u"Chichewa", u"chiCheŵa"},
    {u"oc", u"oci", u"oci", u"Occitan", u"occitan"},
    {u"oj", u"oji", u"oji", u"Ojibwa", u"ᐊᓂᔑᓈᐯᒧᐎᓐ"},
    {u"om", u"orm", u"orm", u"Oromo", u"Afaan Oromoo"},
    {u"or", u"ori", u"ori", u"Oriya", u"ଓଡ଼ିଆ"},
    {u"os", u"oss", u"oss", u"Ossetian", u"ирон æвзаг"},
    {u"pa", u"pan", u"pan", u"Punjabi", u"ਪੰਜਾਬੀ"},
    {u"pi", u"pli", u"pli", u"Pali", u"पालि"},
    {u"pl", u"pol", u"pol", u"Polish", u"polski"},
    {u"ps", u"pus", u"pus", u"Pashto", u"پښتو"},
    {u"pt", u"por", u"por", u"Portuguese", u"português"},
    {u"qu", u"que", u"que", u"Quechua", u"Runa Simi"},
    {u"rm", u"roh", u"roh", u"Romansh", u"rumantsch"},
    {u"rn", u"run", u"run", u"Rundi", u"Ikirundi"},
    {u"ro", u"rum", u"ron", u"Romanian", u"română"},
    {u"ru", u"rus", u"rus", u"Russian", u"русский"},
    {u"rw", u"kin", u"kin", u"Kinyarwanda", u"Ikinyarwanda"},
    {u"sa", u"san", u"san", u"Sanskrit", u"संस्कृतम्"},
    {u"sc", u"srd", u"srd", u"Sardinian", u"sardu"},
    {u"sd", u"snd", u"snd", u"Sindhi", u"سنڌي"},
    {u"se", u"sme", u"sme", u"Northern Sami", u"davvisámegiella"},
    {u"sg", u"sag", u"sag", u"Sango", u"yângâ tî sängö"},
    {u"si", u"sin", u"sin", u"Sinhala", u"සිංහල"},
    {u"sk", u"slo", u"slk", u"Slovak", u"slovenčina"},
    {u"sl", u"slv", u"slv", u"Slovenian", u"slovenščina"},
    {u"sm", u"smo", u"smo", u"Samoan", u"gagana fa'a Samoa"},
    {u"sn", u"sna", u"sna", u"Shona", u"chiShona"},
    {u"so", u"som", u"som", u"Somali", u"Soomaaliga"},
    {u"sq", u"alb", u"sqi", u"Albanian", u"shqip"},
    {u"sr", u"srp", u"srp", u"Serbian", u"српски"},
    {u"ss", u"ssw", u"ssw", u"Swati", u"SiSwati"},
    {u"st", u"sot", u"sot", u"Southern Sotho", u"Sesotho"},
    {u"su", u"sun", u"sun", u"Sundanese", u"Basa Sunda"},
    {u"sv", u"swe", u"swe", u"Swedish", u"svenska"},
    {u"sw", u"swa", u"swa", u"Swahili", u"Kiswahili"},
    {u"ta", u"tam", u"tam", u"Tamil", u"தமிழ்"},
    {u"te", u"tel", u"tel", u"Telugu", u"తెలుగు"},
    {u"tg", u"tgk", u"tgk", u"Tajik", u"тоҷикӣ"},
    {u"th", u"tha", u"tha", u"Thai", u"ไทย"},
    {u"ti", u"tir", u"tir", u"Tigrinya", u"ትግርኛ"},
    {u"tk", u"tuk", u"tuk", u"Turkmen", u"Türkmençe"},
    {u"tl", u"tgl", u"tgl", u"Tagalog", u"Wikang Tagalog"},
    {u"tn", u"tsn", u"tsn", u"Tswana", u"Setswana"},
    {u"to", u"ton", u"ton", u"Tonga", u"faka Tonga"},
    {u"tr", u"tur", u"tur", u"Turkish", u"Türkçe"},
    {u"ts", u"tso", u"tso", u"Tsonga", u"Xitsonga"},
    {u"tt", u"tat", u"tat", u"Tatar", u"татар теле"},
    {u"tw", u"twi", u"twi", u"Twi", u"Twi"},
    {u"ty", u"tah", u"tah", u"Tahitian", u"Reo Tahiti"},
    {u"ug", u"uig", u"uig", u"Uyghur", u"ئۇيغۇرچە"},
    {u"uk", u"ukr", u"ukr", u"Ukrainian", u"українська"},
    {u"ur", u"urd", u"urd", u"Urdu", u"اردو"},
    {u"uz", u"uzb", u"uzb", u"Uzbek", u"oʻzbekcha"},
    {u"ve", u"ven", u"ven", u"Venda", u"Tshivenḓa"},
    {u"vi", u"vie", u"vie", u"Vietnamese", u"Tiếng Việt"},
    {u"vo", u"vol", u"vol", u"Volapük", u"Volapük"},
    {u"wa", u"wln", u"wln", u"Walloon", u"walon"},
    {u"wo", u"wol", u"wol", u"Wolof", u"Wolof"},
    {u"xh", u"xho", u"xho", u"Xhosa", u"isiXhosa"},
    {u"yi", u"yid", u"yid", u"Yiddish", u"ייִדיש"},
    {u"yo", u"yor", u"yor", u"Yoruba", u"Yorùbá"},
    {u"za", u"zha", u"zha", u"Zhuang", u"Saɯ cueŋƅ"},
    {u"zh", u"chi", u"zho", u"Chinese", u"中文"},
    {u"zu", u"zul", u"zul", u"Zulu", u"isiZulu"},
};

QList<Language> buildLanguages() {
    QList<Language> languages;
    languages.reserve(std::size(languageRows));

    for (const LanguageRow &row : languageRows) {
        languages.append({QString::fromUtf16(row.code), QString::fromUtf16(row.bibliographic),
                          QString::fromUtf16(row.terminological), QString::fromUtf16(row.englishName),
                          QString::fromUtf16(row.nativeName)});
    }

    return languages;
}

// Lower-cased codes and English names -> index in allLanguages().
QHash<QString, qsizetype> buildIndex() {
    QHash<QString, qsizetype> index;
    const QList<Language> &languages = allLanguages();

    for (qsizetype i = 0; i < languages.size(); ++i) {
        const Language &language = languages[i];
        index.insert(language.code, i);
        index.insert(language.bibliographic, i);
        index.insert(language.terminological, i);
        index.insert(language.englishName.toLower(), i);
    }

    return index;
}

} // namespace

const QList<Language> &allLanguages() {
    static const QList<Language> languages = buildLanguages();

    return languages;
}

const Language *findLanguage(const QString &tag) {
    static const QHash<QString, qsizetype> index = buildIndex();
    QString key = tag.trimmed().toLower();
    auto found = index.constFind(key);

    if (found == index.cend()) {
        // "en-US", "pt_BR"
        const qsizetype separator = key.indexOf(QRegularExpression(QStringLiteral("[-_]")));

        if (separator > 0) {
            key = key.left(separator);
            found = index.constFind(key);
        }
    }

    if (found == index.cend()) {
        return nullptr;
    }

    return &allLanguages()[*found];
}

QString normalizeLanguageCode(const QString &tag) {
    const Language *language = findLanguage(tag);

    if (language) {
        return language->code;
    }

    return tag.trimmed().toLower();
}

bool sameLanguage(const QString &first, const QString &second) {
    const QString firstCode = normalizeLanguageCode(first);

    return !firstCode.isEmpty() && firstCode == normalizeLanguageCode(second);
}

QString languageLabel(const QString &tag) {
    const Language *language = findLanguage(tag);

    if (!language) {
        return tag;
    }

    return language->nativeName + QStringLiteral(" — ") + language->code;
}

} // namespace afvp
