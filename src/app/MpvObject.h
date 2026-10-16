#pragma once

#include <QQuickFramebufferObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <memory>

struct mpv_handle;
struct mpv_node;

// Video item backed by libmpv's OpenGL render API. Qt Quick must use the OpenGL backend
// (QQuickWindow::setGraphicsApi in main).
class MpvObject : public QQuickFramebufferObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(MpvPlayer)

    Q_PROPERTY(bool hasFile READ hasFile NOTIFY hasFileChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(double position READ position NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool eofReached READ eofReached NOTIFY eofReachedChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(QVariantList audioTracks READ audioTracks NOTIFY audioTracksChanged)
    Q_PROPERTY(int audioTrackId READ audioTrackId NOTIFY audioTrackIdChanged)
    // Chapters of the file (I-72): [{title, time}], time in seconds; the current one, -1 before the first.
    Q_PROPERTY(QVariantList chapters READ chapters NOTIFY chaptersChanged)
    Q_PROPERTY(int chapterIndex READ chapterIndex NOTIFY chapterIndexChanged)
    Q_PROPERTY(QString mpvVersion READ mpvVersion CONSTANT)

public:
    explicit MpvObject(QQuickItem *parent = nullptr);
    ~MpvObject() override;

    Renderer *createRenderer() const override;

    bool hasFile() const;
    bool paused() const;
    void setPaused(bool paused);
    double position() const;
    double duration() const;
    bool eofReached() const;
    int volume() const;
    void setVolume(int volume);
    bool muted() const;
    void setMuted(bool muted);
    QVariantList audioTracks() const;
    int audioTrackId() const;
    QVariantList chapters() const;
    int chapterIndex() const;
    QString mpvVersion() const;

    // Opens a file paused or playing from startSeconds.
    Q_INVOKABLE void loadFile(const QString &path, double startSeconds);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void togglePause();
    // Exact seek to an absolute time (F-46).
    Q_INVOKABLE void seek(double seconds);
    // Fast seek to the nearest keyframe, while the progress bar is dragged (I-34).
    Q_INVOKABLE void seekKeyframe(double seconds);
    // Keyframe seek relative to the current position (F-15).
    Q_INVOKABLE void seekRelative(double seconds);
    Q_INVOKABLE void setAudioTrack(int id);
    // Previous (-1) or next (1) chapter (I-72).
    Q_INVOKABLE void changeChapter(int delta);

    // Shared with the renderer: the handle must outlive the render context.
    std::shared_ptr<mpv_handle> handle() const;

signals:
    void hasFileChanged();
    void pausedChanged();
    void positionChanged();
    void durationChanged();
    void eofReachedChanged();
    void volumeChanged();
    void mutedChanged();
    void audioTracksChanged();
    void audioTrackIdChanged();
    void chaptersChanged();
    void chapterIndexChanged();
    // The file is open and its track list is known.
    void fileLoaded();
    void loadFailed(const QString &message);

private:
    Q_INVOKABLE void processEvents();
    void handlePropertyChange(const char *name, int format, void *data);
    void updateAudioTracks();
    void updateChapters(const mpv_node *node);
    void command(const QStringList &arguments);
    // Before mpv_initialize.
    void setOption(const char *name, const QString &value);
    // At run time.
    void setMpvProperty(const char *name, const QString &value);

    std::shared_ptr<mpv_handle> m_handle;
    bool m_hasFile = false;
    bool m_paused = true;
    double m_position = 0;
    double m_duration = 0;
    bool m_eofReached = false;
    int m_volume = 100;
    bool m_muted = false;
    QVariantList m_audioTracks;
    int m_audioTrackId = 0;
    QVariantList m_chapters;
    int m_chapterIndex = -1;
};
