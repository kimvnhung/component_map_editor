// component_map_editor/services/DiagnosticCodeCatalog.h
//
// Static catalog of normalized diagnostic codes shared by the editor and the
// runtime validation layer. Codes are UPPER_SNAKE_CASE literals that are
// emitted as-is (i.e. already normalized); they must not be re-normalized.
//
// The catalog is versioned with the existing protocol version — new codes are
// appended, existing codes are never redefined.
//
// `normalizeDiagnosticCode` (T002) is the shared code-normalization helper:
// trim, upper-case, collapse every run of [^A-Z0-9_] into a single "_".
// The editor includes this header directly; the runtime subproject cannot see
// it (separate static library with no editor include path), so it carries an
// identical copy of the same function — see the note in DeploymentValidator.cpp.

#pragma once

#include <QString>
#include <QStringList>
#include <QRegularExpression>

namespace cme::editor {

/// Normalize a diagnostic code to stable UPPER_SNAKE_CASE form.
/// Trims, upper-cases, and replaces every run of characters other than
/// A-Z/0-9/_ with a single underscore.
inline QString normalizeDiagnosticCode(const QString &code)
{
    QString normalized = code.trimmed().toUpper();
    normalized.replace(QRegularExpression(QStringLiteral("[^A-Z0-9_]+")), QStringLiteral("_"));
    return normalized;
}

struct DiagnosticCodeCatalog {
    // Editor-internal core codes
    static constexpr char CoreNullGraph[]                  = "CORE_NULL_GRAPH";
    static constexpr char CoreValidationProviderError[]    = "CORE_VALIDATION_PROVIDER_ERROR";

    // Shared editor/runtime codes
    static constexpr char DuplicateComponentId[]           = "DUPLICATE_COMPONENT_ID";
    static constexpr char MissingDescriptor[]               = "MISSING_DESCRIPTOR";
    static constexpr char MissingSchema[]                   = "MISSING_SCHEMA";
    static constexpr char InvalidPortMetadata[]            = "INVALID_PORT_METADATA";
    static constexpr char MissingHandlerCapability[]       = "MISSING_HANDLER_CAPABILITY";
    static constexpr char IncompatibleComponentMetadata[]   = "INCOMPATIBLE_COMPONENT_METADATA";
    static constexpr char ProviderDiscoveryRegistrationFailure[] =
        "PROVIDER_DISCOVERY_REGISTRATION_FAILURE";
    static constexpr char ComponentPropertyRequired[]       = "COMPONENT_PROPERTY_REQUIRED";

    /// All catalog codes, in declaration order.
    static QStringList all()
    {
        return QStringList{
            QString::fromUtf8(CoreNullGraph),
            QString::fromUtf8(CoreValidationProviderError),
            QString::fromUtf8(DuplicateComponentId),
            QString::fromUtf8(MissingDescriptor),
            QString::fromUtf8(MissingSchema),
            QString::fromUtf8(InvalidPortMetadata),
            QString::fromUtf8(MissingHandlerCapability),
            QString::fromUtf8(IncompatibleComponentMetadata),
            QString::fromUtf8(ProviderDiscoveryRegistrationFailure),
            QString::fromUtf8(ComponentPropertyRequired),
        };
    }
};

} // namespace cme::editor
