#pragma once

#include <QLocalServer>
#include <QObject>

// One running player per user (I-74). The first instance listens on a local socket; a later one sends it the
// file to open (an empty message only activates the window) and exits.
class SingleInstance : public QObject {
    Q_OBJECT

public:
    explicit SingleInstance(QObject *parent = nullptr);

    // Passes the file to a running instance. Returns false when there is none.
    static bool sendToRunning(const QString &file);
    // Starts listening for later instances.
    void listen();

signals:
    // file: an absolute path, or empty when another instance was started without a file.
    void messageReceived(const QString &file);

private:
    void readConnection(QLocalSocket *socket);

    QLocalServer m_server;
};
