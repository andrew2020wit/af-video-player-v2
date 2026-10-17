#include "BatchExporter.h"

#include "AutoExport.h"
#include "MediaFiles.h"
#include "SubtitleExtractor.h"

#include <QDir>
#include <QFileInfo>

BatchExporter::BatchExporter(QObject *parent) : QObject(parent) {}

BatchExporter::~BatchExporter() {
    m_cancel = true;
    join();
}

bool BatchExporter::running() const {
    return m_running;
}

QString BatchExporter::folder() const {
    return m_folder;
}

int BatchExporter::videoCount() const {
    return m_videoCount;
}

int BatchExporter::videoNumber() const {
    return m_videoNumber;
}

QString BatchExporter::videoName() const {
    return m_videoName;
}

int BatchExporter::exportedCount() const {
    return m_exportedCount;
}

double BatchExporter::progress() const {
    return m_progress;
}

void BatchExporter::start(const QString &folder) {
    if (m_running) {
        return;
    }

    join();

    const QStringList videos = afvp::videoFilesInFolder(folder);
    m_cancel = false;
    m_running = true;
    m_folder = QDir::toNativeSeparators(QDir::cleanPath(folder));
    m_videoCount = int(videos.size());
    m_videoNumber = 0;
    m_videoName.clear();
    m_exportedCount = 0;
    m_progress = 0;
    emit stateChanged();

    m_thread = std::thread([this, videos]() {
        const auto count = double(videos.size());
        QStringList errors;
        int exported = 0;

        for (qsizetype i = 0; i < videos.size() && !m_cancel; ++i) {
            const QFileInfo video(videos[i]);
            const QString name = video.fileName();
            QMetaObject::invokeMethod(
                this,
                [this, i, name, count]() {
                    m_videoNumber = int(i) + 1;
                    m_videoName = name;
                    m_progress = double(i) / count;
                    emit stateChanged();
                },
                Qt::QueuedConnection);

            QString error;
            const QList<afvp::EmbeddedSubtitleTrack> tracks = afvp::probeSubtitleTracks(video.filePath(), &error);

            if (!error.isEmpty()) {
                errors.append(QStringLiteral("%1: %2").arg(name, error));
                continue;
            }

            // Files exported earlier are not written again.
            const QDir dir = video.absoluteDir();
            const QList<afvp::BatchExportItem> items = afvp::chooseBatchExportTracks(
                video.completeBaseName(), tracks, [&dir](const QString &fileName) { return dir.exists(fileName); });

            if (items.isEmpty()) {
                continue;
            }

            QList<afvp::ExtractionTarget> targets;

            for (const afvp::BatchExportItem &item : items) {
                targets.append({item.track.streamIndex, dir.absoluteFilePath(item.fileName)});
            }

            const bool ok = afvp::extractSubtitleTracks(
                video.filePath(), targets,
                [this, i, count](double progress) {
                    QMetaObject::invokeMethod(
                        this,
                        [this, i, count, progress]() {
                            m_progress = (double(i) + progress) / count;
                            emit stateChanged();
                        },
                        Qt::QueuedConnection);
                },
                m_cancel, &error);

            if (ok) {
                exported += int(targets.size());
                QMetaObject::invokeMethod(
                    this,
                    [this, exported]() {
                        m_exportedCount = exported;
                        emit stateChanged();
                    },
                    Qt::QueuedConnection);
            } else if (!m_cancel) {
                errors.append(QStringLiteral("%1: %2").arg(name, error));
            }
        }

        const bool cancelled = m_cancel.load();

        QMetaObject::invokeMethod(
            this,
            [this, cancelled, errors]() {
                m_running = false;
                m_progress = cancelled ? m_progress : 1.0;
                emit stateChanged();
                emit finished(cancelled, errors);
            },
            Qt::QueuedConnection);
    });
}

void BatchExporter::cancel() {
    m_cancel = true;
}

void BatchExporter::join() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}
