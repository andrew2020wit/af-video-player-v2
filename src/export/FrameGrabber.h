#pragma once

#include <QByteArray>
#include <QString>

#include <memory>

namespace afvp {

// A decoded video frame: 32-bit pixels 0xffRRGGBB in native byte order (QImage::Format_RGB32), rows without
// padding.
struct VideoFrame {
    int width = 0;
    int height = 0;
    QByteArray pixels;

    bool isNull() const {
        return pixels.isEmpty();
    }
};

// Frames of a video file for the preview over the progress bar (I-73). Decodes the keyframe at or before the
// requested time, which needs no decoding of the frames after it and so is fast. Not thread-safe.
class FrameGrabber {
public:
    explicit FrameGrabber(const QString &videoPath);
    ~FrameGrabber();

    FrameGrabber(const FrameGrabber &) = delete;
    FrameGrabber &operator=(const FrameGrabber &) = delete;

    // A frame scaled to the width (the height follows the display aspect ratio); null when the file has no
    // video or cannot be decoded.
    VideoFrame grab(double seconds, int width);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace afvp
