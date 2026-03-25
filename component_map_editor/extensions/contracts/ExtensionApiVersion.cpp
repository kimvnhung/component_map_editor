#include "ExtensionApiVersion.h"

#include <QStringList>

bool ExtensionApiVersion::isValid() const
{
    return major >= 0 && minor >= 0 && patch >= 0;
}

QString ExtensionApiVersion::toString() const
{
    return QStringLiteral("%1.%2.%3").arg(major).arg(minor).arg(patch);
}

ExtensionApiVersion ExtensionApiVersion::parse(const QString &value, bool *ok)
{
    ExtensionApiVersion parsed;
    const QStringList parts = value.trimmed().split('.');
    if (parts.size() != 3) {
        if (ok) {
            *ok = false;
        }
        return parsed;
    }

    bool majorOk = false;
    bool minorOk = false;
    bool patchOk = false;
    parsed.major = parts.at(0).toInt(&majorOk);
    parsed.minor = parts.at(1).toInt(&minorOk);
    parsed.patch = parts.at(2).toInt(&patchOk);

    const bool valid = majorOk && minorOk && patchOk && parsed.isValid();
    if (ok) {
        *ok = valid;
    }
    if (!valid) {
        return ExtensionApiVersion{};
    }
    return parsed;
}

bool operator==(const ExtensionApiVersion &lhs, const ExtensionApiVersion &rhs)
{
    return lhs.major == rhs.major && lhs.minor == rhs.minor && lhs.patch == rhs.patch;
}

bool operator!=(const ExtensionApiVersion &lhs, const ExtensionApiVersion &rhs)
{
    return !(lhs == rhs);
}

bool operator<(const ExtensionApiVersion &lhs, const ExtensionApiVersion &rhs)
{
    if (lhs.major != rhs.major) {
        return lhs.major < rhs.major;
    }
    if (lhs.minor != rhs.minor) {
        return lhs.minor < rhs.minor;
    }
    return lhs.patch < rhs.patch;
}

bool operator<=(const ExtensionApiVersion &lhs, const ExtensionApiVersion &rhs)
{
    return (lhs < rhs) || (lhs == rhs);
}

bool operator>(const ExtensionApiVersion &lhs, const ExtensionApiVersion &rhs)
{
    return rhs < lhs;
}

bool operator>=(const ExtensionApiVersion &lhs, const ExtensionApiVersion &rhs)
{
    return (lhs > rhs) || (lhs == rhs);
}

bool ExtensionCompatibilityReport::compatible() const
{
    return status == ExtensionCompatibilityStatus::Compatible;
}

ExtensionCompatibilityReport evaluateCompatibility(const ExtensionApiVersion &coreApiVersion,
                                                   const ExtensionApiVersion &minSupported,
                                                   const ExtensionApiVersion &maxSupported)
{
    if (!coreApiVersion.isValid() || !minSupported.isValid() || !maxSupported.isValid() || maxSupported < minSupported) {
        return {ExtensionCompatibilityStatus::InvalidInput,
                QStringLiteral("Invalid API version constraints.")};
    }

    if (coreApiVersion.major < minSupported.major || coreApiVersion < minSupported) {
        return {ExtensionCompatibilityStatus::CoreTooOld,
                QStringLiteral("Core API %1 is older than required minimum %2.")
                    .arg(coreApiVersion.toString(), minSupported.toString())};
    }

    if (coreApiVersion.major > maxSupported.major) {
        return {ExtensionCompatibilityStatus::CoreTooNewMajor,
                QStringLiteral("Core API major %1 is newer than supported major %2.")
                    .arg(QString::number(coreApiVersion.major), QString::number(maxSupported.major))};
    }

    if (coreApiVersion > maxSupported) {
        return {ExtensionCompatibilityStatus::OutsideSupportedRange,
                QStringLiteral("Core API %1 is outside supported range [%2, %3].")
                    .arg(coreApiVersion.toString(), minSupported.toString(), maxSupported.toString())};
    }

    return {ExtensionCompatibilityStatus::Compatible,
            QStringLiteral("Compatible with core API %1.").arg(coreApiVersion.toString())};
}
