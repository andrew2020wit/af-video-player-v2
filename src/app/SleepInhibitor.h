#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

// Keeps the display on and the system awake while active (I-69): SetThreadExecutionState on Windows,
// IOKit power assertions on macOS, org.freedesktop.ScreenSaver over D-Bus on Linux.
// With the render API libmpv's own "stop-screensaver" option has no effect.
class SleepInhibitor : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)

public:
    explicit SleepInhibitor(QObject *parent = nullptr);
    ~SleepInhibitor() override;

    bool active() const;
    void setActive(bool active);

signals:
    void activeChanged();

private:
    void inhibit();
    void release();

    bool m_active = false;
    // macOS assertion id or D-Bus cookie; 0 when nothing is held.
    quint32 m_handle = 0;
};
