#include "FramePreviewProvider.h"

#include <QMutexLocker>

namespace {

// About a hundred frames.
constexpr int cacheKiB = 32 * 1024;

} // namespace

FramePreviewProvider::FramePreviewProvider()
    : QQuickImageProvider(QQuickImageProvider::Image, QQuickImageProvider::ForceAsynchronousImageLoading),
      m_cache(cacheKiB) {}

void FramePreviewProvider::setVideo(const QString &path) {
    QMutexLocker locker(&m_mutex);

    if (path == m_path) {
        return;
    }

    m_path = path;
    m_grabber.reset();
    m_cache.clear();
}

QImage FramePreviewProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize) {
    Q_UNUSED(requestedSize)

    QMutexLocker locker(&m_mutex);
    bool ok = false;
    const double seconds = id.toDouble(&ok);
    QImage image;

    if (ok && !m_path.isEmpty()) {
        const qint64 key = qint64(seconds * 1000.0);

        if (const QImage *cached = m_cache.object(key)) {
            image = *cached;
        } else {
            if (!m_grabber) {
                m_grabber = std::make_unique<afvp::FrameGrabber>(m_path);
            }

            const afvp::VideoFrame frame = m_grabber->grab(seconds, frameWidth);

            if (!frame.isNull()) {
                image = QImage(reinterpret_cast<const uchar *>(frame.pixels.constData()), frame.width, frame.height,
                               frame.width * 4, QImage::Format_RGB32)
                            .copy();
                m_cache.insert(key, new QImage(image), int(image.sizeInBytes() / 1024) + 1);
            }
        }
    }

    if (size) {
        *size = image.size();
    }

    return image;
}
