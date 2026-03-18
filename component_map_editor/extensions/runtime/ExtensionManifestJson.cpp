#include "ExtensionManifestJson.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QString requireString(const QJsonObject &obj, const QString &key, QString *error)
{
    const QJsonValue value = obj.value(key);
    if (!value.isString() || value.toString().trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("Manifest key '%1' must be a non-empty string.").arg(key);
        }
        return QString();
    }
    return value.toString().trimmed();
}

QStringList readStringArray(const QJsonObject &obj, const QString &key, QString *error)
{
    QStringList values;
    if (!obj.contains(key)) {
        return values;
    }

    const QJsonValue value = obj.value(key);
    if (!value.isArray()) {
        if (error) {
            *error = QStringLiteral("Manifest key '%1' must be an array of strings.").arg(key);
        }
        return {};
    }

    const QJsonArray array = value.toArray();
    for (int i = 0; i < array.size(); ++i) {
        const QJsonValue item = array.at(i);
        if (!item.isString() || item.toString().trimmed().isEmpty()) {
            if (error) {
                *error = QStringLiteral("Manifest key '%1' contains an invalid item at index %2.")
                             .arg(key, QString::number(i));
            }
            return {};
        }
        values.append(item.toString().trimmed());
    }

    return values;
}

} // namespace

bool ExtensionManifestJson::parseBytes(const QByteArray &jsonBytes,
                                       ExtensionManifest *manifest,
                                       QString *error)
{
    if (!manifest) {
        if (error) {
            *error = QStringLiteral("Manifest output pointer is null.");
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) {
            *error = QStringLiteral("Invalid JSON manifest: %1").arg(parseError.errorString());
        }
        return false;
    }

    const QJsonObject obj = doc.object();
    ExtensionManifest parsed;

    parsed.extensionId = requireString(obj, QStringLiteral("extensionId"), error);
    if (parsed.extensionId.isEmpty()) {
        return false;
    }

    parsed.displayName = requireString(obj, QStringLiteral("displayName"), error);
    if (parsed.displayName.isEmpty()) {
        return false;
    }

    parsed.extensionVersion = requireString(obj, QStringLiteral("extensionVersion"), error);
    if (parsed.extensionVersion.isEmpty()) {
        return false;
    }

    bool minOk = false;
    bool maxOk = false;
    parsed.minCoreApi = ExtensionApiVersion::parse(requireString(obj, QStringLiteral("minCoreApi"), error), &minOk);
    if (!minOk) {
        if (error) {
            *error = QStringLiteral("Manifest key 'minCoreApi' must be semver 'major.minor.patch'.");
        }
        return false;
    }

    parsed.maxCoreApi = ExtensionApiVersion::parse(requireString(obj, QStringLiteral("maxCoreApi"), error), &maxOk);
    if (!maxOk) {
        if (error) {
            *error = QStringLiteral("Manifest key 'maxCoreApi' must be semver 'major.minor.patch'.");
        }
        return false;
    }

    parsed.capabilities = readStringArray(obj, QStringLiteral("capabilities"), error);
    if (error && !error->isEmpty()) {
        return false;
    }

    parsed.dependencies = readStringArray(obj, QStringLiteral("dependencies"), error);
    if (error && !error->isEmpty()) {
        return false;
    }

    if (obj.contains(QStringLiteral("metadata"))) {
        const QJsonValue metadata = obj.value(QStringLiteral("metadata"));
        if (!metadata.isObject()) {
            if (error) {
                *error = QStringLiteral("Manifest key 'metadata' must be a JSON object.");
            }
            return false;
        }
        parsed.metadata = metadata.toObject().toVariantMap();
    }

    QString validationError;
    if (!parsed.isValid(&validationError)) {
        if (error) {
            *error = validationError;
        }
        return false;
    }

    *manifest = parsed;
    return true;
}

bool ExtensionManifestJson::parseFile(const QString &filePath,
                                      ExtensionManifest *manifest,
                                      QString *error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QStringLiteral("Unable to open manifest file '%1': %2")
                         .arg(filePath, file.errorString());
        }
        return false;
    }

    return parseBytes(file.readAll(), manifest, error);
}
