#ifndef RULERUNTIMEREGISTRY_H
#define RULERUNTIMEREGISTRY_H

#include <QObject>

#include "RuleDslTypes.h"

class RuleRuntimeRegistry : public QObject
{
public:
    explicit RuleRuntimeRegistry(QObject *parent = nullptr);

    qint64 revision() const;

    const CompiledRuleDescriptor &descriptor() const;

    // Applies only successful compile results. On failure, descriptor remains
    // unchanged to avoid runtime state corruption.
    bool applyCompileResult(const RuleCompileResult &compileResult,
                            QVector<RuleDiagnostic> *failureDiagnostics = nullptr);

    void setDescriptor(const CompiledRuleDescriptor &descriptor);

private:
    qint64 m_revision = 0;
    CompiledRuleDescriptor m_descriptor;
};

#endif // RULERUNTIMEREGISTRY_H
