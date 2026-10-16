#pragma once

#include <QJsonObject>
#include <QString>

#include <optional>

namespace afvp {

// Reads a JSON object; nullopt when the file is missing or not a valid JSON object.
std::optional<QJsonObject> readJsonObject(const QString &path);

// Writes atomically (QSaveFile: temporary file + rename), creating the directory when needed.
bool writeJsonObject(const QString &path, const QJsonObject &object);

} // namespace afvp
