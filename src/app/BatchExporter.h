#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

#include <atomic>
#include <thread>

// Batch export of embedded subtitles (I-87): every video directly in a folder, one after another, in the
// background. Independent of the export of the open video (I-08, I-85).
class BatchExporter : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by App.batchExport")

    Q_PROPERTY(bool running READ running NOTIFY stateChanged)
    Q_PROPERTY(QString folder READ folder NOTIFY stateChanged)
    Q_PROPERTY(int videoCount READ videoCount NOTIFY stateChanged)
    // The video being exported: its 1-based number (0 before the first one) and file name.
    Q_PROPERTY(int videoNumber READ videoNumber NOTIFY stateChanged)
    Q_PROPERTY(QString videoName READ videoName NOTIFY stateChanged)
    // Subtitle files written so far.
    Q_PROPERTY(int exportedCount READ exportedCount NOTIFY stateChanged)
    // Of the whole folder, 0..1.
    Q_PROPERTY(double progress READ progress NOTIFY stateChanged)

public:
    explicit BatchExporter(QObject *parent = nullptr);
    ~BatchExporter() override;

    bool running() const;
    QString folder() const;
    int videoCount() const;
    int videoNumber() const;
    QString videoName() const;
    int exportedCount() const;
    double progress() const;

    // Does nothing while an export is running.
    Q_INVOKABLE void start(const QString &folder);
    // Stops after the current file is abandoned: the folder keeps the files of the finished videos.
    Q_INVOKABLE void cancel();

signals:
    void stateChanged();
    // errors: "<video file name>: <error>" for each video that failed.
    void finished(bool cancelled, const QStringList &errors);

private:
    void join();

    std::thread m_thread;
    std::atomic_bool m_cancel = false;
    bool m_running = false;
    QString m_folder;
    int m_videoCount = 0;
    int m_videoNumber = 0;
    QString m_videoName;
    int m_exportedCount = 0;
    double m_progress = 0;
};
