#include "RuleCompiler.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace {

void appendMissingFieldErrorLocal(QVector<RuleDiagnostic> *diagnostics,
                                  const QString &sourcePath,
                                  const QString &jsonPath,
                                  const QString &fieldName)
{
    RuleDiagnostic diagnostic;
    diagnostic.severity = RuleDiagnosticSeverity::Error;
    diagnostic.location.filePath = sourcePath;
    diagnostic.location.jsonPath = jsonPath + QStringLiteral(".") + fieldName;
    diagnostic.message = QStringLiteral("Missing required field '%1'.").arg(fieldName);
    diagnostics->append(diagnostic);
}

void appendTypeErrorLocal(QVector<RuleDiagnostic> *diagnostics,
                          const QString &sourcePath,
                          const QString &jsonPath,
                          const QString &expectedType)
{
    RuleDiagnostic diagnostic;
    diagnostic.severity = RuleDiagnosticSeverity::Error;
    diagnostic.location.filePath = sourcePath;
    diagnostic.location.jsonPath = jsonPath;
    diagnostic.message = QStringLiteral("Type mismatch: expected %1.").arg(expectedType);
    diagnostics->append(diagnostic);
}

QString expectString(const QJsonObject &object,
                     const QString &key,
                     const QString &sourcePath,
                     const QString &jsonPath,
                     QVector<RuleDiagnostic> *diagnostics)
{
    if (!object.contains(key)) {
        appendMissingFieldErrorLocal(diagnostics, sourcePath, jsonPath, key);
        return QString();
    }

    const QJsonValue value = object.value(key);
    if (!value.isString()) {
        appendTypeErrorLocal(diagnostics,
                             sourcePath,
                             jsonPath + QStringLiteral(".") + key,
                             QStringLiteral("string"));
        return QString();
    }

    return value.toString().trimmed();
}

bool expectBool(const QJsonObject &object,
                const QString &key,
                bool defaultValue,
                bool *usedDefault,
                const QString &sourcePath,
                const QString &jsonPath,
                QVector<RuleDiagnostic> *diagnostics)
{
    if (!object.contains(key)) {
        if (usedDefault)
            *usedDefault = true;
        return defaultValue;
    }

    const QJsonValue value = object.value(key);
    if (!value.isBool()) {
        appendTypeErrorLocal(diagnostics,
                             sourcePath,
                             jsonPath + QStringLiteral(".") + key,
                             QStringLiteral("bool"));
        if (usedDefault)
            *usedDefault = true;
        return defaultValue;
    }

    if (usedDefault)
        *usedDefault = false;
    return value.toBool();
}

bool expectArray(const QJsonObject &object,
                 const QString &key,
                 QJsonArray *out,
                 const QString &sourcePath,
                 const QString &jsonPath,
                 QVector<RuleDiagnostic> *diagnostics)
{
    const QJsonValue value = object.value(key);
    if (value.isUndefined()) {
        *out = QJsonArray();
        return true;
    }

    if (!value.isArray()) {
        appendTypeErrorLocal(diagnostics,
                             sourcePath,
                             jsonPath + QStringLiteral(".") + key,
                             QStringLiteral("array"));
        return false;
    }

    *out = value.toArray();
    return true;
}

} // namespace

RuleCompileResult RuleCompiler::compileFromFile(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        RuleCompileResult result;
        result.ok = false;
        result.diagnostics.append(makeError(filePath,
                                            QStringLiteral("$"),
                                            0,
                                            0,
                                            QStringLiteral("Unable to open rules file: %1")
                                                .arg(file.errorString())));
        return result;
    }

    const QByteArray bytes = file.readAll();
    file.close();
    return compileFromJsonText(QString::fromUtf8(bytes), filePath);
}

RuleCompileResult RuleCompiler::compileFromJsonText(const QString &jsonText,
                                                    const QString &sourcePath) const
{
    RuleCompileResult result;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        int line = 0;
        int column = 0;
        offsetToLineColumn(jsonText, parseError.offset, &line, &column);

        result.ok = false;
        result.diagnostics.append(makeError(sourcePath,
                                            QStringLiteral("$"),
                                            line,
                                            column,
                                            QStringLiteral("JSON parse error: %1")
                                                .arg(parseError.errorString())));
        return result;
    }

    if (!doc.isObject()) {
        result.ok = false;
        result.diagnostics.append(makeError(sourcePath,
                                            QStringLiteral("$"),
                                            1,
                                            1,
                                            QStringLiteral("Top-level rules document must be a JSON object.")));
        return result;
    }

    const QJsonObject root = doc.object();

    {
        bool usedDefault = false;
        result.descriptor.defaultConnectionAllow = expectBool(root,
                                                              QStringLiteral("defaultConnectionAllow"),
                                                              true,
                                                              &usedDefault,
                                                              sourcePath,
                                                              QStringLiteral("$"),
                                                              &result.diagnostics);
        Q_UNUSED(usedDefault)
    }

    QJsonArray connections;
    expectArray(root,
                QStringLiteral("connections"),
                &connections,
                sourcePath,
                QStringLiteral("$"),
                &result.diagnostics);

    for (int i = 0; i < connections.size(); ++i) {
        const QString basePath = QStringLiteral("$.connections[%1]").arg(i);
        const QJsonValue entry = connections.at(i);
        if (!entry.isObject()) {
            appendTypeError(&result.diagnostics, sourcePath, basePath, QStringLiteral("object"));
            continue;
        }

        const QJsonObject object = entry.toObject();
        const QString source = expectString(object,
                                            QStringLiteral("source"),
                                            sourcePath,
                                            basePath,
                                            &result.diagnostics);
        const QString target = expectString(object,
                                            QStringLiteral("target"),
                                            sourcePath,
                                            basePath,
                                            &result.diagnostics);
        bool usedDefault = false;
        const bool allow = expectBool(object,
                                      QStringLiteral("allow"),
                                      false,
                                      &usedDefault,
                                      sourcePath,
                                      basePath,
                                      &result.diagnostics);
        const QString reason = object.value(QStringLiteral("reason")).toString();

        if (source.isEmpty() || target.isEmpty())
            continue;

        CompiledConnectionRule compiled;
        compiled.sourceType = source;
        compiled.targetType = target;
        compiled.allow = allow;
        compiled.reason = reason;

        // Optimization pass: direct O(1) source->target table.
        result.descriptor.connectionTable[source].insert(target, compiled);
    }

    QJsonArray validations;
    expectArray(root,
                QStringLiteral("validations"),
                &validations,
                sourcePath,
                QStringLiteral("$"),
                &result.diagnostics);

    for (int i = 0; i < validations.size(); ++i) {
        const QString basePath = QStringLiteral("$.validations[%1]").arg(i);
        const QJsonValue entry = validations.at(i);
        if (!entry.isObject()) {
            appendTypeError(&result.diagnostics, sourcePath, basePath, QStringLiteral("object"));
            continue;
        }

        const QJsonObject object = entry.toObject();
        CompiledValidationRule rule;
        rule.kind = expectString(object,
                                 QStringLiteral("kind"),
                                 sourcePath,
                                 basePath,
                                 &result.diagnostics);
        rule.code = expectString(object,
                                 QStringLiteral("code"),
                                 sourcePath,
                                 basePath,
                                 &result.diagnostics);
        rule.severity = expectString(object,
                                     QStringLiteral("severity"),
                                     sourcePath,
                                     basePath,
                                     &result.diagnostics);
        rule.message = expectString(object,
                                    QStringLiteral("message"),
                                    sourcePath,
                                    basePath,
                                    &result.diagnostics);

        if (rule.kind == QStringLiteral("exactlyOneType")) {
            rule.type = expectString(object,
                                     QStringLiteral("type"),
                                     sourcePath,
                                     basePath,
                                     &result.diagnostics);
        } else if (rule.kind == QStringLiteral("endpointExists") ||
                   rule.kind == QStringLiteral("noIsolated")) {
            // No additional fields.
        } else if (!rule.kind.isEmpty()) {
            result.diagnostics.append(makeError(sourcePath,
                                                basePath + QStringLiteral(".kind"),
                                                0,
                                                0,
                                                QStringLiteral("Unsupported validation rule kind '%1'.")
                                                    .arg(rule.kind)));
        }

        if (rule.kind.isEmpty() || rule.code.isEmpty() || rule.severity.isEmpty() || rule.message.isEmpty())
            continue;

        result.descriptor.validationRules.append(rule);
    }

    QJsonArray derived;
    expectArray(root,
                QStringLiteral("derivedProperties"),
                &derived,
                sourcePath,
                QStringLiteral("$"),
                &result.diagnostics);

    for (int i = 0; i < derived.size(); ++i) {
        const QString basePath = QStringLiteral("$.derivedProperties[%1]").arg(i);
        const QJsonValue entry = derived.at(i);
        if (!entry.isObject()) {
            appendTypeError(&result.diagnostics, sourcePath, basePath, QStringLiteral("object"));
            continue;
        }

        const QJsonObject object = entry.toObject();
        const QString target = expectString(object,
                                            QStringLiteral("target"),
                                            sourcePath,
                                            basePath,
                                            &result.diagnostics);
        const QString property = expectString(object,
                                              QStringLiteral("property"),
                                              sourcePath,
                                              basePath,
                                              &result.diagnostics);

        if (!object.contains(QStringLiteral("value"))) {
            appendMissingFieldError(&result.diagnostics,
                                    sourcePath,
                                    basePath,
                                    QStringLiteral("value"));
            continue;
        }

        const QVariant value = object.value(QStringLiteral("value")).toVariant();

        if (target.isEmpty() || property.isEmpty())
            continue;

        result.descriptor.derivedPropertyTable[target].insert(property, value);
    }

    result.ok = result.diagnostics.isEmpty();
    return result;
}

RuleDiagnostic RuleCompiler::makeError(const QString &sourcePath,
                                       const QString &jsonPath,
                                       int line,
                                       int column,
                                       const QString &message)
{
    RuleDiagnostic diagnostic;
    diagnostic.severity = RuleDiagnosticSeverity::Error;
    diagnostic.location.filePath = sourcePath;
    diagnostic.location.jsonPath = jsonPath;
    diagnostic.location.line = line;
    diagnostic.location.column = column;
    diagnostic.message = message;
    return diagnostic;
}

void RuleCompiler::appendMissingFieldError(QVector<RuleDiagnostic> *diagnostics,
                                           const QString &sourcePath,
                                           const QString &jsonPath,
                                           const QString &fieldName)
{
    diagnostics->append(makeError(sourcePath,
                                  jsonPath + QStringLiteral(".") + fieldName,
                                  0,
                                  0,
                                  QStringLiteral("Missing required field '%1'.").arg(fieldName)));
}

void RuleCompiler::appendTypeError(QVector<RuleDiagnostic> *diagnostics,
                                   const QString &sourcePath,
                                   const QString &jsonPath,
                                   const QString &expectedType)
{
    diagnostics->append(makeError(sourcePath,
                                  jsonPath,
                                  0,
                                  0,
                                  QStringLiteral("Type mismatch: expected %1.").arg(expectedType)));
}

void RuleCompiler::offsetToLineColumn(const QString &jsonText,
                                      int offset,
                                      int *line,
                                      int *column)
{
    int currentLine = 1;
    int currentColumn = 1;

    const int clampedOffset = qMax(0, qMin(offset, jsonText.size()));
    for (int i = 0; i < clampedOffset; ++i) {
        if (jsonText.at(i) == QLatin1Char('\n')) {
            ++currentLine;
            currentColumn = 1;
        } else {
            ++currentColumn;
        }
    }

    if (line)
        *line = currentLine;
    if (column)
        *column = currentColumn;
}
