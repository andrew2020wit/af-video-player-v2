#pragma once

#include <QObject>
#include <QRect>
#include <QWindow>
#include <QtQml/qqmlregistration.h>

#include <optional>

// Leaving full screen straight into a maximized window (I-30). On Windows Qt first gives the window its normal
// geometry and then maximizes it again with the system animation. While a maximized window is in full screen, its
// normal place is its maximized one, so it goes to the maximized size at once; the real normal place is put back
// after. Elsewhere it does nothing.
class FullScreenPlacement : public QObject {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit FullScreenPlacement(QObject *parent = nullptr);

    // Before showFullScreen() of a maximized window.
    Q_INVOKABLE void enter(QWindow *window);
    // After the window has left full screen.
    Q_INVOKABLE void leave(QWindow *window);

private:
    // The real normal place in workspace coordinates while the maximized one stands in for it.
    std::optional<QRect> m_normalPlace;
};
