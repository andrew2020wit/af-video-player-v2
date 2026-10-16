#include "JsonFile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>

namespace afvp {

std::optional<QJsonObject> readJsonObject(const QString &path) {
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);

    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return std::nullopt;
    }

    return document.object();
}

bool writeJsonObject(const QString &path, const QJsonObject &object) {
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        return false;
    }

    QSaveFile file(path);

    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));

    return file.commit();
}

} // namespace afvp
