#include "FullScreenPlacement.h"

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

namespace {

#if defined(Q_OS_WIN)
// Window placement uses workspace coordinates: relative to the work area of the window's monitor.
QPoint workspaceOffset(HWND hwnd) {
    MONITORINFO info{};
    info.cbSize = sizeof(info);

    if (!GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &info)) {
        return {};
    }

    return {info.rcWork.left - info.rcMonitor.left, info.rcWork.top - info.rcMonitor.top};
}

QRect toQRect(const RECT &rect) {
    return {QPoint(rect.left, rect.top), QPoint(rect.right - 1, rect.bottom - 1)};
}

RECT toRECT(const QRect &rect) {
    return {rect.left(), rect.top(), rect.right() + 1, rect.bottom() + 1};
}
#endif

} // namespace

FullScreenPlacement::FullScreenPlacement(QObject *parent) : QObject(parent) {}

void FullScreenPlacement::enter(QWindow *window) {
#if defined(Q_OS_WIN)
    if (!window || m_normalPlace) {
        return;
    }

    const auto hwnd = reinterpret_cast<HWND>(window->winId());
    WINDOWPLACEMENT placement{};
    placement.length = sizeof(placement);
    RECT frame{};

    if (!GetWindowPlacement(hwnd, &placement) || placement.showCmd != SW_SHOWMAXIMIZED
        || !GetWindowRect(hwnd, &frame)) {
        return;
    }

    const QRect normalPlace = toQRect(placement.rcNormalPosition);
    placement.rcNormalPosition = toRECT(toQRect(frame).translated(-workspaceOffset(hwnd)));

    if (SetWindowPlacement(hwnd, &placement)) {
        m_normalPlace = normalPlace;
    }
#else
    Q_UNUSED(window)
#endif
}

void FullScreenPlacement::leave(QWindow *window) {
#if defined(Q_OS_WIN)
    if (!window || !m_normalPlace) {
        return;
    }

    const auto hwnd = reinterpret_cast<HWND>(window->winId());
    WINDOWPLACEMENT placement{};
    placement.length = sizeof(placement);

    if (GetWindowPlacement(hwnd, &placement)) {
        placement.rcNormalPosition = toRECT(*m_normalPlace);
        SetWindowPlacement(hwnd, &placement);
    }

    m_normalPlace.reset();
#else
    Q_UNUSED(window)
#endif
}
