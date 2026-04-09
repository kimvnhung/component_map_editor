#include "RuleRuntimeRegistry.h"

RuleRuntimeRegistry::RuleRuntimeRegistry(QObject *parent)
    : QObject(parent)
{}

qint64 RuleRuntimeRegistry::revision() const
{
    return m_revision;
}

const CompiledRuleDescriptor &RuleRuntimeRegistry::descriptor() const
{
    return m_descriptor;
}

bool RuleRuntimeRegistry::applyCompileResult(const RuleCompileResult &compileResult,
                                             QVector<RuleDiagnostic> *failureDiagnostics)
{
    if (!compileResult.ok) {
        if (failureDiagnostics)
            *failureDiagnostics = compileResult.diagnostics;
        return false;
    }

    setDescriptor(compileResult.descriptor);
    return true;
}

void RuleRuntimeRegistry::setDescriptor(const CompiledRuleDescriptor &descriptor)
{
    m_descriptor = descriptor;
    ++m_revision;
}
