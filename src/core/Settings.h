#pragma once

#include <QJsonObject>
#include <QString>

namespace afvp {

// The main window between runs (I-75): the geometry of the normal (not maximized) window in device-independent
// pixels and its state. Not valid until the window has been closed once.
struct WindowGeometry {
    static constexpr int minimumWidth = 640;
    static constexpr int minimumHeight = 400;

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool maximized = false;
    bool fullScreen = false;

    bool isValid() const;

    QJsonObject toJson() const;
    // An invalid size gives an invalid geometry.
    static WindowGeometry fromJson(const QJsonObject &object);

    bool operator==(const WindowGeometry &other) const = default;
};

// Contents of settings.json (SPEC 3.4).
struct Settings {
    static constexpr int currentVersion = 1;
    static constexpr int minimumHistorySize = 1;
    static constexpr int maximumHistorySize = 1000;
    static constexpr int minimumSubtitleFontSize = 10;
    static constexpr int maximumSubtitleFontSize = 72;

    QString studiedLanguage = QStringLiteral("en");
    QString nativeLanguage = QStringLiteral("uk");
    // Empty: same as the studied language (I-54).
    QString preferredAudioLanguage;
    // URL templates with {text}, {src} and {dst} placeholders: the dictionary (I-42) and the translator (I-47).
    QString dictionaryUrl = defaultDictionaryUrl();
    QString translatorUrl = defaultTranslatorUrl();
    // Font sizes of the subtitle list, in pixels.
    int mainSubtitleFontSize = 26;
    int secondarySubtitleFontSize = 20;
    int volume = 100;
    bool muted = false;
    // Subtitles over the video during playback (I-22) and their font sizes, independent of the list's.
    bool showSubtitleOverlay = false;
    int mainOverlayFontSize = 34;
    int secondaryOverlayFontSize = 30;
    int historySize = 100;
    // A watched file (I-76) opens from the beginning, not from the saved position.
    bool restartWatched = true;
    QString lastOpenFolder;
    WindowGeometry window;

    QString effectiveAudioLanguage() const;

    QJsonObject toJson() const;
    // Missing or invalid fields get default values; unknown fields are ignored.
    static Settings fromJson(const QJsonObject &object);

    static QString defaultDictionaryUrl();
    static QString defaultTranslatorUrl();
    // A URL template is valid when it contains {text}.
    static bool isValidUrlTemplate(const QString &url);

    bool operator==(const Settings &other) const = default;
};

} // namespace afvp
