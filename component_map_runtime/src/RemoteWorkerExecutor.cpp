#include "component_map_runtime/RemoteWorkerExecutor.h"

#include <QVariant>

namespace cme::runtime
{

void RemoteWorkerExecutor::setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

bool RemoteWorkerExecutor::validDescriptor(const ComponentDescriptor &descriptor, QString *error)
{
    if (descriptor.source != ComponentSource::Network
        || descriptor.execution.mode != ExecutionMode::RemoteWorker)
    {
        setError(error, QStringLiteral("Remote execution requires a network remote_worker descriptor."));
        return false;
    }
    if (descriptor.trustState != TrustState::Trusted
        || descriptor.status != ComponentStatus::Available)
    {
        setError(error, QStringLiteral("Remote execution requires a trusted available descriptor."));
        return false;
    }
    const QStringList validationErrors = validateComponentDescriptor(descriptor);
    if (!validationErrors.isEmpty())
    {
        setError(error, validationErrors.join(QStringLiteral("; ")));
        return false;
    }
    return true;
}

void RemoteWorkerExecutor::populateRequest(const ComponentDescriptor &descriptor,
                                           const QString &requestId,
                                           const QString &graphInstanceId,
                                           const QByteArray &inputPayload,
                                           const QVariantMap &configuration,
                                           int attempt,
                                           cme::RemoteExecutionRequest *request)
{
    request->set_request_id(requestId.toStdString());
    request->set_component_identity(descriptor.identity.toStdString());
    request->set_component_version(descriptor.version.toStdString());
    request->set_worker_capability_reference(
        descriptor.execution.workerCapabilityReference.toStdString());
    request->set_graph_instance_id(graphInstanceId.toStdString());
    request->set_input_payload(inputPayload.toStdString());
    request->set_deadline_ms(descriptor.execution.timeoutMs);
    request->set_attempt(attempt);
    request->set_idempotent(descriptor.execution.idempotent);
    for (auto it = configuration.constBegin(); it != configuration.constEnd(); ++it)
        request->mutable_configuration()->insert({it.key().toStdString(), it.value().toString().toStdString()});
}

bool RemoteWorkerExecutor::execute(const ComponentDescriptor &descriptor,
                                    const QString &requestId,
                                    const QString &graphInstanceId,
                                    const QByteArray &inputPayload,
                                    const QVariantMap &configuration,
                                    cme::RemoteExecutionResult *result,
                                    QString *error) const
{
    if (!result)
    {
        setError(error, QStringLiteral("Remote execution result must not be null."));
        return false;
    }
    if (!m_transport)
    {
        setError(error, QStringLiteral("Remote worker transport is not configured."));
        return false;
    }
    if (requestId.trimmed().isEmpty())
    {
        setError(error, QStringLiteral("Remote execution request id must not be empty."));
        return false;
    }
    if (!validDescriptor(descriptor, error))
        return false;

    const int attempts = descriptor.execution.retryable ? 2 : 1;
    for (int attempt = 1; attempt <= attempts; ++attempt)
    {
        cme::RemoteExecutionRequest request;
        populateRequest(descriptor, requestId, graphInstanceId, inputPayload,
                         configuration, attempt, &request);
        if (!m_transport->execute(request, result, error))
            continue;
        if (result->request_id() != requestId.toStdString())
        {
            setError(error, QStringLiteral("Remote execution response request id mismatch."));
            return false;
        }
        return result->status() == cme::REMOTE_EXECUTION_STATUS_SUCCEEDED;
    }
    return false;
}

bool RemoteWorkerExecutor::cancel(const QString &requestId,
                                   const QString &cancellationToken,
                                   cme::RemoteExecutionCancelResponse *response,
                                   QString *error) const
{
    if (!m_transport)
    {
        setError(error, QStringLiteral("Remote worker transport is not configured."));
        return false;
    }
    if (requestId.trimmed().isEmpty())
    {
        setError(error, QStringLiteral("Remote execution request id must not be empty."));
        return false;
    }

    cme::RemoteExecutionCancelRequest request;
    request.set_request_id(requestId.toStdString());
    request.set_cancellation_token(cancellationToken.toStdString());
    cme::RemoteExecutionCancelResponse local;
    if (!m_transport->cancel(request, &local, error))
        return false;
    if (response)
        *response = local;
    if (!local.accepted())
    {
        setError(error, QString::fromStdString(local.error_code() + ": " + local.error_message()));
        return false;
    }
    return true;
}

} // namespace cme::runtime
