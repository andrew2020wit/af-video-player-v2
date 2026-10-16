#include "SleepInhibitor.h"

#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_MACOS)
#include <IOKit/pwr_mgt/IOPMLib.h>
#elif defined(Q_OS_LINUX)
#include <QDBusInterface>
#include <QDBusReply>
#endif

namespace {

#if defined(Q_OS_LINUX)
const QString screenSaverService = QStringLiteral("org.freedesktop.ScreenSaver");
const QString screenSaverPath = QStringLiteral("/org/freedesktop/ScreenSaver");
#endif

} // namespace

SleepInhibitor::SleepInhibitor(QObject *parent) : QObject(parent) {}

SleepInhibitor::~SleepInhibitor() {
    if (m_active) {
        release();
    }
}

bool SleepInhibitor::active() const {
    return m_active;
}

void SleepInhibitor::setActive(bool active) {
    if (active == m_active) {
        return;
    }

    m_active = active;

    if (active) {
        inhibit();
    } else {
        release();
    }

    emit activeChanged();
}

void SleepInhibitor::inhibit() {
#if defined(Q_OS_WIN)
    SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
#elif defined(Q_OS_MACOS)
    IOPMAssertionID id = 0;

    if (IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleDisplaySleep, kIOPMAssertionLevelOn,
                                    CFSTR("AF Video Player: video playback"), &id) == kIOReturnSuccess) {
        m_handle = id;
    }
#elif defined(Q_OS_LINUX)
    QDBusInterface interface(screenSaverService, screenSaverPath, screenSaverService);

    if (interface.isValid()) {
        const QDBusReply<quint32> reply =
            interface.call(QStringLiteral("Inhibit"), QStringLiteral("AF Video Player"), QStringLiteral("Video playback"));

        if (reply.isValid()) {
            m_handle = reply.value();
        }
    }
#endif
}

void SleepInhibitor::release() {
#if defined(Q_OS_WIN)
    SetThreadExecutionState(ES_CONTINUOUS);
#elif defined(Q_OS_MACOS)
    if (m_handle != 0) {
        IOPMAssertionRelease(m_handle);
    }
#elif defined(Q_OS_LINUX)
    if (m_handle != 0) {
        QDBusInterface interface(screenSaverService, screenSaverPath, screenSaverService);
        interface.call(QStringLiteral("UnInhibit"), m_handle);
    }
#endif

    m_handle = 0;
}
