#ifndef RULEDSLTYPES_H
#define RULEDSLTYPES_H

#include <QHash>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct RuleSourceLocation
{
    QString filePath;
    QString jsonPath;
    int line = 0;
    int column = 0;
};

enum class RuleDiagnosticSeverity {
    Warning,
    Error
};

struct RuleDiagnostic
{
    RuleDiagnosticSeverity severity = RuleDiagnosticSeverity::Error;
    RuleSourceLocation location;
    QString message;
};

struct CompiledConnectionRule
{
    QString sourceType;
    QString targetType;
    bool allow = false;
    QString reason;
};

// Typed enum for validation rule kinds (Phase 8: replaces raw string dispatch).
// String literals for rule kinds are confined to RuleCompiler (JSON parsing only).
enum class RuleKind {
    Unknown,         // unrecognized kind — compile error
    ExactlyOneType,  // "exactlyOneType" in JSON
    EndpointExists,  // "endpointExists" in JSON
    NoIsolated       // "noIsolated" in JSON
};

struct CompiledValidationRule
{
    // Supported kinds encoded as typed enum (was: raw QString).
    RuleKind kind = RuleKind::Unknown;
    QString type;
    QString code;
    QString severity;
    QString message;
};

struct CompiledDerivedRule
{
    QString target;
    QString property;
    QVariant value;
};

struct CompiledRuleDescriptor
{
    bool defaultConnectionAllow = true;

    // Fast lookup table: sourceType -> targetType -> rule.
    QHash<QString, QHash<QString, CompiledConnectionRule>> connectionTable;

    QVector<CompiledValidationRule> validationRules;

    // Fast lookup: targetId -> property map
    QHash<QString, QVariantMap> derivedPropertyTable;
};

struct RuleCompileResult
{
    bool ok = false;
    CompiledRuleDescriptor descriptor;
    QVector<RuleDiagnostic> diagnostics;
};

#endif // RULEDSLTYPES_H
