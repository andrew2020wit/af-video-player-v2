#include "AppController.h"

#include "AudioTracks.h"
#include "AutoExport.h"
#include "HistoryGroups.h"
#include "CueText.h"
#include "JsonFile.h"
#include "KeyMapping.h"
#include "Languages.h"
#include "MediaFiles.h"
#include "SubtitleExtractor.h"
#include "SubtitleParser.h"
#include "TimeFormat.h"
#include "WordLookup.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QLocale>
#include <QScreen>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>

namespace {

QString settingsFilePath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/settings.json");
}

QString historyFilePath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/history.json");
}

QString folderUrl(const QString &folder) {
    if (folder.isEmpty() || !QFileInfo(folder).isDir()) {
        return QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)).toString();
    }

    return QUrl::fromLocalFile(folder).toString();
}

// "today 14:05", "yesterday 21:40" or a short date.
QString describeDate(const QDateTime &dateTime) {
    if (!dateTime.isValid()) {
        return QString();
    }

    const QDateTime local = dateTime.toLocalTime();
    const QDate today = QDate::currentDate();
    const QLocale locale;

    if (local.date() == today) {
        return AppController::tr("today %1").arg(locale.toString(local.time(), QLocale::ShortFormat));
    }

    if (local.date() == today.addDays(-1)) {
        return AppController::tr("yesterday %1").arg(locale.toString(local.time(), QLocale::ShortFormat));
    }

    return locale.toString(local.date(), QLocale::ShortFormat);
}

QString describeEmbeddedTrack(const afvp::EmbeddedSubtitleTrack &track) {
    QStringList parts;
    parts.append(track.language.isEmpty() ? AppController::tr("language not set") : afvp::languageLabel(track.language));

    if (!track.title.isEmpty()) {
        parts.append(track.title);
    }

    parts.append(track.codecName.toUpper());

    if (track.forced) {
        parts.append(QStringLiteral("forced"));
    }

    if (track.hearingImpaired) {
        parts.append(QStringLiteral("SDH"));
    }

    return QStringLiteral("%1. %2").arg(track.streamIndex).arg(parts.join(QStringLiteral(" · ")));
}

} // namespace

AppController::AppController(QObject *parent)
    : QObject(parent), m_settingsPath(settingsFilePath()), m_historyPath(historyFilePath()) {
    loadSettings();
    loadHistory();

    connect(&m_batchExport, &BatchExporter::finished, this, [this](bool cancelled, const QStringList &errors) {
        // The open video may be in the folder: its new files are found (I-06).
        refreshFoundSubtitles();

        if (!cancelled && m_batchExport.videoCount() == 0) {
            emit errorOccurred(tr("The folder has no video files:\n%1").arg(m_batchExport.folder()));
        } else if (!errors.isEmpty()) {
            emit errorOccurred(tr("Batch export failed for some videos:\n\n%1").arg(errors.join(u'\n')));
        }
    });
}

AppController::~AppController() {
    m_exportCancel = true;
    joinExportThread();
}

QString AppController::studiedLanguage() const {
    return m_settings.studiedLanguage;
}

void AppController::setStudiedLanguage(const QString &code) {
    if (code.isEmpty() || code == m_settings.studiedLanguage) {
        return;
    }

    m_settings.studiedLanguage = code;
    saveSettings();
    applyFoundSubtitles();
}

QString AppController::nativeLanguage() const {
    return m_settings.nativeLanguage;
}

void AppController::setNativeLanguage(const QString &code) {
    if (code.isEmpty() || code == m_settings.nativeLanguage) {
        return;
    }

    m_settings.nativeLanguage = code;
    saveSettings();
    applyFoundSubtitles();
}

QString AppController::preferredAudioLanguage() const {
    return m_settings.preferredAudioLanguage;
}

void AppController::setPreferredAudioLanguage(const QString &code) {
    if (code == m_settings.preferredAudioLanguage) {
        return;
    }

    m_settings.preferredAudioLanguage = code;
    saveSettings();
}

QString AppController::dictionaryUrl() const {
    return m_settings.dictionaryUrl;
}

QString AppController::translatorUrl() const {
    return m_settings.translatorUrl;
}

int AppController::mainSubtitleFontSize() const {
    return m_settings.mainSubtitleFontSize;
}

void AppController::setMainSubtitleFontSize(int size) {
    setFontSize(m_settings.mainSubtitleFontSize, size);
}

int AppController::secondarySubtitleFontSize() const {
    return m_settings.secondarySubtitleFontSize;
}

void AppController::setSecondarySubtitleFontSize(int size) {
    setFontSize(m_settings.secondarySubtitleFontSize, size);
}

bool AppController::showSubtitleOverlay() const {
    return m_settings.showSubtitleOverlay;
}

void AppController::setShowSubtitleOverlay(bool show) {
    if (show == m_settings.showSubtitleOverlay) {
        return;
    }

    m_settings.showSubtitleOverlay = show;
    saveSettings();
}

int AppController::mainOverlayFontSize() const {
    return m_settings.mainOverlayFontSize;
}

void AppController::setMainOverlayFontSize(int size) {
    setFontSize(m_settings.mainOverlayFontSize, size);
}

int AppController::secondaryOverlayFontSize() const {
    return m_settings.secondaryOverlayFontSize;
}

void AppController::setSecondaryOverlayFontSize(int size) {
    setFontSize(m_settings.secondaryOverlayFontSize, size);
}

int AppController::minimumSubtitleFontSize() const {
    return afvp::Settings::minimumSubtitleFontSize;
}

int AppController::maximumSubtitleFontSize() const {
    return afvp::Settings::maximumSubtitleFontSize;
}

int AppController::volume() const {
    return m_settings.volume;
}

void AppController::setVolume(int volume) {
    volume = std::clamp(volume, 0, 100);

    if (volume == m_settings.volume) {
        return;
    }

    m_settings.volume = volume;
    saveSettings();
}

bool AppController::muted() const {
    return m_settings.muted;
}

void AppController::setMuted(bool muted) {
    if (muted == m_settings.muted) {
        return;
    }

    m_settings.muted = muted;
    saveSettings();
}

int AppController::historySize() const {
    return m_settings.historySize;
}

void AppController::setHistorySize(int size) {
    size = std::clamp(size, afvp::Settings::minimumHistorySize, afvp::Settings::maximumHistorySize);

    if (size == m_settings.historySize) {
        return;
    }

    m_settings.historySize = size;
    saveSettings();
    m_history.trim(size);
    saveHistory();
}

bool AppController::restartWatched() const {
    return m_settings.restartWatched;
}

void AppController::setRestartWatched(bool restart) {
    if (restart == m_settings.restartWatched) {
        return;
    }

    m_settings.restartWatched = restart;
    saveSettings();
}

QString AppController::openFolderUrl() const {
    const QString &file = m_settings.lastOpenFile;

    return folderUrl(file.isEmpty() ? QString() : afvp::existingFolder(QFileInfo(file).absolutePath()));
}

QString AppController::openFileUrl() const {
    const QString &file = m_settings.lastOpenFile;

    return !file.isEmpty() && QFileInfo(file).isFile() ? QUrl::fromLocalFile(file).toString() : QString();
}

QVariantList AppController::history() const {
    // Entries of missing files are kept for moved files (I-12) but not shown (I-56).
    const QList<afvp::HistoryGroup> groups = afvp::groupHistory(
        m_history.entries(), [](const QString &path) { return QFileInfo(path).isFile(); }, afvp::videoFilesInFolder);
    QVariantList items;

    for (const afvp::HistoryGroup &group : groups) {
        QVariantList entries;

        for (const afvp::HistoryEntry &entry : group.entries) {
            const QFileInfo fileInfo(entry.path);
            const double progress = entry.durationMs > 0
                                        ? std::clamp(double(entry.positionMs) / double(entry.durationMs), 0.0, 1.0)
                                        : -1.0;
            entries.append(QVariantMap{
                {QStringLiteral("path"), entry.path},
                {QStringLiteral("name"), fileInfo.fileName()},
                {QStringLiteral("folder"), QDir::toNativeSeparators(fileInfo.absolutePath())},
                {QStringLiteral("progress"), progress},
                {QStringLiteral("watched"), afvp::isWatched(entry)},
                {QStringLiteral("lastOpened"), describeDate(entry.lastOpened)},
            });
        }

        items.append(QVariantMap{
            {QStringLiteral("folder"), QDir::toNativeSeparators(group.folder)},
            {QStringLiteral("folderName"), QFileInfo(group.folder).fileName()},
            {QStringLiteral("isSeries"), group.isSeries()},
            {QStringLiteral("watchedCount"), group.watchedCount},
            {QStringLiteral("videoCount"), int(group.videoFiles.size())},
            {QStringLiteral("continuePath"), group.continuePath},
            {QStringLiteral("continueName"), QFileInfo(group.continuePath).fileName()},
            {QStringLiteral("lastOpened"), entries.front().toMap().value(QStringLiteral("lastOpened"))},
            {QStringLiteral("entries"), entries},
        });
    }

    return items;
}

QString AppController::videoPath() const {
    return m_videoPath;
}

QString AppController::videoName() const {
    return QFileInfo(m_videoPath).fileName();
}

QString AppController::videoFolderUrl() const {
    return m_videoPath.isEmpty() ? openFolderUrl() : folderUrl(QFileInfo(m_videoPath).absolutePath());
}

int AppController::embeddedTextTrackCount() const {
    return int(std::count_if(m_embeddedTracks.cbegin(), m_embeddedTracks.cend(),
                             [](const afvp::EmbeddedSubtitleTrack &track) { return track.isText; }));
}

QString AppController::mainSubtitlesPath() const {
    return currentSubtitles().main;
}

QString AppController::secondarySubtitlesPath() const {
    return currentSubtitles().secondary;
}

bool AppController::subtitlesAutomatic() const {
    const afvp::HistoryEntry *entry = m_videoPath.isEmpty() ? nullptr : m_history.find(m_videoPath);

    return entry && !entry->subtitlesChosen &&
           (!m_automaticSubtitles.main.isEmpty() || !m_automaticSubtitles.secondary.isEmpty());
}

QVariantList AppController::foundSubtitles() const {
    QVariantList list;

    for (const afvp::FoundSubtitle &file : m_foundSubtitles) {
        QStringList description{file.info.language.isEmpty() ? tr("Unknown language")
                                                             : afvp::languageLabel(file.info.language)};

        if (file.info.forced) {
            description.append(tr("forced"));
        }

        if (file.info.sdh) {
            description.append(tr("SDH"));
        }

        list.append(QVariantMap{
            {QStringLiteral("path"), file.path},
            {QStringLiteral("relativePath"), QDir::toNativeSeparators(file.relativePath)},
            {QStringLiteral("description"), description.join(QStringLiteral(" · "))},
        });
    }

    return list;
}

SubtitleListModel *AppController::subtitles() {
    return &m_subtitles;
}

bool AppController::exportRunning() const {
    return m_exportRunning;
}

double AppController::exportProgress() const {
    return m_exportProgress;
}

BatchExporter *AppController::batchExport() {
    return &m_batchExport;
}

QVariantList AppController::languages() const {
    QVariantList list;

    for (const afvp::Language &language : afvp::allLanguages()) {
        list.append(QVariantMap{
            {QStringLiteral("code"), language.code},
            {QStringLiteral("label"), afvp::languageLabel(language.code)},
            // For the filter in the language picker (I-50).
            {QStringLiteral("search"), QStringList{language.code, language.terminological, language.englishName,
                                                   language.nativeName}
                                           .join(u' ')
                                           .toLower()},
        });
    }

    return list;
}

QString AppController::appVersion() const {
    return QCoreApplication::applicationVersion();
}

QString AppController::homepageUrl() const {
    return QStringLiteral(AFVP_HOMEPAGE_URL);
}

QString AppController::qtVersion() const {
    return QString::fromLatin1(qVersion());
}

QString AppController::ffmpegVersion() const {
    return afvp::ffmpegVersion();
}

bool AppController::isValidUrlTemplate(const QString &url) const {
    return afvp::Settings::isValidUrlTemplate(url.trimmed());
}

bool AppController::setDictionaryUrl(const QString &url) {
    return setUrlTemplate(m_settings.dictionaryUrl, url);
}

bool AppController::setTranslatorUrl(const QString &url) {
    return setUrlTemplate(m_settings.translatorUrl, url);
}

void AppController::resetDictionaryUrl() {
    setUrlTemplate(m_settings.dictionaryUrl, afvp::Settings::defaultDictionaryUrl());
}

void AppController::resetTranslatorUrl() {
    setUrlTemplate(m_settings.translatorUrl, afvp::Settings::defaultTranslatorUrl());
}

bool AppController::openVideo(const QString &location) {
    const QString path = toLocalPath(location);
    const QFileInfo fileInfo(path);

    if (path.isEmpty() || !fileInfo.isFile()) {
        emit errorOccurred(tr("File not found: %1").arg(QDir::toNativeSeparators(path)));

        return false;
    }

    bool isNew = false;
    afvp::HistoryEntry &entry =
        m_history.touch(fileInfo.absoluteFilePath(), fileInfo.size(), QDateTime::currentDateTimeUtc(),
                        [](const QString &file) { return QFileInfo::exists(file); }, &isNew);
    const bool restart = m_settings.restartWatched && afvp::isWatched(entry);
    const double startSeconds = restart ? 0.0 : double(entry.positionMs) / 1000.0;
    m_history.trim(m_settings.historySize);
    saveHistory();

    m_videoPath = fileInfo.absoluteFilePath();
    m_embeddedTracks = afvp::probeSubtitleTracks(m_videoPath);
    m_foundSubtitles = afvp::findSubtitles(m_videoPath);
    m_automaticSubtitles =
        afvp::chooseSubtitles(m_foundSubtitles, m_settings.studiedLanguage, m_settings.nativeLanguage);
    m_settings.lastOpenFile = m_videoPath;
    saveSettings();
    reloadSubtitles();
    emit videoChanged();
    emit videoOpened(m_videoPath, startSeconds);

    if (isNew && m_foundSubtitles.isEmpty()) {
        startAutoExport();
    }

    return true;
}

QVariantMap AppController::windowGeometry() const {
    const afvp::WindowGeometry &geometry = m_settings.window;
    // The title bar must be reachable: the top strip of the window has to be on a screen.
    const QRect top(geometry.x, geometry.y, geometry.width, 40);
    const QList<QScreen *> screens = QGuiApplication::screens();
    const bool onScreen = std::any_of(screens.cbegin(), screens.cend(), [&top](const QScreen *screen) {
        return screen->availableGeometry().intersected(top).width() >= 100;
    });

    return {
        {QStringLiteral("valid"), geometry.isValid() && onScreen},
        {QStringLiteral("x"), geometry.x},
        {QStringLiteral("y"), geometry.y},
        {QStringLiteral("width"), geometry.width},
        {QStringLiteral("height"), geometry.height},
        {QStringLiteral("maximized"), geometry.maximized},
        {QStringLiteral("fullScreen"), geometry.fullScreen},
    };
}

void AppController::saveWindowGeometry(int x, int y, int width, int height, bool maximized, bool fullScreen) {
    const afvp::WindowGeometry geometry{x, y, width, height, maximized, fullScreen};

    if (!geometry.isValid() || geometry == m_settings.window) {
        return;
    }

    m_settings.window = geometry;
    saveSettings();
}

void AppController::requestOpen(const QString &file) {
    emit openRequested(file);
}

bool AppController::openDropped(const QString &location) {
    const QString path = toLocalPath(location);

    if (afvp::isSubtitleFileName(path)) {
        if (m_videoPath.isEmpty()) {
            emit errorOccurred(tr("Open a video first: a dropped subtitle file is added to the open video."));

            return false;
        }

        setMainSubtitles(path);

        return true;
    }

    return openVideo(path);
}

void AppController::closeVideo() {
    m_videoPath.clear();
    m_embeddedTracks.clear();
    m_foundSubtitles.clear();
    m_automaticSubtitles = {};
    m_subtitles.setCues({});
    m_mainCues.clear();
    m_secondaryCues.clear();
    emit videoChanged();
    emit subtitlesChanged();
}

void AppController::savePlayback(double positionSeconds, double durationSeconds) {
    afvp::HistoryEntry *entry = currentEntry();

    if (!entry) {
        return;
    }

    entry->positionMs = qint64(std::max(0.0, positionSeconds) * 1000.0);

    if (durationSeconds > 0) {
        entry->durationMs = qint64(durationSeconds * 1000.0);
    }

    saveHistory();
}

void AppController::clearHistory() {
    m_history.clear();
    saveHistory();
}

void AppController::removeFromHistory(const QStringList &paths) {
    for (const QString &path : paths) {
        m_history.remove(path);
    }

    saveHistory();
}

void AppController::setMainSubtitles(const QString &location) {
    afvp::HistoryEntry *entry = chooseSubtitlesEntry();

    if (!entry) {
        return;
    }

    entry->mainSubtitles = toLocalPath(location);
    saveHistory();
    reloadSubtitles();
}

void AppController::setSecondarySubtitles(const QString &location) {
    afvp::HistoryEntry *entry = chooseSubtitlesEntry();

    if (!entry) {
        return;
    }

    entry->secondarySubtitles = toLocalPath(location);
    saveHistory();
    reloadSubtitles();
}

void AppController::refreshFoundSubtitles() {
    if (m_videoPath.isEmpty()) {
        return;
    }

    m_foundSubtitles = afvp::findSubtitles(m_videoPath);
    applyFoundSubtitles();
}

QString AppController::mainOverlayText(double seconds) const {
    return afvp::activeCueText(m_mainCues, qint64(seconds * 1000.0 + 0.5));
}

QString AppController::secondaryOverlayText(double seconds) const {
    return afvp::activeCueText(m_secondaryCues, qint64(seconds * 1000.0 + 0.5));
}

QString AppController::nextVideo() const {
    return m_videoPath.isEmpty() ? QString() : afvp::nextVideoFile(m_videoPath);
}

int AppController::chooseAudioTrack(const QVariantList &tracks) const {
    QList<afvp::AudioTrack> audioTracks;

    for (const QVariant &item : tracks) {
        const QVariantMap map = item.toMap();
        audioTracks.append({map.value(QStringLiteral("id")).toInt(), map.value(QStringLiteral("language")).toString(),
                            map.value(QStringLiteral("title")).toString(), map.value(QStringLiteral("codec")).toString(),
                            map.value(QStringLiteral("channelCount")).toInt(),
                            map.value(QStringLiteral("isDefault")).toBool()});
    }

    const afvp::HistoryEntry *entry = m_history.find(m_videoPath);

    return afvp::chooseAudioTrack(audioTracks, entry ? entry->audioTrackId : 0, m_settings.effectiveAudioLanguage());
}

void AppController::setUserAudioTrack(int id) {
    afvp::HistoryEntry *entry = currentEntry();

    if (!entry || id <= 0) {
        return;
    }

    entry->audioTrackId = id;
    saveHistory();
}

QString AppController::chapterLabel(const QString &title, int index) const {
    return title.trimmed().isEmpty() ? tr("Chapter %1").arg(index + 1) : title.trimmed();
}

QString AppController::audioTrackLabel(const QVariantMap &track) const {
    const afvp::AudioTrack audioTrack{track.value(QStringLiteral("id")).toInt(),
                                      track.value(QStringLiteral("language")).toString(),
                                      track.value(QStringLiteral("title")).toString(),
                                      track.value(QStringLiteral("codec")).toString(),
                                      track.value(QStringLiteral("channelCount")).toInt(),
                                      track.value(QStringLiteral("isDefault")).toBool()};

    return afvp::describeAudioTrack(audioTrack, tr("Track %1"));
}

bool AppController::lookupWord(const QString &text, int position) {
    const QString word = afvp::wordAt(text, position);

    if (word.isEmpty()) {
        return false;
    }

    return openUrl(m_settings.dictionaryUrl, word);
}

bool AppController::lookupSelection(const QString &selection) {
    const QString text = afvp::cleanSelection(selection);

    if (text.isEmpty()) {
        return false;
    }

    return openUrl(m_settings.dictionaryUrl, text);
}

bool AppController::translate(const QString &text) {
    const QString cueText = afvp::normalizeCueText(text).replace(u'\n', u' ');

    if (cueText.isEmpty()) {
        return false;
    }

    return openUrl(m_settings.translatorUrl, cueText);
}

QVariantList AppController::embeddedTracks() const {
    QVariantList list;

    for (const afvp::EmbeddedSubtitleTrack &track : m_embeddedTracks) {
        if (!track.isText) {
            continue;
        }

        list.append(QVariantMap{
            {QStringLiteral("streamIndex"), track.streamIndex},
            {QStringLiteral("label"), describeEmbeddedTrack(track)},
            {QStringLiteral("language"), track.language},
        });
    }

    return list;
}

QStringList AppController::exportConflicts(const QVariantList &streamIndexes, const QString &folderLocation) const {
    const QDir folder(toLocalPath(folderLocation));
    const QStringList names = afvp::exportFileNames(QFileInfo(m_videoPath).completeBaseName(),
                                                    selectedTracks(streamIndexes), afvp::ConflictPolicy::Replace,
                                                    [](const QString &) { return false; });
    QStringList conflicts;

    for (const QString &name : names) {
        if (folder.exists(name)) {
            conflicts.append(name);
        }
    }

    return conflicts;
}

void AppController::startExport(const QVariantList &streamIndexes, const QString &folderLocation, bool keepBoth) {
    if (m_exportRunning || m_videoPath.isEmpty()) {
        return;
    }

    runExport(selectedTracks(streamIndexes), QDir(toLocalPath(folderLocation)),
              keepBoth ? afvp::ConflictPolicy::KeepBoth : afvp::ConflictPolicy::Replace, std::nullopt);
}

void AppController::startAutoExport() {
    if (m_exportRunning || m_videoPath.isEmpty()) {
        return;
    }

    const afvp::AutoExportChoice choice =
        afvp::chooseAutoExportTracks(m_embeddedTracks, m_settings.studiedLanguage, m_settings.nativeLanguage);

    if (choice.isEmpty()) {
        return;
    }

    QVariantList streamIndexes{choice.mainStreamIndex};

    if (choice.secondaryStreamIndex >= 0) {
        streamIndexes.append(choice.secondaryStreamIndex);
    }

    // An existing file is never replaced.
    runExport(selectedTracks(streamIndexes), QDir(QFileInfo(m_videoPath).absolutePath()),
              afvp::ConflictPolicy::KeepBoth, choice);
}

void AppController::runExport(const QList<afvp::EmbeddedSubtitleTrack> &tracks, const QDir &folder,
                              afvp::ConflictPolicy policy, const std::optional<afvp::AutoExportChoice> &automatic) {
    joinExportThread();

    const QStringList names =
        afvp::exportFileNames(QFileInfo(m_videoPath).completeBaseName(), tracks, policy,
                              [&folder](const QString &name) { return folder.exists(name); });
    QList<afvp::ExtractionTarget> targets;
    QVariantList files;

    for (qsizetype i = 0; i < tracks.size(); ++i) {
        const QString path = folder.absoluteFilePath(names[i]);
        targets.append({tracks[i].streamIndex, path});
        files.append(QVariantMap{
            {QStringLiteral("path"), path},
            {QStringLiteral("name"), names[i]},
            {QStringLiteral("language"), tracks[i].language},
        });
    }

    m_exportCancel = false;
    m_exportRunning = true;
    m_exportProgress = 0;
    emit exportStateChanged();

    const QString videoPath = m_videoPath;
    m_exportThread = std::thread([this, videoPath, targets, files, automatic]() {
        QString error;
        const bool ok = afvp::extractSubtitleTracks(
            videoPath, targets,
            [this](double progress) {
                QMetaObject::invokeMethod(
                    this,
                    [this, progress]() {
                        m_exportProgress = progress;
                        emit exportStateChanged();
                    },
                    Qt::QueuedConnection);
            },
            m_exportCancel, &error);
        const bool cancelled = m_exportCancel.load();

        QMetaObject::invokeMethod(
            this,
            [this, ok, cancelled, error, files, videoPath, targets, automatic]() {
                m_exportRunning = false;
                m_exportProgress = ok ? 1.0 : 0.0;
                emit exportStateChanged();

                if (ok && automatic) {
                    assignAutoExported(videoPath, *automatic, targets);
                }

                if (ok) {
                    refreshFoundSubtitles();
                }

                // Another video may have been opened meanwhile: the files are offered only to their own video.
                // Automatically exported files are assigned without asking (I-85).
                const bool sameVideo = afvp::History::samePath(m_videoPath, videoPath);
                emit exportFinished(ok, cancelled, error,
                                    ok && sameVideo && !automatic ? files : QVariantList());
            },
            Qt::QueuedConnection);
    });
}

void AppController::cancelExport() {
    m_exportCancel = true;
}

void AppController::assignAutoExported(const QString &videoPath, const afvp::AutoExportChoice &choice,
                                       const QList<afvp::ExtractionTarget> &targets) {
    afvp::HistoryEntry *entry = m_history.find(videoPath);

    // The user has chosen subtitles during the export.
    if (!entry || entry->subtitlesChosen) {
        return;
    }

    const auto exportedPath = [&targets](int streamIndex) {
        for (const afvp::ExtractionTarget &target : targets) {
            if (target.streamIndex == streamIndex) {
                return target.outputPath;
            }
        }

        return QString();
    };

    entry->mainSubtitles = exportedPath(choice.mainStreamIndex);
    entry->secondarySubtitles = exportedPath(choice.secondaryStreamIndex);
    entry->subtitlesChosen = true;
    saveHistory();

    if (afvp::History::samePath(m_videoPath, videoPath)) {
        reloadSubtitles();
    }
}

int AppController::findFileByLanguage(const QVariantList &files, const QString &language) const {
    for (qsizetype i = 0; i < files.size(); ++i) {
        if (afvp::sameLanguage(files[i].toMap().value(QStringLiteral("language")).toString(), language)) {
            return int(i);
        }
    }

    return -1;
}

QString AppController::languageLabel(const QString &code) const {
    return afvp::languageLabel(code);
}

int AppController::physicalKey(int key) const {
    return afvp::physicalKey(key);
}

QString AppController::formatTime(double seconds) const {
    return afvp::formatTime(qint64(seconds * 1000.0));
}

QString AppController::toLocalPath(const QString &location) const {
    if (location.isEmpty()) {
        return QString();
    }

    const QUrl url(location);

    if (url.isLocalFile()) {
        return QDir::fromNativeSeparators(url.toLocalFile());
    }

    return QDir::fromNativeSeparators(location);
}

QString AppController::fileName(const QString &path) const {
    return QFileInfo(path).fileName();
}

void AppController::loadSettings() {
    const auto object = afvp::readJsonObject(m_settingsPath);
    m_settings = object ? afvp::Settings::fromJson(*object) : afvp::Settings();
}

void AppController::saveSettings() {
    if (!afvp::writeJsonObject(m_settingsPath, m_settings.toJson())) {
        qWarning("Failed to save %s", qPrintable(m_settingsPath));
    }

    emit settingsChanged();
}

void AppController::loadHistory() {
    const auto object = afvp::readJsonObject(m_historyPath);

    if (object) {
        m_history = afvp::History::fromJson(*object);
        m_history.trim(m_settings.historySize);
    }
}

void AppController::saveHistory() {
    if (!afvp::writeJsonObject(m_historyPath, m_history.toJson())) {
        qWarning("Failed to save %s", qPrintable(m_historyPath));
    }

    emit historyChanged();
}

void AppController::reloadSubtitles() {
    const afvp::HistoryEntry *entry = currentEntry();

    if (!entry) {
        m_subtitles.setCues({});
        m_mainCues.clear();
        m_secondaryCues.clear();
        emit subtitlesChanged();

        return;
    }

    const afvp::SubtitleChoice files = currentSubtitles();
    m_mainCues = afvp::shiftCues(loadCues(files.main), entry->mainOffsetMs);
    m_secondaryCues = afvp::shiftCues(loadCues(files.secondary), entry->secondaryOffsetMs);
    m_subtitles.setCues(afvp::mergeSubtitles(m_mainCues, m_secondaryCues));
    emit subtitlesChanged();
}

void AppController::applyFoundSubtitles() {
    if (m_videoPath.isEmpty()) {
        return;
    }

    const afvp::SubtitleChoice before = currentSubtitles();
    m_automaticSubtitles =
        afvp::chooseSubtitles(m_foundSubtitles, m_settings.studiedLanguage, m_settings.nativeLanguage);

    if (currentSubtitles() != before) {
        reloadSubtitles();
    } else {
        emit subtitlesChanged();
    }
}

afvp::SubtitleChoice AppController::currentSubtitles() const {
    const afvp::HistoryEntry *entry = m_videoPath.isEmpty() ? nullptr : m_history.find(m_videoPath);

    if (!entry) {
        return {};
    }

    if (entry->subtitlesChosen) {
        return {entry->mainSubtitles, entry->secondarySubtitles};
    }

    return m_automaticSubtitles;
}

afvp::HistoryEntry *AppController::chooseSubtitlesEntry() {
    afvp::HistoryEntry *entry = currentEntry();

    if (!entry) {
        return nullptr;
    }

    if (!entry->subtitlesChosen) {
        entry->mainSubtitles = m_automaticSubtitles.main;
        entry->secondarySubtitles = m_automaticSubtitles.secondary;
    }

    entry->subtitlesChosen = true;

    return entry;
}

afvp::SubtitleCues AppController::loadCues(const QString &path) {
    if (path.isEmpty()) {
        return {};
    }

    const afvp::SubtitleLoadResult result = afvp::loadSubtitleFile(path);

    if (!result.ok) {
        emit errorOccurred(
            tr("Cannot open subtitles %1: %2").arg(QDir::toNativeSeparators(path), result.error));
    }

    return result.cues;
}

afvp::HistoryEntry *AppController::currentEntry() {
    return m_videoPath.isEmpty() ? nullptr : m_history.find(m_videoPath);
}

bool AppController::openUrl(const QString &urlTemplate, const QString &text) {
    const QString url =
        afvp::fillUrlTemplate(urlTemplate, text, m_settings.studiedLanguage, m_settings.nativeLanguage);

    return QDesktopServices::openUrl(QUrl(url, QUrl::TolerantMode));
}

bool AppController::setUrlTemplate(QString &target, const QString &url) {
    const QString trimmed = url.trimmed();

    if (!afvp::Settings::isValidUrlTemplate(trimmed)) {
        return false;
    }

    if (trimmed != target) {
        target = trimmed;
        saveSettings();
    }

    return true;
}

void AppController::setFontSize(int &target, int size) {
    size = std::clamp(size, afvp::Settings::minimumSubtitleFontSize, afvp::Settings::maximumSubtitleFontSize);

    if (size != target) {
        target = size;
        saveSettings();
    }
}

QList<afvp::EmbeddedSubtitleTrack> AppController::selectedTracks(const QVariantList &streamIndexes) const {
    QList<afvp::EmbeddedSubtitleTrack> tracks;

    for (const afvp::EmbeddedSubtitleTrack &track : m_embeddedTracks) {
        for (const QVariant &index : streamIndexes) {
            if (index.toInt() == track.streamIndex && track.isText) {
                tracks.append(track);
            }
        }
    }

    return tracks;
}

void AppController::joinExportThread() {
    if (m_exportThread.joinable()) {
        m_exportThread.join();
    }
}
