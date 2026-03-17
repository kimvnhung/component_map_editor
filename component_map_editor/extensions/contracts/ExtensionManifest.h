#ifndef EXTENSIONMANIFEST_H
#define EXTENSIONMANIFEST_H

#include <QString>
#include <QStringList>

#include "ExtensionApiVersion.h"

struct ExtensionManifest
{
    QString extensionId;
    QString displayName;
    QString extensionVersion;

    ExtensionApiVersion minCoreApi;
    ExtensionApiVersion maxCoreApi;

    QStringList capabilities;

    bool isValid(QString *error = nullptr) const;
};

#endif // EXTENSIONMANIFEST_H
