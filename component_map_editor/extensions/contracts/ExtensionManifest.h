#ifndef EXTENSIONMANIFEST_H
#define EXTENSIONMANIFEST_H

#include <QString>
#include <QStringList>
#include <QVariantMap>

#include "ExtensionApiVersion.h"
#include "extensions/common.h"

struct ExtensionManifest
{
    QString extensionId;
    QString displayName;
    QString extensionVersion;

    ExtensionApiVersion minCoreApi;
    ExtensionApiVersion maxCoreApi;

    extensions::Capability capabilities;
    QStringList dependencies;
    QVariantMap metadata;

    bool isValid(QString *error = nullptr) const;
};

#endif // EXTENSIONMANIFEST_H
