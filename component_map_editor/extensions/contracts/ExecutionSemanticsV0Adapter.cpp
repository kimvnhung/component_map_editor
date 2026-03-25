#include "ExecutionSemanticsV0Adapter.h"

ExecutionSemanticsV0Adapter::ExecutionSemanticsV0Adapter(
    const IExecutionSemanticsProviderV0 *legacyProvider)
    : m_legacyProvider(legacyProvider)
{
}

QString ExecutionSemanticsV0Adapter::providerId() const
{
    if (!m_legacyProvider)
        return {};
    return m_legacyProvider->providerId();
}

QStringList ExecutionSemanticsV0Adapter::supportedComponentTypes() const
{
    if (!m_legacyProvider)
        return {};
    return m_legacyProvider->supportedComponentTypes();
}

bool ExecutionSemanticsV0Adapter::executeComponent(const QString &componentType,
                                                   const QString &componentId,
                                                   const QVariantMap &componentSnapshot,
                                                   const QVariantMap &inputState,
                                                   QVariantMap *outputState,
                                                   QVariantMap *trace,
                                                   QString *error) const
{
    if (!m_legacyProvider) {
        if (error)
            *error = QStringLiteral("Legacy provider is null");
        return false;
    }

    const bool ok = m_legacyProvider->executeComponent(componentType,
                                                       componentId,
                                                       componentSnapshot,
                                                       inputState,
                                                       outputState,
                                                       error);
    if (ok && trace) {
        trace->insert(QStringLiteral("adapter"), QStringLiteral("executionSemantics.v0"));
        trace->insert(QStringLiteral("providerId"), m_legacyProvider->providerId());
    }

    return ok;
}
