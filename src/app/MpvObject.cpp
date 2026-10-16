#include "MpvObject.h"

#include <QMetaObject>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QQuickOpenGLUtils>
#include <QVariantMap>

#include <mpv/client.h>
#include <mpv/render_gl.h>

#include <algorithm>
#include <cstring>

namespace {

// Lives in the GUI thread; mpv's update callback (any thread) reaches the item through it. It is deleted only
// after the update callback is unset, so a queued call never targets a deleted object.
class MpvUpdateBridge : public QObject {
    Q_OBJECT

public:
    Q_INVOKABLE void requestUpdate() {
        emit updateRequested();
    }

signals:
    void updateRequested();
};

void onMpvRenderUpdate(void *context) {
    QMetaObject::invokeMethod(static_cast<MpvUpdateBridge *>(context), "requestUpdate", Qt::QueuedConnection);
}

void onMpvWakeup(void *context) {
    QMetaObject::invokeMethod(static_cast<MpvObject *>(context), "processEvents", Qt::QueuedConnection);
}

void *getProcAddress(void *, const char *name) {
    QOpenGLContext *context = QOpenGLContext::currentContext();

    if (!context) {
        return nullptr;
    }

    return reinterpret_cast<void *>(context->getProcAddress(QByteArray(name)));
}

class MpvRenderer : public QQuickFramebufferObject::Renderer {
public:
    explicit MpvRenderer(MpvObject *item) : m_handle(item->handle()), m_bridge(new MpvUpdateBridge) {
        m_bridge->moveToThread(item->thread());
        QObject::connect(m_bridge, &MpvUpdateBridge::updateRequested, item, &QQuickItem::update);
    }

    ~MpvRenderer() override {
        if (m_context) {
            mpv_render_context_set_update_callback(m_context, nullptr, nullptr);
            mpv_render_context_free(m_context);
        }

        m_bridge->deleteLater();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        if (!m_context) {
            mpv_opengl_init_params glInitParams{getProcAddress, nullptr};
            mpv_render_param parameters[] = {
                {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
                {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInitParams},
                {MPV_RENDER_PARAM_INVALID, nullptr},
            };

            if (mpv_render_context_create(&m_context, m_handle.get(), parameters) < 0) {
                qWarning("Failed to create the mpv render context");
                m_context = nullptr;
            } else {
                mpv_render_context_set_update_callback(m_context, onMpvRenderUpdate, m_bridge);
            }
        }

        return QQuickFramebufferObject::Renderer::createFramebufferObject(size);
    }

    void render() override {
        if (!m_context) {
            return;
        }

        QOpenGLFramebufferObject *framebuffer = framebufferObject();
        mpv_opengl_fbo mpvFramebuffer{static_cast<int>(framebuffer->handle()), framebuffer->width(),
                                      framebuffer->height(), 0};
        int flipY = 0;
        mpv_render_param parameters[] = {
            {MPV_RENDER_PARAM_OPENGL_FBO, &mpvFramebuffer},
            {MPV_RENDER_PARAM_FLIP_Y, &flipY},
            {MPV_RENDER_PARAM_INVALID, nullptr},
        };
        mpv_render_context_render(m_context, parameters);
        QQuickOpenGLUtils::resetOpenGLState();
    }

private:
    std::shared_ptr<mpv_handle> m_handle;
    mpv_render_context *m_context = nullptr;
    MpvUpdateBridge *m_bridge;
};

QString nodeString(const mpv_node &node) {
    return node.format == MPV_FORMAT_STRING ? QString::fromUtf8(node.u.string) : QString();
}

} // namespace

MpvObject::MpvObject(QQuickItem *parent) : QQuickFramebufferObject(parent) {
    mpv_handle *handle = mpv_create();

    if (!handle) {
        qFatal("mpv_create failed");
    }

    m_handle = std::shared_ptr<mpv_handle>(handle, mpv_terminate_destroy);

    // Subtitles are never rendered over the video by mpv (P-12); user mpv configs and scripts are ignored.
    setOption("vo", QStringLiteral("libmpv"));
    setOption("hwdec", QStringLiteral("auto-safe"));
    setOption("keep-open", QStringLiteral("yes"));
    setOption("idle", QStringLiteral("yes"));
    setOption("sid", QStringLiteral("no"));
    setOption("sub-auto", QStringLiteral("no"));
    setOption("input-default-bindings", QStringLiteral("no"));
    setOption("input-vo-keyboard", QStringLiteral("no"));
    setOption("osc", QStringLiteral("no"));
    setOption("config", QStringLiteral("no"));
    setOption("load-scripts", QStringLiteral("no"));
    setOption("ytdl", QStringLiteral("no"));
    setOption("terminal", QStringLiteral("no"));
    setOption("audio-client-name", QStringLiteral("AF Video Player"));

    if (mpv_initialize(handle) < 0) {
        qFatal("mpv_initialize failed");
    }

    mpv_observe_property(handle, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(handle, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(handle, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(handle, 0, "eof-reached", MPV_FORMAT_FLAG);
    mpv_observe_property(handle, 0, "volume", MPV_FORMAT_DOUBLE);
    mpv_observe_property(handle, 0, "mute", MPV_FORMAT_FLAG);
    mpv_observe_property(handle, 0, "aid", MPV_FORMAT_INT64);
    mpv_observe_property(handle, 0, "track-list/count", MPV_FORMAT_INT64);
    mpv_observe_property(handle, 0, "chapter-list", MPV_FORMAT_NODE);
    mpv_observe_property(handle, 0, "chapter", MPV_FORMAT_INT64);
    mpv_set_wakeup_callback(handle, onMpvWakeup, this);
}

MpvObject::~MpvObject() {
    mpv_set_wakeup_callback(m_handle.get(), nullptr, nullptr);
    command({QStringLiteral("stop")});
}

QQuickFramebufferObject::Renderer *MpvObject::createRenderer() const {
    return new MpvRenderer(const_cast<MpvObject *>(this));
}

bool MpvObject::hasFile() const {
    return m_hasFile;
}

bool MpvObject::paused() const {
    return m_paused;
}

void MpvObject::setPaused(bool paused) {
    int flag = paused ? 1 : 0;
    mpv_set_property(m_handle.get(), "pause", MPV_FORMAT_FLAG, &flag);
}

double MpvObject::position() const {
    return m_position;
}

double MpvObject::duration() const {
    return m_duration;
}

bool MpvObject::eofReached() const {
    return m_eofReached;
}

int MpvObject::volume() const {
    return m_volume;
}

void MpvObject::setVolume(int volume) {
    double value = std::clamp(volume, 0, 100);
    mpv_set_property(m_handle.get(), "volume", MPV_FORMAT_DOUBLE, &value);
}

bool MpvObject::muted() const {
    return m_muted;
}

void MpvObject::setMuted(bool muted) {
    int flag = muted ? 1 : 0;
    mpv_set_property(m_handle.get(), "mute", MPV_FORMAT_FLAG, &flag);
}

QVariantList MpvObject::audioTracks() const {
    return m_audioTracks;
}

int MpvObject::audioTrackId() const {
    return m_audioTrackId;
}

QVariantList MpvObject::chapters() const {
    return m_chapters;
}

int MpvObject::chapterIndex() const {
    return m_chapterIndex;
}

QString MpvObject::mpvVersion() const {
    char *version = mpv_get_property_string(m_handle.get(), "mpv-version");
    const QString result = QString::fromUtf8(version);
    mpv_free(version);

    return result;
}

void MpvObject::loadFile(const QString &path, double startSeconds) {
    // Options set before loadfile apply to the new file: start position and automatic audio selection (I-54
    // is applied after the track list is known).
    setMpvProperty("start", startSeconds > 0 ? QString::number(startSeconds, 'f', 3) : QStringLiteral("none"));
    setMpvProperty("aid", QStringLiteral("auto"));
    command({QStringLiteral("loadfile"), path, QStringLiteral("replace")});
    setPaused(false);
}

void MpvObject::stop() {
    command({QStringLiteral("stop")});
}

void MpvObject::togglePause() {
    setPaused(!m_paused);
}

void MpvObject::seek(double seconds) {
    command({QStringLiteral("seek"), QString::number(std::max(0.0, seconds), 'f', 3), QStringLiteral("absolute+exact")});
}

void MpvObject::seekKeyframe(double seconds) {
    command({QStringLiteral("seek"), QString::number(std::max(0.0, seconds), 'f', 3),
             QStringLiteral("absolute+keyframes")});
}

void MpvObject::changeChapter(int delta) {
    command({QStringLiteral("add"), QStringLiteral("chapter"), QString::number(delta)});
}

void MpvObject::seekRelative(double seconds) {
    command({QStringLiteral("seek"), QString::number(seconds, 'f', 3), QStringLiteral("relative")});
}

void MpvObject::setAudioTrack(int id) {
    if (id > 0) {
        setMpvProperty("aid", QString::number(id));
    }
}

std::shared_ptr<mpv_handle> MpvObject::handle() const {
    return m_handle;
}

void MpvObject::processEvents() {
    while (m_handle) {
        mpv_event *event = mpv_wait_event(m_handle.get(), 0);

        if (event->event_id == MPV_EVENT_NONE) {
            break;
        }

        switch (event->event_id) {
        case MPV_EVENT_PROPERTY_CHANGE: {
            const auto *property = static_cast<mpv_event_property *>(event->data);
            handlePropertyChange(property->name, property->format, property->data);

            break;
        }
        case MPV_EVENT_FILE_LOADED:
            updateAudioTracks();
            m_hasFile = true;
            emit hasFileChanged();
            emit fileLoaded();

            break;
        case MPV_EVENT_END_FILE: {
            const auto *endFile = static_cast<mpv_event_end_file *>(event->data);

            if (endFile->reason == MPV_END_FILE_REASON_ERROR) {
                emit loadFailed(QString::fromUtf8(mpv_error_string(endFile->error)));
            }

            if (m_hasFile) {
                m_hasFile = false;
                emit hasFileChanged();
            }

            break;
        }
        default:
            break;
        }
    }
}

void MpvObject::handlePropertyChange(const char *name, int format, void *data) {
    const bool hasValue = format != MPV_FORMAT_NONE && data;

    if (std::strcmp(name, "pause") == 0) {
        m_paused = hasValue ? *static_cast<int *>(data) != 0 : true;
        emit pausedChanged();
    } else if (std::strcmp(name, "time-pos") == 0) {
        m_position = hasValue ? *static_cast<double *>(data) : 0;
        emit positionChanged();
    } else if (std::strcmp(name, "duration") == 0) {
        m_duration = hasValue ? *static_cast<double *>(data) : 0;
        emit durationChanged();
    } else if (std::strcmp(name, "eof-reached") == 0) {
        m_eofReached = hasValue && *static_cast<int *>(data) != 0;
        emit eofReachedChanged();
    } else if (std::strcmp(name, "volume") == 0) {
        m_volume = hasValue ? int(*static_cast<double *>(data) + 0.5) : 100;
        emit volumeChanged();
    } else if (std::strcmp(name, "mute") == 0) {
        m_muted = hasValue && *static_cast<int *>(data) != 0;
        emit mutedChanged();
    } else if (std::strcmp(name, "aid") == 0) {
        m_audioTrackId = hasValue ? int(*static_cast<int64_t *>(data)) : 0;
        emit audioTrackIdChanged();
    } else if (std::strcmp(name, "track-list/count") == 0) {
        updateAudioTracks();
    } else if (std::strcmp(name, "chapter-list") == 0) {
        updateChapters(hasValue && format == MPV_FORMAT_NODE ? static_cast<const mpv_node *>(data) : nullptr);
    } else if (std::strcmp(name, "chapter") == 0) {
        m_chapterIndex = hasValue ? int(*static_cast<int64_t *>(data)) : -1;
        emit chapterIndexChanged();
    }
}

void MpvObject::updateChapters(const mpv_node *node) {
    QVariantList chapters;

    if (node && node->format == MPV_FORMAT_NODE_ARRAY) {
        for (int i = 0; i < node->u.list->num; ++i) {
            const mpv_node &chapterNode = node->u.list->values[i];

            if (chapterNode.format != MPV_FORMAT_NODE_MAP) {
                continue;
            }

            QVariantMap chapter{{QStringLiteral("title"), QString()}, {QStringLiteral("time"), 0.0}};

            for (int j = 0; j < chapterNode.u.list->num; ++j) {
                const char *key = chapterNode.u.list->keys[j];
                const mpv_node &value = chapterNode.u.list->values[j];

                if (std::strcmp(key, "title") == 0) {
                    chapter.insert(QStringLiteral("title"), nodeString(value));
                } else if (std::strcmp(key, "time") == 0 && value.format == MPV_FORMAT_DOUBLE) {
                    chapter.insert(QStringLiteral("time"), value.u.double_);
                }
            }

            chapters.append(chapter);
        }
    }

    if (chapters != m_chapters) {
        m_chapters = chapters;
        emit chaptersChanged();
    }
}

void MpvObject::updateAudioTracks() {
    mpv_node node;

    if (mpv_get_property(m_handle.get(), "track-list", MPV_FORMAT_NODE, &node) < 0) {
        return;
    }

    QVariantList tracks;

    if (node.format == MPV_FORMAT_NODE_ARRAY) {
        for (int i = 0; i < node.u.list->num; ++i) {
            const mpv_node &trackNode = node.u.list->values[i];

            if (trackNode.format != MPV_FORMAT_NODE_MAP) {
                continue;
            }

            QVariantMap track;
            QString type;

            for (int j = 0; j < trackNode.u.list->num; ++j) {
                const char *key = trackNode.u.list->keys[j];
                const mpv_node &value = trackNode.u.list->values[j];

                if (std::strcmp(key, "type") == 0) {
                    type = nodeString(value);
                } else if (std::strcmp(key, "id") == 0 && value.format == MPV_FORMAT_INT64) {
                    track.insert(QStringLiteral("id"), int(value.u.int64));
                } else if (std::strcmp(key, "lang") == 0) {
                    track.insert(QStringLiteral("language"), nodeString(value));
                } else if (std::strcmp(key, "title") == 0) {
                    track.insert(QStringLiteral("title"), nodeString(value));
                } else if (std::strcmp(key, "codec") == 0) {
                    track.insert(QStringLiteral("codec"), nodeString(value));
                } else if (std::strcmp(key, "demux-channel-count") == 0 && value.format == MPV_FORMAT_INT64) {
                    track.insert(QStringLiteral("channelCount"), int(value.u.int64));
                } else if (std::strcmp(key, "default") == 0 && value.format == MPV_FORMAT_FLAG) {
                    track.insert(QStringLiteral("isDefault"), value.u.flag != 0);
                }
            }

            if (type == u"audio") {
                tracks.append(track);
            }
        }
    }

    mpv_free_node_contents(&node);

    if (tracks != m_audioTracks) {
        m_audioTracks = tracks;
        emit audioTracksChanged();
    }
}

void MpvObject::command(const QStringList &arguments) {
    QList<QByteArray> utf8Arguments;
    std::vector<const char *> pointers;

    for (const QString &argument : arguments) {
        utf8Arguments.append(argument.toUtf8());
    }

    for (const QByteArray &argument : utf8Arguments) {
        pointers.push_back(argument.constData());
    }

    pointers.push_back(nullptr);
    mpv_command(m_handle.get(), pointers.data());
}

void MpvObject::setOption(const char *name, const QString &value) {
    mpv_set_option_string(m_handle.get(), name, value.toUtf8().constData());
}

void MpvObject::setMpvProperty(const char *name, const QString &value) {
    mpv_set_property_string(m_handle.get(), name, value.toUtf8().constData());
}

#include "MpvObject.moc"
