#include "RuntimeClient.h"

#include <chrono>

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
}

namespace cme::editor
{

QString runtimeChannelStateName(RuntimeChannelState state)
{
    switch (state)
    {
    case RuntimeChannelState::Idle: return QStringLiteral("idle");
    case RuntimeChannelState::Connecting: return QStringLiteral("connecting");
    case RuntimeChannelState::Ready: return QStringLiteral("ready");
    case RuntimeChannelState::TransientFailure: return QStringLiteral("transient_failure");
    case RuntimeChannelState::Shutdown: return QStringLiteral("shutdown");
    case RuntimeChannelState::Unknown: return QStringLiteral("unknown");
    }
    return QStringLiteral("unknown");
}

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
    cancelPendingCalls();
}

void RuntimeClient::setTimeoutMs(int timeoutMs)
{
    m_timeoutMs = qMax(1, timeoutMs);
}

void RuntimeClient::setExpectedProtocolVersion(const QString &version)
{
    m_expectedProtocolVersion = version.trimmed();
}

RuntimeChannelState RuntimeClient::channelState() const
{
    if (!m_channel)
        return RuntimeChannelState::Idle;
    switch (m_channel->GetState(false))
    {
    case GRPC_CHANNEL_IDLE: return RuntimeChannelState::Idle;
    case GRPC_CHANNEL_CONNECTING: return RuntimeChannelState::Connecting;
    case GRPC_CHANNEL_READY: return RuntimeChannelState::Ready;
    case GRPC_CHANNEL_TRANSIENT_FAILURE: return RuntimeChannelState::TransientFailure;
    case GRPC_CHANNEL_SHUTDOWN: return RuntimeChannelState::Shutdown;
    }
    return RuntimeChannelState::Unknown;
}

bool RuntimeClient::reconnect(QString *error) const
{
    if (!checkAddress(error))
        return false;
    cancelPendingCalls();
    rebuildStubs();
    m_channel->GetState(true);
    clearError(error);
    return true;
}

void RuntimeClient::cancelPendingCalls() const
{
    m_channel.reset();
    m_control.reset();
    m_events.reset();
    m_query.reset();
}

void RuntimeClient::configureContext(grpc::ClientContext *context) const
{
    context->set_deadline(std::chrono::system_clock::now()
                          + std::chrono::milliseconds(m_timeoutMs));
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

bool RuntimeClient::checkProtocol(const QString &actual, QString *error) const
{
    if (m_expectedProtocolVersion.isEmpty() || actual.isEmpty()
        || actual == m_expectedProtocolVersion)
        return true;
    setError(error, QStringLiteral("Runtime protocol mismatch. Expected %1, received %2.")
                       .arg(m_expectedProtocolVersion, actual));
    return false;
}

bool RuntimeClient::deployGraph(const cme::runtime::v1::DeploymentGraph &graph,
                                cme::runtime::v1::ControlResponse *response,
                                QString *error) const
{
    if (!checkAddress(error)) return false;
    rebuildStubs();
    cme::runtime::v1::DeployGraphRequest request;
    *request.mutable_graph() = graph;
    cme::runtime::v1::ControlResponse local;
    grpc::ClientContext context;
    configureContext(&context);
    if (!checkRpc(m_control->DeployGraph(&context, request, &local), error)) return false;
    if (response) *response = local;
    return checkApplication(local.success(), QString::fromStdString(local.error_code()),
                            QString::fromStdString(local.error_message()), error);
}

bool RuntimeClient::activateGraph(const QString &graphVersion,
                                  cme::runtime::v1::ControlResponse *response,
                                  QString *error) const
{
    if (!checkAddress(error)) return false;
    rebuildStubs();
    cme::runtime::v1::ActivateGraphRequest request;
    request.set_graph_version(graphVersion.toStdString());
    cme::runtime::v1::ControlResponse local;
    grpc::ClientContext context;
    configureContext(&context);
    if (!checkRpc(m_control->ActivateGraph(&context, request, &local), error)) return false;
    if (response) *response = local;
    return checkApplication(local.success(), QString::fromStdString(local.error_code()),
                            QString::fromStdString(local.error_message()), error);
}

bool RuntimeClient::startRuntime(cme::runtime::v1::ControlResponse *response, QString *error) const
{
    if (!checkAddress(error)) return false;
    rebuildStubs();
    cme::runtime::v1::EmptyRequest request;
    cme::runtime::v1::ControlResponse local;
    grpc::ClientContext context;
    configureContext(&context);
    if (!checkRpc(m_control->StartRuntime(&context, request, &local), error)) return false;
    if (response) *response = local;
    return checkApplication(local.success(), QString::fromStdString(local.error_code()),
                            QString::fromStdString(local.error_message()), error);
}

bool RuntimeClient::stopRuntime(cme::runtime::v1::ControlResponse *response, QString *error) const
{
    if (!checkAddress(error)) return false;
    rebuildStubs();
    cme::runtime::v1::EmptyRequest request;
    cme::runtime::v1::ControlResponse local;
    grpc::ClientContext context;
    configureContext(&context);
    if (!checkRpc(m_control->StopRuntime(&context, request, &local), error)) return false;
    if (response) *response = local;
    return checkApplication(local.success(), QString::fromStdString(local.error_code()),
                            QString::fromStdString(local.error_message()), error);
}

bool RuntimeClient::runtimeStatus(cme::runtime::v1::RuntimeStatusResponse *response,
                                  QString *error) const
{
    if (!response) { setError(error, QStringLiteral("Runtime status response must not be null.")); return false; }
    if (!checkAddress(error)) return false;
    rebuildStubs();
    cme::runtime::v1::RuntimeStatusRequest request;
    grpc::ClientContext context;
    configureContext(&context);
    if (!checkRpc(m_control->GetRuntimeStatus(&context, request, response), error)) return false;
    if (!checkProtocol(QString::fromStdString(response->protocol_version()), error)) return false;
    return checkApplication(response->success(), QString::fromStdString(response->error_code()),
                            QString::fromStdString(response->error_message()), error);
}

bool RuntimeClient::publishEvent(const cme::runtime::v1::PublishEventRequest &request,
                                 cme::runtime::v1::PublishEventResponse *response,
                                 QString *error) const
{
    if (!checkAddress(error)) return false;
    rebuildStubs();
    cme::runtime::v1::PublishEventResponse local;
    grpc::ClientContext context;
    configureContext(&context);
    if (!checkRpc(m_events->PublishEvent(&context, request, &local), error)) return false;
    if (response) *response = local;
    return checkApplication(local.accepted(), QString::fromStdString(local.error_code()),
                            QString::fromStdString(local.error_message()), error);
}

bool RuntimeClient::componentStatus(const QString &componentId,
                                    cme::runtime::v1::ComponentStatusResponse *response,
                                    QString *error) const
{
    if (!response) { setError(error, QStringLiteral("Component status response must not be null.")); return false; }
    if (!checkAddress(error)) return false;
    rebuildStubs();
    cme::runtime::v1::ComponentStatusRequest request;
    request.set_component_id(componentId.toStdString());
    grpc::ClientContext context;
    configureContext(&context);
    return checkRpc(m_query->GetComponentStatus(&context, request, response), error);
}

bool RuntimeClient::componentCatalog(const cme::runtime::v1::ComponentCatalogRequest &request,
                                     cme::runtime::v1::ComponentCatalogResponse *response,
                                     QString *error) const
{
    if (!response) { setError(error, QStringLiteral("Component catalog response must not be null.")); return false; }
    if (!checkAddress(error)) return false;
    rebuildStubs();
    grpc::ClientContext context;
    configureContext(&context);
    return checkRpc(m_query->ListComponentCatalog(&context, request, response), error);
}

bool RuntimeClient::componentCatalog(const QString &identityFilter,
                                     const QString &sourceFilter,
                                     const QString &statusFilter,
                                     cme::runtime::v1::ComponentCatalogResponse *response,
                                     QString *error) const
{
    cme::runtime::v1::ComponentCatalogRequest request;
    auto *query = request.mutable_query();
    query->set_contract_version("1.0.0");
    query->set_identity_filter(identityFilter.toStdString());
    query->set_source_filter(sourceFilter.toStdString());
    query->set_status_filter(statusFilter.toStdString());
    return componentCatalog(request, response, error);
}

QVariantMap RuntimeClient::componentDescriptorMap(const cme::ComponentDescriptor &descriptor)
{
    QVariantMap result;
    result.insert(QStringLiteral("identity"), QString::fromStdString(descriptor.identity()));
    result.insert(QStringLiteral("version"), QString::fromStdString(descriptor.version()));
    result.insert(QStringLiteral("contractVersion"), QString::fromStdString(descriptor.contract_version()));
    result.insert(QStringLiteral("source"), static_cast<int>(descriptor.source()));
    result.insert(QStringLiteral("sourceId"), QString::fromStdString(descriptor.source_id()));
    result.insert(QStringLiteral("status"), static_cast<int>(descriptor.status()));
    result.insert(QStringLiteral("trustState"), static_cast<int>(descriptor.trust_state()));
    result.insert(QStringLiteral("title"), QString::fromStdString(descriptor.title()));
    result.insert(QStringLiteral("category"), QString::fromStdString(descriptor.category()));
    result.insert(QStringLiteral("icon"), QString::fromStdString(descriptor.icon()));
    result.insert(QStringLiteral("shape"), QString::fromStdString(descriptor.shape()));
    result.insert(QStringLiteral("description"), QString::fromStdString(descriptor.description()));
    result.insert(QStringLiteral("defaultWidth"), descriptor.default_width());
    result.insert(QStringLiteral("defaultHeight"), descriptor.default_height());
    QVariantList properties;
    for (const auto &property : descriptor.properties())
    {
        QVariantMap item;
        item.insert(QStringLiteral("id"), QString::fromStdString(property.id()));
        item.insert(QStringLiteral("type"), QString::fromStdString(property.type()));
        item.insert(QStringLiteral("required"), property.required());
        item.insert(QStringLiteral("default"), QString::fromStdString(property.default_value()));
        item.insert(QStringLiteral("label"), QString::fromStdString(property.label()));
        item.insert(QStringLiteral("description"), QString::fromStdString(property.description()));
        properties.append(item);
    }
    result.insert(QStringLiteral("properties"), properties);
    QVariantMap defaultProperties;
    for (const auto &property : descriptor.properties())
    {
        const QString id = QString::fromStdString(property.id());
        if (!id.isEmpty() && !property.default_value().empty())
            defaultProperties.insert(id, QString::fromStdString(property.default_value()));
    }
    result.insert(QStringLiteral("defaultProperties"), defaultProperties);
    QVariantList outputs;
    for (const auto &property : descriptor.output_properties())
    {
        QVariantMap item;
        item.insert(QStringLiteral("id"), QString::fromStdString(property.id()));
        item.insert(QStringLiteral("type"), QString::fromStdString(property.type()));
        item.insert(QStringLiteral("required"), property.required());
        outputs.append(item);
    }
    result.insert(QStringLiteral("outputProperties"), outputs);
    QVariantList ports;
    for (const auto &port : descriptor.ports())
    {
        QVariantMap item;
        item.insert(QStringLiteral("id"), QString::fromStdString(port.id()));
        item.insert(QStringLiteral("direction"), QString::fromStdString(port.direction()));
        item.insert(QStringLiteral("payloadType"), QString::fromStdString(port.payload_type()));
        item.insert(QStringLiteral("cardinality"), QString::fromStdString(port.cardinality()));
        item.insert(QStringLiteral("required"), port.required());
        ports.append(item);
    }
    result.insert(QStringLiteral("ports"), ports);
    QVariantMap execution;
    execution.insert(QStringLiteral("mode"), static_cast<int>(descriptor.execution().mode()));
    execution.insert(QStringLiteral("protocol"), QString::fromStdString(descriptor.execution().protocol()));
    execution.insert(QStringLiteral("workerCapabilityReference"), QString::fromStdString(descriptor.execution().worker_capability_reference()));
    execution.insert(QStringLiteral("timeoutMs"), descriptor.execution().timeout_ms());
    execution.insert(QStringLiteral("supportsCancellation"), descriptor.execution().supports_cancellation());
    execution.insert(QStringLiteral("retryable"), descriptor.execution().retryable());
    execution.insert(QStringLiteral("idempotent"), descriptor.execution().idempotent());
    result.insert(QStringLiteral("execution"), execution);
    return result;
}

QVariantList RuntimeClient::componentDescriptorMaps(const cme::runtime::v1::ComponentCatalogResponse &response)
{
    QVariantList result;
    if (!response.has_catalog())
        return result;
    for (const auto &descriptor : response.catalog().components())
        result.append(componentDescriptorMap(descriptor));
    return result;
}

bool RuntimeClient::componentCatalog(const QString &identityFilter,
                                     const QString &sourceFilter,
                                     const QString &statusFilter,
                                     QVariantList *descriptors,
                                     QString *error) const
{
    if (!descriptors)
    {
        setError(error, QStringLiteral("Component catalog descriptors must not be null."));
        return false;
    }
    cme::runtime::v1::ComponentCatalogResponse response;
    if (!componentCatalog(identityFilter, sourceFilter, statusFilter, &response, error))
        return false;
    *descriptors = componentDescriptorMaps(response);
    return true;
}

bool RuntimeClient::eventHistory(int limit,
                                 cme::runtime::v1::EventHistoryResponse *response,
                                 QString *error) const
{
    if (!response) { setError(error, QStringLiteral("Event history response must not be null.")); return false; }
    if (!checkAddress(error)) return false;
    rebuildStubs();
    cme::runtime::v1::QueryRequest request;
    request.set_limit(limit);
    grpc::ClientContext context;
    configureContext(&context);
    return checkRpc(m_query->ListEventHistory(&context, request, response), error);
}

bool RuntimeClient::runtimeLogs(int limit,
                                cme::runtime::v1::RuntimeLogsResponse *response,
                                QString *error) const
{
    if (!response) { setError(error, QStringLiteral("Runtime logs response must not be null.")); return false; }
    if (!checkAddress(error)) return false;
    rebuildStubs();
    cme::runtime::v1::QueryRequest request;
    request.set_limit(limit);
    grpc::ClientContext context;
    configureContext(&context);
    return checkRpc(m_query->ListRuntimeLogs(&context, request, response), error);
}

bool RuntimeClient::rejections(int limit,
                               cme::runtime::v1::RejectionsResponse *response,
                               QString *error) const
{
    if (!response) { setError(error, QStringLiteral("Rejections response must not be null.")); return false; }
    if (!checkAddress(error)) return false;
    rebuildStubs();
    cme::runtime::v1::QueryRequest request;
    request.set_limit(limit);
    grpc::ClientContext context;
    configureContext(&context);
    return checkRpc(m_query->ListRejections(&context, request, response), error);
}

} // namespace cme::editor
