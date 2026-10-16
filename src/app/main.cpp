#include "AppController.h"
#include "FramePreviewProvider.h"
#include "HandCursorFilter.h"
#include "SingleInstance.h"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStyleHints>

#include <clocale>

int main(int argc, char *argv[]) {
    // The video item renders through libmpv's OpenGL API.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QGuiApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("af-video-player"));
    QCoreApplication::setApplicationVersion(QStringLiteral(AFVP_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("af-video-player"));

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    // The window icon (I-84); on Windows and macOS it comes from the executable and the bundle.
    QIcon icon;

    for (const int size : {16, 24, 32, 48, 64, 128, 256}) {
        icon.addFile(QStringLiteral(":/icons/%1.png").arg(size));
    }

    QGuiApplication::setWindowIcon(icon);
#endif

    // libmpv requires the C numeric locale.
    std::setlocale(LC_NUMERIC, "C");

    // No dark theme (SPEC 3.5): the window frame stays light too; the palette is set in Main.qml.
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    HandCursorFilter handCursorFilter;
    application.installEventFilter(&handCursorFilter);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("AF Video Player"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("Video file to open."));
    parser.process(application);

    // "Open with" and the command line (I-05).
    const QStringList files = parser.positionalArguments();
    const QString file = files.isEmpty() ? QString() : files.front();

    // A running player opens the file instead (I-74).
    if (SingleInstance::sendToRunning(file)) {
        return 0;
    }

    SingleInstance singleInstance;
    singleInstance.listen();

    QQmlApplicationEngine engine;
    // Owned by the engine.
    auto *framePreview = new FramePreviewProvider;
    engine.addImageProvider(QStringLiteral("frame"), framePreview);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &application, []() { QCoreApplication::exit(1); },
        Qt::QueuedConnection);
    engine.loadFromModule("AfVideoPlayer", "Main");

    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    auto *app = engine.singletonInstance<AppController *>("AfVideoPlayer", "App");
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().front());

    QObject::connect(app, &AppController::videoChanged, app,
                     [app, framePreview]() { framePreview->setVideo(app->videoPath()); });

    if (!file.isEmpty()) {
        app->openVideo(file);
    }

    QObject::connect(&singleInstance, &SingleInstance::messageReceived, app, [app, window](const QString &path) {
        if (window) {
            window->setWindowStates(window->windowStates() & ~Qt::WindowMinimized);
            window->raise();
            window->requestActivate();
        }

        if (!path.isEmpty()) {
            app->requestOpen(path);
        }
    });

    return application.exec();
}
