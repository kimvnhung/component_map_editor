#include "component_map_runtime/NetworkComponentSource.h"

#include <QHash>

namespace cme::runtime
{
namespace
{
void addDiagnostic(QList<ComponentDiagnostic> &diagnostics, const QString &code,
                   const QString &message, const QString &fieldPath = {})
{
    diagnostics.append(ComponentDiagnostic{code, message, fieldPath, false});
}
}

QList<ComponentDiagnostic> validateNetworkComponentDescriptor(
    const ComponentDescriptor &descriptor,
    const DiscoverySourceConfig &sourceConfig)
{
    QList<ComponentDiagnostic> diagnostics;
    const QStringList contractErrors = validateComponentDescriptor(descriptor);
    for (const QString &error : contractErrors)
        addDiagnostic(diagnostics, QStringLiteral("invalid_descriptor"), error);

    if (!sourceConfig.isValid() || sourceConfig.kind != DiscoverySourceKind::Network)
        addDiagnostic(diagnostics, QStringLiteral("invalid_source_configuration"),
                      QStringLiteral("Network source configuration is invalid."));
    if (descriptor.source != ComponentSource::Network)
        addDiagnostic(diagnostics, QStringLiteral("invalid_source"),
                      QStringLiteral("Network discovery descriptors must use network source."),
                      QStringLiteral("source"));
    if (descriptor.sourceId != sourceConfig.sourceId)
        addDiagnostic(diagnostics, QStringLiteral("source_mismatch"),
                      QStringLiteral("Descriptor source id does not match the configured source."),
                      QStringLiteral("source_id"));
    if (!sourceConfig.trusted)
        addDiagnostic(diagnostics, QStringLiteral("untrusted_source"),
                      QStringLiteral("Network descriptors are rejected until the source is trusted."));
    if (descriptor.trustState != TrustState::Trusted)
        addDiagnostic(diagnostics, QStringLiteral("untrusted_descriptor"),
                      QStringLiteral("Network descriptor trust state must be trusted."),
                      QStringLiteral("trust_state"));
    if (descriptor.status == ComponentStatus::Available
        && descriptor.execution.mode != ExecutionMode::RemoteWorker)
    {
        addDiagnostic(diagnostics, QStringLiteral("invalid_execution_mode"),
                      QStringLiteral("Network components must use remote_worker execution."),
                      QStringLiteral("execution.mode"));
    }
    return diagnostics;
}

QList<ComponentDiagnostic> validateNetworkDiscoveryResult(
    const NetworkDiscoveryResult &result,
    const DiscoverySourceConfig &sourceConfig)
{
    QList<ComponentDiagnostic> diagnostics = result.diagnostics;
    QHash<QString, QString> identities;
    for (const ComponentDescriptor &descriptor : result.descriptors)
    {
        diagnostics.append(validateNetworkComponentDescriptor(descriptor, sourceConfig));
        const QString key = descriptor.identity + QStringLiteral("@") + descriptor.version;
        if (identities.contains(key))
        {
            addDiagnostic(diagnostics, QStringLiteral("descriptor_conflict"),
                          QStringLiteral("Network discovery returned duplicate identity/version."),
                          QStringLiteral("identity"));
        }
        else
        {
            identities.insert(key, descriptor.sourceId);
        }
    }
    return diagnostics;
}

} // namespace cme::runtime
