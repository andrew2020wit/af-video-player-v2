#pragma once

#include "FrameGrabber.h"

#include <QCache>
#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>

#include <memory>

// Frames over the progress bar (I-73): "image://frame/<seconds>" is the keyframe at or before that time in the
// open video. Images are made on Qt's image loading thread; recent ones are cached.
class FramePreviewProvider : public QQuickImageProvider {
public:
    static constexpr int frameWidth = 320;

    FramePreviewProvider();

    // The open video; empty when none is open.
    void setVideo(const QString &path);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    QMutex m_mutex;
    QString m_path;
    std::unique_ptr<afvp::FrameGrabber> m_grabber;
    // By the time in milliseconds; the cost is the image size in KiB.
    QCache<qint64, QImage> m_cache;
};
