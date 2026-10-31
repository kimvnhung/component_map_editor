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
                                     RuntimePersistence *persistence)
    : m_engine(engine)
    , m_deploymentManager(deploymentManager)
    , m_runtimeService(runtimeService)
    , m_persistence(persistence)
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
    m_owner->m_engine->processAll();
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
    response->set_mailbox_depth(m_owner->m_engine->mailboxDepth(componentId));
    response->set_state(m_owner->m_engine->componentState(componentId).constData(),
                        m_owner->m_engine->componentState(componentId).size());
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
