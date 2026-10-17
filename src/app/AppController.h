#pragma once

#include "AutoExport.h"
#include "BatchExporter.h"
#include "ExportNaming.h"
#include "History.h"
#include "Settings.h"
#include "SubtitleCue.h"
#include "SubtitleExtractor.h"
#include "SubtitleListModel.h"
#include "SubtitleSearch.h"

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <atomic>
#include <optional>
#include <thread>

class QDir;

// Application state for QML: settings, history, the open video and its subtitles, export of embedded subtitles.
class AppController : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(App)
    QML_SINGLETON

    // Settings (I-50); every change is saved at once.
    Q_PROPERTY(QString studiedLanguage READ studiedLanguage WRITE setStudiedLanguage NOTIFY settingsChanged)
    Q_PROPERTY(QString nativeLanguage READ nativeLanguage WRITE setNativeLanguage NOTIFY settingsChanged)
    Q_PROPERTY(QString preferredAudioLanguage READ preferredAudioLanguage WRITE setPreferredAudioLanguage
                   NOTIFY settingsChanged)
    Q_PROPERTY(QString dictionaryUrl READ dictionaryUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString translatorUrl READ translatorUrl NOTIFY settingsChanged)
    Q_PROPERTY(int mainSubtitleFontSize READ mainSubtitleFontSize WRITE setMainSubtitleFontSize
                   NOTIFY settingsChanged)
    Q_PROPERTY(int secondarySubtitleFontSize READ secondarySubtitleFontSize WRITE setSecondarySubtitleFontSize
                   NOTIFY settingsChanged)
    Q_PROPERTY(bool showSubtitleOverlay READ showSubtitleOverlay WRITE setShowSubtitleOverlay NOTIFY settingsChanged)
    Q_PROPERTY(int mainOverlayFontSize READ mainOverlayFontSize WRITE setMainOverlayFontSize NOTIFY settingsChanged)
    Q_PROPERTY(int secondaryOverlayFontSize READ secondaryOverlayFontSize WRITE setSecondaryOverlayFontSize
                   NOTIFY settingsChanged)
    Q_PROPERTY(int minimumSubtitleFontSize READ minimumSubtitleFontSize CONSTANT)
    Q_PROPERTY(int maximumSubtitleFontSize READ maximumSubtitleFontSize CONSTANT)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY settingsChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY settingsChanged)
    Q_PROPERTY(int historySize READ historySize WRITE setHistorySize NOTIFY settingsChanged)
    Q_PROPERTY(bool restartWatched READ restartWatched WRITE setRestartWatched NOTIFY settingsChanged)
    // The file open dialog (I-86): the folder of the last opened video or its nearest existing parent, and the
    // video itself when it still exists (empty otherwise).
    Q_PROPERTY(QString openFolderUrl READ openFolderUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString openFileUrl READ openFileUrl NOTIFY settingsChanged)

    // History by folder (I-76): [{folder, folderName, isSeries, watchedCount, videoCount, continuePath,
    // continueName, lastOpened, entries: [{path, name, folder, progress, watched, lastOpened}]}].
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)

    Q_PROPERTY(QString videoPath READ videoPath NOTIFY videoChanged)
    Q_PROPERTY(QString videoName READ videoName NOTIFY videoChanged)
    Q_PROPERTY(QString videoFolderUrl READ videoFolderUrl NOTIFY videoChanged)
    Q_PROPERTY(int embeddedTextTrackCount READ embeddedTextTrackCount NOTIFY videoChanged)
    Q_PROPERTY(QString mainSubtitlesPath READ mainSubtitlesPath NOTIFY subtitlesChanged)
    Q_PROPERTY(QString secondarySubtitlesPath READ secondarySubtitlesPath NOTIFY subtitlesChanged)
    // The current subtitles were chosen by the automatic search (I-06), not by the user.
    Q_PROPERTY(bool subtitlesAutomatic READ subtitlesAutomatic NOTIFY subtitlesChanged)
    // Subtitle files found next to the video (I-06): [{path, relativePath, description}].
    Q_PROPERTY(QVariantList foundSubtitles READ foundSubtitles NOTIFY subtitlesChanged)
    Q_PROPERTY(SubtitleListModel *subtitles READ subtitles CONSTANT)

    Q_PROPERTY(bool exportRunning READ exportRunning NOTIFY exportStateChanged)
    Q_PROPERTY(double exportProgress READ exportProgress NOTIFY exportStateChanged)
    // Batch export of a folder (I-87).
    Q_PROPERTY(BatchExporter *batchExport READ batchExport CONSTANT)

    Q_PROPERTY(QVariantList languages READ languages CONSTANT)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString homepageUrl READ homepageUrl CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(QString ffmpegVersion READ ffmpegVersion CONSTANT)

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    QString studiedLanguage() const;
    void setStudiedLanguage(const QString &code);
    QString nativeLanguage() const;
    void setNativeLanguage(const QString &code);
    QString preferredAudioLanguage() const;
    void setPreferredAudioLanguage(const QString &code);
    QString dictionaryUrl() const;
    QString translatorUrl() const;
    int mainSubtitleFontSize() const;
    void setMainSubtitleFontSize(int size);
    int secondarySubtitleFontSize() const;
    void setSecondarySubtitleFontSize(int size);
    bool showSubtitleOverlay() const;
    void setShowSubtitleOverlay(bool show);
    int mainOverlayFontSize() const;
    void setMainOverlayFontSize(int size);
    int secondaryOverlayFontSize() const;
    void setSecondaryOverlayFontSize(int size);
    int minimumSubtitleFontSize() const;
    int maximumSubtitleFontSize() const;
    int volume() const;
    void setVolume(int volume);
    bool muted() const;
    void setMuted(bool muted);
    int historySize() const;
    void setHistorySize(int size);
    bool restartWatched() const;
    void setRestartWatched(bool restart);
    QString openFolderUrl() const;
    QString openFileUrl() const;

    QVariantList history() const;

    QString videoPath() const;
    QString videoName() const;
    QString videoFolderUrl() const;
    int embeddedTextTrackCount() const;
    QString mainSubtitlesPath() const;
    QString secondarySubtitlesPath() const;
    bool subtitlesAutomatic() const;
    QVariantList foundSubtitles() const;
    SubtitleListModel *subtitles();

    bool exportRunning() const;
    double exportProgress() const;
    BatchExporter *batchExport();

    QVariantList languages() const;
    QString appVersion() const;
    // The project page with the source code (I-56).
    QString homepageUrl() const;
    QString qtVersion() const;
    QString ffmpegVersion() const;

    // URL templates of the dictionary and the translator. A template is saved only when it is valid (has {text}, no
    // whitespace); leading and trailing whitespace is ignored.
    Q_INVOKABLE bool isValidUrlTemplate(const QString &url) const;
    Q_INVOKABLE bool setDictionaryUrl(const QString &url);
    Q_INVOKABLE bool setTranslatorUrl(const QString &url);
    Q_INVOKABLE void resetDictionaryUrl();
    Q_INVOKABLE void resetTranslatorUrl();

    // Opens a video (a local path or a file URL) with the data saved for it (I-13). Emits videoOpened.
    Q_INVOKABLE bool openVideo(const QString &location);
    // A file from another instance of the player (I-74); the window saves the playback and opens it.
    void requestOpen(const QString &file);
    // A dropped file: subtitles go to the open video (an error without one), anything else is opened as a video
    // (I-05).
    Q_INVOKABLE bool openDropped(const QString &location);
    Q_INVOKABLE void closeVideo();
    // Saves the playback position of the open video (I-14).
    Q_INVOKABLE void savePlayback(double positionSeconds, double durationSeconds);

    Q_INVOKABLE void clearHistory();
    // Removes files from the history (I-15): one file or all files of a series.
    Q_INVOKABLE void removeFromHistory(const QStringList &paths);

    // Subtitle files of the open video (I-57); an empty location removes the file.
    Q_INVOKABLE void setMainSubtitles(const QString &location);
    Q_INVOKABLE void setSecondarySubtitles(const QString &location);
    // Searches the video folder for subtitle files again (I-06).
    Q_INVOKABLE void refreshFoundSubtitles();
    // Main and secondary subtitle text shown over the video at a playback time (I-22).
    Q_INVOKABLE QString mainOverlayText(double seconds) const;
    Q_INVOKABLE QString secondaryOverlayText(double seconds) const;
    // The next video file in the folder of the open one (I-16); empty for the last one.
    Q_INVOKABLE QString nextVideo() const;

    // Audio track on file open (I-54) for mpv's track list ({id, language, ...}).
    Q_INVOKABLE int chooseAudioTrack(const QVariantList &tracks) const;
    // The user's choice is remembered for the file (I-02, I-13).
    Q_INVOKABLE void setUserAudioTrack(int id);
    Q_INVOKABLE QString audioTrackLabel(const QVariantMap &track) const;
    // A chapter's title, or "Chapter N" for an untitled one (I-72); index is 0-based.
    Q_INVOKABLE QString chapterLabel(const QString &title, int index) const;

    // Dictionary lookup of the word at a cursor position or of a selection (I-59), translation of a cue (I-47).
    Q_INVOKABLE bool lookupWord(const QString &text, int position);
    Q_INVOKABLE bool lookupSelection(const QString &selection);
    Q_INVOKABLE bool translate(const QString &text);

    // Embedded text subtitle tracks of the open video: [{streamIndex, label, language}] (I-08).
    Q_INVOKABLE QVariantList embeddedTracks() const;
    // Names of the files that already exist in the folder for the chosen tracks.
    Q_INVOKABLE QStringList exportConflicts(const QVariantList &streamIndexes, const QString &folderLocation) const;
    Q_INVOKABLE void startExport(const QVariantList &streamIndexes, const QString &folderLocation, bool keepBoth);
    Q_INVOKABLE void cancelExport();
    // Index of the first file in the language, or -1 (pre-selection after export, I-55).
    Q_INVOKABLE int findFileByLanguage(const QVariantList &files, const QString &language) const;

    // The main window between runs (I-75): {valid, x, y, width, height, maximized, fullScreen}. Not valid when
    // nothing is saved or the saved place is on none of the current screens.
    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized, bool fullScreen);

    Q_INVOKABLE QString languageLabel(const QString &code) const;
    Q_INVOKABLE int physicalKey(int key) const;
    Q_INVOKABLE QString formatTime(double seconds) const;
    Q_INVOKABLE QString toLocalPath(const QString &location) const;
    Q_INVOKABLE QString fileName(const QString &path) const;

signals:
    void settingsChanged();
    void historyChanged();
    void videoChanged();
    void subtitlesChanged();
    void exportStateChanged();
    void videoOpened(const QString &path, double startSeconds);
    void openRequested(const QString &file);
    // files: [{path, name, language}]; empty on failure or when another video was opened during the export.
    void exportFinished(bool ok, bool cancelled, const QString &error, const QVariantList &files);
    void errorOccurred(const QString &message);

private:
    void loadSettings();
    void saveSettings();
    void loadHistory();
    void saveHistory();
    void reloadSubtitles();
    // Applies the automatic search (I-06) to the open video; reloads the subtitles when the files change.
    void applyFoundSubtitles();
    afvp::SubtitleChoice currentSubtitles() const;
    // Stores the current subtitles in the history as the user's choice before the user changes one of them.
    afvp::HistoryEntry *chooseSubtitlesEntry();
    afvp::SubtitleCues loadCues(const QString &path);
    afvp::HistoryEntry *currentEntry();
    bool openUrl(const QString &urlTemplate, const QString &text);
    bool setUrlTemplate(QString &target, const QString &url);
    void setFontSize(int &target, int size);
    QList<afvp::EmbeddedSubtitleTrack> selectedTracks(const QVariantList &streamIndexes) const;
    // Exports the tracks of a new video without subtitle files and assigns them (I-85).
    void startAutoExport();
    // automatic: the tracks of the automatic export, assigned to the video afterwards (I-85).
    void runExport(const QList<afvp::EmbeddedSubtitleTrack> &tracks, const QDir &folder, afvp::ConflictPolicy policy,
                   const std::optional<afvp::AutoExportChoice> &automatic);
    void assignAutoExported(const QString &videoPath, const afvp::AutoExportChoice &choice,
                            const QList<afvp::ExtractionTarget> &targets);
    void joinExportThread();

    QString m_settingsPath;
    QString m_historyPath;
    afvp::Settings m_settings;
    afvp::History m_history;

    QString m_videoPath;
    QList<afvp::EmbeddedSubtitleTrack> m_embeddedTracks;
    QList<afvp::FoundSubtitle> m_foundSubtitles;
    afvp::SubtitleChoice m_automaticSubtitles;
    SubtitleListModel m_subtitles;
    // Cues of the open subtitle files with their offsets, for the overlay (I-22).
    afvp::SubtitleCues m_mainCues;
    afvp::SubtitleCues m_secondaryCues;

    std::thread m_exportThread;
    std::atomic_bool m_exportCancel = false;
    bool m_exportRunning = false;
    double m_exportProgress = 0;
    BatchExporter m_batchExport;
};
