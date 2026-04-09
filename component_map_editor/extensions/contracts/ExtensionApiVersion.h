#ifndef EXTENSIONAPIVERSION_H
#define EXTENSIONAPIVERSION_H

#include <QString>

struct ExtensionApiVersion
{
    int major = 0;
    int minor = 0;
    int patch = 0;

    bool isValid() const;
    QString toString() const;

    static ExtensionApiVersion parse(const QString &value, bool *ok = nullptr);

    friend bool operator==(const ExtensionApiVersion &lhs, const ExtensionApiVersion &rhs);
    friend bool operator!=(const ExtensionApiVersion &lhs, const ExtensionApiVersion &rhs);
    friend bool operator<(const ExtensionApiVersion &lhs, const ExtensionApiVersion &rhs);
    friend bool operator<=(const ExtensionApiVersion &lhs, const ExtensionApiVersion &rhs);
    friend bool operator>(const ExtensionApiVersion &lhs, const ExtensionApiVersion &rhs);
    friend bool operator>=(const ExtensionApiVersion &lhs, const ExtensionApiVersion &rhs);
};

enum class ExtensionCompatibilityStatus {
    Compatible,
    InvalidInput,
    CoreTooOld,
    CoreTooNewMajor,
    OutsideSupportedRange
};

struct ExtensionCompatibilityReport
{
    ExtensionCompatibilityStatus status = ExtensionCompatibilityStatus::InvalidInput;
    QString message;

    bool compatible() const;
};

ExtensionCompatibilityReport evaluateCompatibility(const ExtensionApiVersion &coreApiVersion,
                                                   const ExtensionApiVersion &minSupported,
                                                   const ExtensionApiVersion &maxSupported);

#endif // EXTENSIONAPIVERSION_H
