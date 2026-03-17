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

    return true;
}
