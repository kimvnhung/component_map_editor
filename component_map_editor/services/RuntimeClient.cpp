#include "RuntimeClient.h"

namespace
{

void clearError(QString *error)
{
    if (error)
        error->clear();
}

void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

} // namespace

namespace cme::editor
{

RuntimeClient::RuntimeClient(const QString &address)
    : m_address(address.trimmed())
{
}

QString RuntimeClient::address() const
{
    return m_address;
}

void RuntimeClient::setAddress(const QString &address)
{
    const QString normalized = address.trimmed();
    if (m_address == normalized)
        return;
    m_address = normalized;
    m_channel.reset();
    m_control.reset();
    m_events.reset();
    m_query.reset();
}

bool RuntimeClient::checkAddress(QString *error) const
{
    if (!m_address.isEmpty())
        return true;
    setError(error, QStringLiteral("Runtime address must not be empty."));
    return false;
}

void RuntimeClient::rebuildStubs() const
{
    if (m_channel)
        return;

    m_channel = grpc::CreateChannel(m_address.toStdString(), grpc::InsecureChannelCredentials());
    m_control = cme::runtime::v1::RuntimeControl::NewStub(m_channel);
    m_events = cme::runtime::v1::RuntimeEvents::NewStub(m_channel);
    m_query = cme::runtime::v1::RuntimeQuery::NewStub(m_channel);
}

bool RuntimeClient::checkRpc(const grpc::Status &status, QString *error)
{
    if (status.ok())
    {
        clearError(error);
        return true;
    }

    setError(error, QStringLiteral("Runtime RPC failed (%1): %2")
                        .arg(static_cast<int>(status.error_code()))
                        .arg(QString::fromStdString(status.error_message())));
    return false;
}

bool RuntimeClient::checkApplication(bool success, const QString &code,
                                      const QString &message, QString *error)
{
    if (success)
    {
        clearError(error);
        return true;
    }

    const QString detail = message.isEmpty() ? code : QStringLiteral("%1: %2").arg(code, message);
    setError(error, detail.isEmpty() ? QStringLiteral("Runtime operation was rejected.") : detail);
    return false;
}

bool RuntimeClient::deployGraph(const cme::runtime::v1::DeploymentGraph &graph,
                                cme::runtime::v1::ControlResponse *response,
                                QString *error) const
{
    if (!checkAddress(error))
        return false;
    rebuildStubs();
    cme::runtime::v1::DeployGraphRequest request;
    *request.mutable_graph() = graph;
    cme::runtime::v1::ControlResponse local;
    grpc::ClientContext context;
    const grpc::Status status = m_control->DeployGraph(&context, request, &local);
    if (!checkRpc(status, error))
        return false;
    if (response)
        *response = local;
    return checkApplication(local.success(), QString::fromStdString(local.error_code()),
                            QString::fromStdString(local.error_message()), error);
}

bool RuntimeClient::activateGraph(const QString &graphVersion,
                                  cme::runtime::v1::ControlResponse *response,
                                  QString *error) const
{
    if (!checkAddress(error))
        return false;
    rebuildStubs();
    cme::runtime::v1::ActivateGraphRequest request;
    request.set_graph_version(graphVersion.toStdString());
    cme::runtime::v1::ControlResponse local;
    grpc::ClientContext context;
    const grpc::Status status = m_control->ActivateGraph(&context, request, &local);
    if (!checkRpc(status, error))
        return false;
    if (response)
        *response = local;
    return checkApplication(local.success(), QString::fromStdString(local.error_code()),
                            QString::fromStdString(local.error_message()), error);
}

bool RuntimeClient::startRuntime(cme::runtime::v1::ControlResponse *response, QString *error) const
{
    if (!checkAddress(error))
        return false;
    rebuildStubs();
    cme::runtime::v1::EmptyRequest request;
    cme::runtime::v1::ControlResponse local;
    grpc::ClientContext context;
    const grpc::Status status = m_control->StartRuntime(&context, request, &local);
    if (!checkRpc(status, error))
        return false;
    if (response)
        *response = local;
    return checkApplication(local.success(), QString::fromStdString(local.error_code()),
                            QString::fromStdString(local.error_message()), error);
}

bool RuntimeClient::stopRuntime(cme::runtime::v1::ControlResponse *response, QString *error) const
{
    if (!checkAddress(error))
        return false;
    rebuildStubs();
    cme::runtime::v1::EmptyRequest request;
    cme::runtime::v1::ControlResponse local;
    grpc::ClientContext context;
    const grpc::Status status = m_control->StopRuntime(&context, request, &local);
    if (!checkRpc(status, error))
        return false;
    if (response)
        *response = local;
    return checkApplication(local.success(), QString::fromStdString(local.error_code()),
                            QString::fromStdString(local.error_message()), error);
}

bool RuntimeClient::runtimeStatus(cme::runtime::v1::RuntimeStatusResponse *response,
                                  QString *error) const
{
    if (!response)
    {
        setError(error, QStringLiteral("Runtime status response must not be null."));
        return false;
    }
    if (!checkAddress(error))
        return false;
    rebuildStubs();
    cme::runtime::v1::RuntimeStatusRequest request;
    grpc::ClientContext context;
    const grpc::Status status = m_control->GetRuntimeStatus(&context, request, response);
    if (!checkRpc(status, error))
        return false;
    return checkApplication(response->success(), QString::fromStdString(response->error_code()),
                            QString::fromStdString(response->error_message()), error);
}

bool RuntimeClient::publishEvent(const cme::runtime::v1::PublishEventRequest &request,
                                 cme::runtime::v1::PublishEventResponse *response,
                                 QString *error) const
{
    if (!checkAddress(error))
        return false;
    rebuildStubs();
    cme::runtime::v1::PublishEventResponse local;
    grpc::ClientContext context;
    const grpc::Status status = m_events->PublishEvent(&context, request, &local);
    if (!checkRpc(status, error))
        return false;
    if (response)
        *response = local;
    return checkApplication(local.accepted(), QString::fromStdString(local.error_code()),
                            QString::fromStdString(local.error_message()), error);
}

bool RuntimeClient::componentStatus(const QString &componentId,
                                    cme::runtime::v1::ComponentStatusResponse *response,
                                    QString *error) const
{
    if (!response)
    {
        setError(error, QStringLiteral("Component status response must not be null."));
        return false;
    }
    if (!checkAddress(error))
        return false;
    rebuildStubs();
    cme::runtime::v1::ComponentStatusRequest request;
    request.set_component_id(componentId.toStdString());
    grpc::ClientContext context;
    return checkRpc(m_query->GetComponentStatus(&context, request, response), error);
}

bool RuntimeClient::eventHistory(int limit,
                                 cme::runtime::v1::EventHistoryResponse *response,
                                 QString *error) const
{
    if (!response)
    {
        setError(error, QStringLiteral("Event history response must not be null."));
        return false;
    }
    if (!checkAddress(error))
        return false;
    rebuildStubs();
    cme::runtime::v1::QueryRequest request;
    request.set_limit(limit);
    grpc::ClientContext context;
    return checkRpc(m_query->ListEventHistory(&context, request, response), error);
}

bool RuntimeClient::runtimeLogs(int limit,
                                cme::runtime::v1::RuntimeLogsResponse *response,
                                QString *error) const
{
    if (!response)
    {
        setError(error, QStringLiteral("Runtime logs response must not be null."));
        return false;
    }
    if (!checkAddress(error))
        return false;
    rebuildStubs();
    cme::runtime::v1::QueryRequest request;
    request.set_limit(limit);
    grpc::ClientContext context;
    return checkRpc(m_query->ListRuntimeLogs(&context, request, response), error);
}

bool RuntimeClient::rejections(int limit,
                               cme::runtime::v1::RejectionsResponse *response,
                               QString *error) const
{
    if (!response)
    {
        setError(error, QStringLiteral("Rejections response must not be null."));
        return false;
    }
    if (!checkAddress(error))
        return false;
    rebuildStubs();
    cme::runtime::v1::QueryRequest request;
    request.set_limit(limit);
    grpc::ClientContext context;
    return checkRpc(m_query->ListRejections(&context, request, response), error);
}

} // namespace cme::editor
