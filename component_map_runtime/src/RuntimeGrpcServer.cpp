#include "component_map_runtime/RuntimeGrpcServer.h"

namespace
{
using namespace cme::runtime;
using namespace cme::runtime::v1;

void setError(ControlResponse *response, const QString &code, const QString &message)
{
    response->set_success(false);
    response->set_error_code(code.toStdString());
    response->set_error_message(message.toStdString());
}
}

namespace cme::runtime
{

RuntimeGrpcServer::RuntimeGrpcServer(RuntimeEngine *engine,
                                     DeploymentManager *deploymentManager,
                                     RuntimeService *runtimeService,
                                     RuntimePersistence *persistence,
                                     ComponentCatalog *catalog)
    : m_engine(engine)
    , m_deploymentManager(deploymentManager)
    , m_runtimeService(runtimeService)
    , m_persistence(persistence)
    , m_catalog(catalog)
    , m_controlService(this)
    , m_eventService(this)
    , m_queryService(this)
{
}

RuntimeGrpcServer::~RuntimeGrpcServer()
{
    shutdown();
}

bool RuntimeGrpcServer::start(const QString &address, QString *error)
{
    if (m_server)
    {
        if (error) error->clear();
        return true;
    }
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address.toStdString(), grpc::InsecureServerCredentials());
    builder.RegisterService(&m_controlService);
    builder.RegisterService(&m_eventService);
    builder.RegisterService(&m_queryService);
    m_server = builder.BuildAndStart();
    if (!m_server)
    {
        if (error) *error = QStringLiteral("Unable to start gRPC server.");
        return false;
    }
    if (error) error->clear();
    return true;
}

void RuntimeGrpcServer::shutdown()
{
    if (m_server)
    {
        m_server->Shutdown();
        m_server.reset();
    }
}

bool RuntimeGrpcServer::isRunning() const
{
    return m_server != nullptr;
}

grpc::Status RuntimeGrpcServer::ControlService::DeployGraph(
    grpc::ServerContext *, const DeployGraphRequest *request, ControlResponse *response)
{
    if (!request->has_graph())
    {
        setError(response, QStringLiteral("GRAPH_REQUIRED"), QStringLiteral("Graph is required."));
        return grpc::Status::OK;
    }
    const auto result = m_owner->m_deploymentManager->deploy(request->graph());
    if (!result.valid())
    {
        const auto &issue = result.issues(0);
        setError(response, QString::fromStdString(issue.code()), QString::fromStdString(issue.message()));
        return grpc::Status::OK;
    }
    if (m_owner->m_persistence)
    {
        QString error;
        if (!m_owner->m_persistence->saveGraph(request->graph(), &error))
        {
            setError(response, QStringLiteral("PERSISTENCE_FAILURE"), error);
            return grpc::Status::OK;
        }
    }
    response->set_success(true);
    return grpc::Status::OK;
}

grpc::Status RuntimeGrpcServer::ControlService::ActivateGraph(
    grpc::ServerContext *, const ActivateGraphRequest *, ControlResponse *response)
{
    const auto result = m_owner->m_deploymentManager->activate();
    if (!result.valid())
    {
        const auto &issue = result.issues(0);
        setError(response, QString::fromStdString(issue.code()), QString::fromStdString(issue.message()));
        return grpc::Status::OK;
    }
    response->set_success(true);
    return grpc::Status::OK;
}

grpc::Status RuntimeGrpcServer::ControlService::StartRuntime(
    grpc::ServerContext *, const EmptyRequest *, ControlResponse *response)
{
    QString error;
    if (!m_owner->m_engine->start(&error))
    {
        setError(response, QStringLiteral("START_FAILED"), error);
        return grpc::Status::OK;
    }
    response->set_success(true);
    response->set_status(m_owner->m_runtimeService->status());
    return grpc::Status::OK;
}

grpc::Status RuntimeGrpcServer::ControlService::StopRuntime(
    grpc::ServerContext *, const EmptyRequest *, ControlResponse *response)
{
    QString error;
    if (!m_owner->m_engine->stop(&error))
    {
        setError(response, QStringLiteral("STOP_FAILED"), error);
        return grpc::Status::OK;
    }
    response->set_success(true);
    response->set_status(m_owner->m_runtimeService->status());
    return grpc::Status::OK;
}

grpc::Status RuntimeGrpcServer::ControlService::GetRuntimeStatus(
    grpc::ServerContext *, const RuntimeStatusRequest *, RuntimeStatusResponse *response)
{
    response->set_success(true);
    response->set_status(m_owner->m_runtimeService->status());
    response->set_runtime_id(m_owner->m_runtimeService->descriptor().runtime_id());
    response->set_protocol_version(m_owner->m_runtimeService->descriptor().protocol_version());
    if (m_owner->m_deploymentManager->hasActiveGraph())
    {
        response->set_graph_version(m_owner->m_deploymentManager->activeGraph().graph_version());
    }
    return grpc::Status::OK;
}

grpc::Status RuntimeGrpcServer::EventService::PublishEvent(
    grpc::ServerContext *, const PublishEventRequest *request, PublishEventResponse *response)
{
    RuntimeEvent event;
    event.eventId = QString::fromStdString(request->event_id());
    event.idempotencyKey = QString::fromStdString(request->idempotency_key());
    event.messageType = QString::fromStdString(request->message_type());
    event.targetComponentId = QString::fromStdString(request->target_component_id());
    event.payload = QByteArray(request->payload().data(), static_cast<int>(request->payload().size()));
    RuntimeFailure failure;
    if (!m_owner->m_engine->publish(event, &failure))
    {
        response->set_error_code(failure.code.toStdString());
        response->set_error_message(failure.message.toStdString());
        response->set_duplicate(failure.code == QStringLiteral("IDEMPOTENCY_IN_FLIGHT"));
        return grpc::Status::OK;
    }
    if (!m_owner->m_engine->backgroundExecution())
        m_owner->m_engine->processAll();
    else
        m_owner->m_engine->waitForIdle(5000);
    response->set_accepted(true);
    response->set_duplicate(false);
    return grpc::Status::OK;
}

grpc::Status RuntimeGrpcServer::QueryService::GetComponentStatus(
    grpc::ServerContext *, const ComponentStatusRequest *request, ComponentStatusResponse *response)
{
    const QString componentId = QString::fromStdString(request->component_id());
    response->set_component_id(request->component_id());
    if (!m_owner->m_engine->stateStore().contains(componentId)
        && m_owner->m_engine->actorStatus(componentId) == ActorStatus::Stopped)
    {
        return grpc::Status::OK;
    }
    response->set_found(true);
    response->set_status(QString::number(static_cast<int>(m_owner->m_engine->actorStatus(componentId))).toStdString());
    if (m_owner->m_deploymentManager->hasActiveGraph())
    {
        for (const auto &component : m_owner->m_deploymentManager->activeGraph().components())
        {
            if (component.instance_id() == request->component_id())
            {
                response->set_component_type(component.component_type());
                break;
            }
        }
    }
    response->set_mailbox_depth(m_owner->m_engine->mailboxDepth(componentId));
    response->set_state(m_owner->m_engine->componentState(componentId).constData(),
                        m_owner->m_engine->componentState(componentId).size());
    return grpc::Status::OK;
}

grpc::Status RuntimeGrpcServer::QueryService::ListComponentCatalog(
    grpc::ServerContext *, const cme::runtime::v1::ComponentCatalogRequest *request,
    cme::runtime::v1::ComponentCatalogResponse *response)
{
    if (!m_owner->m_catalog)
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Component catalog is not configured.");

    CatalogFilter filter;
    if (request->has_query())
    {
        const auto &query = request->query();
        if (!query.identity_filter().empty())
            filter.identity = QString::fromStdString(query.identity_filter());
        if (!query.category_filter().empty())
            filter.category = QString::fromStdString(query.category_filter());
        if (!query.source_filter().empty())
        {
            filter.filterSource = true;
            filter.source = QString::fromStdString(query.source_filter()) == QStringLiteral("network")
                                ? ComponentSource::Network : ComponentSource::Builtin;
        }
        if (!query.status_filter().empty())
        {
            filter.filterStatus = true;
            const QString status = QString::fromStdString(query.status_filter());
            if (status == QStringLiteral("available")) filter.status = ComponentStatus::Available;
            else if (status == QStringLiteral("unavailable")) filter.status = ComponentStatus::Unavailable;
            else if (status == QStringLiteral("rejected")) filter.status = ComponentStatus::Rejected;
            else if (status == QStringLiteral("removed")) filter.status = ComponentStatus::Removed;
            else filter.status = ComponentStatus::Registered;
        }
    }

    auto *catalogResponse = response->mutable_catalog();
    catalogResponse->set_catalog_version(m_owner->m_catalog->snapshotVersion());
    for (const auto &descriptor : m_owner->m_catalog->snapshot(filter))
    {
        auto *output = catalogResponse->add_components();
        output->set_identity(descriptor.identity.toStdString());
        output->set_version(descriptor.version.toStdString());
        output->set_contract_version(descriptor.contractVersion.toStdString());
        output->set_source(descriptor.source == ComponentSource::Network
                               ? cme::COMPONENT_SOURCE_NETWORK : cme::COMPONENT_SOURCE_BUILTIN);
        output->set_source_id(descriptor.sourceId.toStdString());
        output->set_status(static_cast<cme::ComponentStatus>(static_cast<int>(descriptor.status) + 1));
        output->set_trust_state(static_cast<cme::TrustState>(static_cast<int>(descriptor.trustState) + 1));
        output->set_title(descriptor.title.toStdString());
        output->set_category(descriptor.category.toStdString());
        output->set_icon(descriptor.icon.toStdString());
        output->set_shape(descriptor.shape.toStdString());
        output->set_description(descriptor.description.toStdString());
        output->set_default_width(descriptor.defaultWidth);
        output->set_default_height(descriptor.defaultHeight);
        for (const auto &property : descriptor.properties)
        {
            auto *item = output->add_properties();
            item->set_id(property.id.toStdString());
            item->set_type(property.type.toStdString());
            item->set_required(property.required);
            item->set_default_value(property.defaultValue.toStdString());
            item->set_label(property.label.toStdString());
            item->set_description(property.description.toStdString());
            for (auto it = property.constraints.cbegin(); it != property.constraints.cend(); ++it)
                (*item->mutable_constraints())[it.key().toStdString()] = it.value().toStdString();
        }
        for (const auto &port : descriptor.ports)
        {
            auto *item = output->add_ports();
            item->set_id(port.id.toStdString());
            item->set_direction(port.direction.toStdString());
            item->set_payload_type(port.payloadType.toStdString());
            item->set_cardinality(port.cardinality.toStdString());
            item->set_required(port.required);
            item->set_label(port.label.toStdString());
            item->set_description(port.description.toStdString());
        }
        auto *execution = output->mutable_execution();
        execution->set_mode(descriptor.execution.mode == ExecutionMode::RemoteWorker
                                ? cme::COMPONENT_EXECUTION_MODE_REMOTE_WORKER
                                : cme::COMPONENT_EXECUTION_MODE_BUILTIN);
        for (const auto &value : descriptor.execution.capabilities)
            execution->add_capabilities(value.toStdString());
        for (const auto &value : descriptor.execution.inputRequirements)
            execution->add_input_requirements(value.toStdString());
        for (const auto &value : descriptor.execution.outputGuarantees)
            execution->add_output_guarantees(value.toStdString());
        execution->set_worker_capability_reference(descriptor.execution.workerCapabilityReference.toStdString());
        execution->set_protocol(descriptor.execution.protocol.toStdString());
        execution->set_timeout_ms(descriptor.execution.timeoutMs);
        execution->set_supports_cancellation(descriptor.execution.supportsCancellation);
        execution->set_retryable(descriptor.execution.retryable);
        execution->set_idempotent(descriptor.execution.idempotent);
        execution->set_error_behavior(descriptor.execution.errorBehavior.toStdString());
    }
    return grpc::Status::OK;
}

grpc::Status RuntimeGrpcServer::QueryService::ListEventHistory(
    grpc::ServerContext *, const QueryRequest *request, EventHistoryResponse *response)
{
    if (!m_owner->m_persistence)
    {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Persistence is not configured.");
    }
    QString error;
    const auto items = m_owner->m_persistence->eventHistory(request->limit() > 0 ? request->limit() : 100,
                                                            &error);
    if (!error.isEmpty())
    {
        return grpc::Status(grpc::StatusCode::INTERNAL, error.toStdString());
    }
    for (const auto &item : items)
    {
        auto *output = response->add_items();
        output->set_event_id(item.eventId.toStdString());
        output->set_status(item.status.toStdString());
        output->set_failure_code(item.failureCode.toStdString());
        output->set_failure_message(item.failureMessage.toStdString());
        output->set_created_at(item.createdAt);
    }
    return grpc::Status::OK;
}

grpc::Status RuntimeGrpcServer::QueryService::ListRuntimeLogs(
    grpc::ServerContext *, const QueryRequest *request, RuntimeLogsResponse *response)
{
    if (!m_owner->m_persistence)
    {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "Persistence is not configured.");
    }
    QString error;
    const auto items = m_owner->m_persistence->runtimeLogs(request->limit() > 0 ? request->limit() : 100,
                                                           &error);
    if (!error.isEmpty())
    {
        return grpc::Status(grpc::StatusCode::INTERNAL, error.toStdString());
    }
    for (const auto &item : items)
    {
        auto *output = response->add_items();
        output->set_level(item.level.toStdString());
        output->set_code(item.code.toStdString());
        output->set_message(item.message.toStdString());
        output->set_created_at(item.createdAt);
    }
    return grpc::Status::OK;
}

grpc::Status RuntimeGrpcServer::QueryService::ListRejections(
    grpc::ServerContext *, const QueryRequest *, RejectionsResponse *)
{
    return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, "Rejection listing is not exposed yet.");
}

} // namespace cme::runtime
