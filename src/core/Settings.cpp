#include "Settings.h"

#include <algorithm>

namespace afvp {

namespace {

QString urlTemplateOrDefault(const QJsonObject &object, const QString &key, const QString &defaultValue) {
    const QString url = object.value(key).toString().trimmed();

    return Settings::isValidUrlTemplate(url) ? url : defaultValue;
}

QString stringOrDefault(const QJsonObject &object, const QString &key, const QString &defaultValue) {
    const QJsonValue value = object.value(key);

    return value.isString() ? value.toString() : defaultValue;
}

QString nonEmptyStringOrDefault(const QJsonObject &object, const QString &key, const QString &defaultValue) {
    const QString value = stringOrDefault(object, key, defaultValue).trimmed();

    return value.isEmpty() ? defaultValue : value;
}

int fontSizeOrDefault(const QJsonObject &object, const QString &key, int defaultValue) {
    return std::clamp(object.value(key).toInt(defaultValue), Settings::minimumSubtitleFontSize,
                      Settings::maximumSubtitleFontSize);
}

} // namespace

bool WindowGeometry::isValid() const {
    return width >= minimumWidth && height >= minimumHeight;
}

QJsonObject WindowGeometry::toJson() const {
    return {
        {QStringLiteral("x"), x},
        {QStringLiteral("y"), y},
        {QStringLiteral("width"), width},
        {QStringLiteral("height"), height},
        {QStringLiteral("maximized"), maximized},
        {QStringLiteral("fullScreen"), fullScreen},
    };
}

WindowGeometry WindowGeometry::fromJson(const QJsonObject &object) {
    WindowGeometry geometry;
    geometry.x = object.value(QStringLiteral("x")).toInt();
    geometry.y = object.value(QStringLiteral("y")).toInt();
    geometry.width = object.value(QStringLiteral("width")).toInt();
    geometry.height = object.value(QStringLiteral("height")).toInt();
    geometry.maximized = object.value(QStringLiteral("maximized")).toBool();
    geometry.fullScreen = object.value(QStringLiteral("fullScreen")).toBool();

    return geometry.isValid() ? geometry : WindowGeometry();
}

QString Settings::effectiveAudioLanguage() const {
    return preferredAudioLanguage.isEmpty() ? studiedLanguage : preferredAudioLanguage;
}

QJsonObject Settings::toJson() const {
    return {
        {QStringLiteral("version"), currentVersion},
        {QStringLiteral("studiedLanguage"), studiedLanguage},
        {QStringLiteral("nativeLanguage"), nativeLanguage},
        {QStringLiteral("preferredAudioLanguage"),
         preferredAudioLanguage.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(preferredAudioLanguage)},
        {QStringLiteral("dictionaryUrl"), dictionaryUrl},
        {QStringLiteral("translatorUrl"), translatorUrl},
        {QStringLiteral("mainSubtitleFontSize"), mainSubtitleFontSize},
        {QStringLiteral("secondarySubtitleFontSize"), secondarySubtitleFontSize},
        {QStringLiteral("volume"), volume},
        {QStringLiteral("muted"), muted},
        {QStringLiteral("showSubtitleOverlay"), showSubtitleOverlay},
        {QStringLiteral("mainOverlayFontSize"), mainOverlayFontSize},
        {QStringLiteral("secondaryOverlayFontSize"), secondaryOverlayFontSize},
        {QStringLiteral("historySize"), historySize},
        {QStringLiteral("restartWatched"), restartWatched},
        {QStringLiteral("lastOpenFolder"),
         lastOpenFolder.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(lastOpenFolder)},
        {QStringLiteral("window"), window.isValid() ? QJsonValue(window.toJson()) : QJsonValue(QJsonValue::Null)},
    };
}

Settings Settings::fromJson(const QJsonObject &object) {
    const Settings defaults;
    Settings settings;
    settings.studiedLanguage =
        nonEmptyStringOrDefault(object, QStringLiteral("studiedLanguage"), defaults.studiedLanguage);
    settings.nativeLanguage = nonEmptyStringOrDefault(object, QStringLiteral("nativeLanguage"), defaults.nativeLanguage);
    settings.preferredAudioLanguage =
        stringOrDefault(object, QStringLiteral("preferredAudioLanguage"), QString()).trimmed();
    settings.dictionaryUrl = urlTemplateOrDefault(object, QStringLiteral("dictionaryUrl"), defaults.dictionaryUrl);
    settings.translatorUrl = urlTemplateOrDefault(object, QStringLiteral("translatorUrl"), defaults.translatorUrl);
    settings.mainSubtitleFontSize =
        fontSizeOrDefault(object, QStringLiteral("mainSubtitleFontSize"), defaults.mainSubtitleFontSize);
    settings.secondarySubtitleFontSize =
        fontSizeOrDefault(object, QStringLiteral("secondarySubtitleFontSize"), defaults.secondarySubtitleFontSize);
    settings.volume = std::clamp(object.value(QStringLiteral("volume")).toInt(defaults.volume), 0, 100);
    settings.muted = object.value(QStringLiteral("muted")).toBool(defaults.muted);
    settings.showSubtitleOverlay =
        object.value(QStringLiteral("showSubtitleOverlay")).toBool(defaults.showSubtitleOverlay);
    settings.mainOverlayFontSize =
        fontSizeOrDefault(object, QStringLiteral("mainOverlayFontSize"), defaults.mainOverlayFontSize);
    settings.secondaryOverlayFontSize =
        fontSizeOrDefault(object, QStringLiteral("secondaryOverlayFontSize"), defaults.secondaryOverlayFontSize);
    settings.historySize = std::clamp(object.value(QStringLiteral("historySize")).toInt(defaults.historySize),
                                      minimumHistorySize, maximumHistorySize);
    settings.restartWatched = object.value(QStringLiteral("restartWatched")).toBool(defaults.restartWatched);
    settings.lastOpenFolder = stringOrDefault(object, QStringLiteral("lastOpenFolder"), QString());
    settings.window = WindowGeometry::fromJson(object.value(QStringLiteral("window")).toObject());

    return settings;
}

QString Settings::defaultDictionaryUrl() {
    return QStringLiteral("https://www.ldoceonline.com/dictionary/{text}");
}

QString Settings::defaultTranslatorUrl() {
    return QStringLiteral("https://translate.google.com/?sl={src}&tl={dst}&text={text}&op=translate");
}

bool Settings::isValidUrlTemplate(const QString &url) {
    return url.contains(QStringLiteral("{text}"));
}

} // namespace afvp
