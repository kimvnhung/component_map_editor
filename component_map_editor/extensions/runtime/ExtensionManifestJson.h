#ifndef EXTENSIONMANIFESTJSON_H
#define EXTENSIONMANIFESTJSON_H

#include <QByteArray>
#include <QString>

#include "extensions/contracts/ExtensionManifest.h"

class ExtensionManifestJson
{
public:
    static bool parseBytes(const QByteArray &jsonBytes,
                           ExtensionManifest *manifest,
                           QString *error = nullptr);

    static bool parseFile(const QString &filePath,
                          ExtensionManifest *manifest,
                          QString *error = nullptr);
};

#endif // EXTENSIONMANIFESTJSON_H
