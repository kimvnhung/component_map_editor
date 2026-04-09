#ifndef RULECOMPILER_H
#define RULECOMPILER_H

#include <QString>

#include "RuleDslTypes.h"

class RuleCompiler
{
public:
    RuleCompileResult compileFromFile(const QString &filePath) const;
    RuleCompileResult compileFromJsonText(const QString &jsonText,
                                          const QString &sourcePath = QString()) const;

private:
    static RuleDiagnostic makeError(const QString &sourcePath,
                                    const QString &jsonPath,
                                    int line,
                                    int column,
                                    const QString &message);

    static void appendMissingFieldError(QVector<RuleDiagnostic> *diagnostics,
                                        const QString &sourcePath,
                                        const QString &jsonPath,
                                        const QString &fieldName);

    static void appendTypeError(QVector<RuleDiagnostic> *diagnostics,
                                const QString &sourcePath,
                                const QString &jsonPath,
                                const QString &expectedType);

    static void offsetToLineColumn(const QString &jsonText,
                                   int offset,
                                   int *line,
                                   int *column);
};

#endif // RULECOMPILER_H
