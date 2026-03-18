#include "ExtensionManifest.h"

bool ExtensionManifest::isValid(QString *error) const
{
    if (extensionId.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("Manifest extensionId is required.");
        }
        return false;
    }

    if (displayName.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("Manifest displayName is required.");
        }
        return false;
    }

    if (!minCoreApi.isValid() || !maxCoreApi.isValid() || maxCoreApi < minCoreApi) {
        if (error) {
            *error = QStringLiteral("Manifest core API range is invalid.");
        }
        return false;
    }

    if (extensionVersion.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("Manifest extensionVersion is required.");
        }
        return false;
    }

    for (const QString &dependency : dependencies) {
        if (dependency.trimmed().isEmpty()) {
            if (error) {
                *error = QStringLiteral("Manifest dependencies must not contain empty values.");
            }
            return false;
        }
        if (dependency.trimmed() == extensionId.trimmed()) {
            if (error) {
                *error = QStringLiteral("Manifest dependency cannot reference itself: %1").arg(extensionId);
            }
            return false;
        }
    }

    return true;
}
