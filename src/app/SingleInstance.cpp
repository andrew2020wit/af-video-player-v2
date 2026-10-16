#include "SingleInstance.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QLocalSocket>

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

namespace {

constexpr int timeoutMs = 1000;

// Per user: local socket names on Windows (named pipes) are visible to all sessions of the machine.
QString serverName() {
    QString user = qEnvironmentVariable("USERNAME");

    if (user.isEmpty()) {
        user = qEnvironmentVariable("USER");
    }

    return QCoreApplication::applicationName() + QLatin1Char('-') + user;
}

} // namespace

SingleInstance::SingleInstance(QObject *parent) : QObject(parent) {
    connect(&m_server, &QLocalServer::newConnection, this, [this]() {
        while (QLocalSocket *socket = m_server.nextPendingConnection()) {
            readConnection(socket);
        }
    });
}

bool SingleInstance::sendToRunning(const QString &file) {
    QLocalSocket socket;
    socket.connectToServer(serverName());

    if (!socket.waitForConnected(timeoutMs)) {
        return false;
    }

#if defined(Q_OS_WIN)
    // The started process owns the foreground; this lets the running one bring its window to the front.
    AllowSetForegroundWindow(ASFW_ANY);
#endif

    const QString path = file.isEmpty() ? QString() : QFileInfo(file).absoluteFilePath();
    socket.write(path.toUtf8());
    socket.waitForBytesWritten(timeoutMs);
    socket.disconnectFromServer();

    if (socket.state() != QLocalSocket::UnconnectedState) {
        socket.waitForDisconnected(timeoutMs);
    }

    return true;
}

void SingleInstance::listen() {
    // A socket file left by a crashed instance (Unix) would make listen() fail.
    QLocalServer::removeServer(serverName());
    m_server.setSocketOptions(QLocalServer::UserAccessOption);

    if (!m_server.listen(serverName())) {
        qWarning("Single instance server: %s", qPrintable(m_server.errorString()));
    }
}

void SingleInstance::readConnection(QLocalSocket *socket) {
    // The message is complete when the sender disconnects.
    const auto finish = [this, socket]() {
        const QString file = QString::fromUtf8(socket->readAll());
        socket->deleteLater();
        emit messageReceived(file);
    };

    if (socket->state() == QLocalSocket::UnconnectedState) {
        finish();
    } else {
        connect(socket, &QLocalSocket::disconnected, this, finish);
    }
}
