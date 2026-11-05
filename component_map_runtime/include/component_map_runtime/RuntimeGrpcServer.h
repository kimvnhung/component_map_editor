#ifndef COMPONENT_MAP_RUNTIME_RUNTIME_GRPC_SERVER_H
#define COMPONENT_MAP_RUNTIME_RUNTIME_GRPC_SERVER_H

#include "component_map_runtime/RuntimeCore.h"
#include "component_map_runtime/RuntimePersistence.h"
#include "component_map_runtime/ComponentCatalog.h"
#include "component_map_runtime/ComponentDiscoveryTypes.h"
#include "component_map_runtime/RuntimeHandlerRegistry.h"

#include <control.grpc.pb.h>
#include <event.grpc.pb.h>
#include <query.grpc.pb.h>
#include <grpcpp/grpcpp.h>
#include <memory>

namespace cme::runtime
{

class RuntimeGrpcServer
{
public:
    RuntimeGrpcServer(RuntimeEngine *engine,
                      DeploymentManager *deploymentManager,
                      RuntimeService *runtimeService,
                      RuntimePersistence *persistence,
                      ComponentCatalog *catalog = nullptr);
    ~RuntimeGrpcServer();

    bool start(const QString &address, QString *error = nullptr);
    void shutdown();
    bool isRunning() const;

private:
    class ControlService final : public cme::runtime::v1::RuntimeControl::Service
    {
    public:
        explicit ControlService(RuntimeGrpcServer *owner) : m_owner(owner) {}
        grpc::Status DeployGraph(grpc::ServerContext *, const cme::runtime::v1::DeployGraphRequest *,
                                 cme::runtime::v1::ControlResponse *) override;
        grpc::Status ActivateGraph(grpc::ServerContext *, const cme::runtime::v1::ActivateGraphRequest *,
                                   cme::runtime::v1::ControlResponse *) override;
        grpc::Status StartRuntime(grpc::ServerContext *, const cme::runtime::v1::EmptyRequest *,
                                  cme::runtime::v1::ControlResponse *) override;
        grpc::Status StopRuntime(grpc::ServerContext *, const cme::runtime::v1::EmptyRequest *,
                                 cme::runtime::v1::ControlResponse *) override;
        grpc::Status GetRuntimeStatus(grpc::ServerContext *, const cme::runtime::v1::RuntimeStatusRequest *,
                                      cme::runtime::v1::RuntimeStatusResponse *) override;
    private:
        RuntimeGrpcServer *m_owner;
    };

    class EventService final : public cme::runtime::v1::RuntimeEvents::Service
    {
    public:
        explicit EventService(RuntimeGrpcServer *owner) : m_owner(owner) {}
        grpc::Status PublishEvent(grpc::ServerContext *, const cme::runtime::v1::PublishEventRequest *,
                                  cme::runtime::v1::PublishEventResponse *) override;
    private:
        RuntimeGrpcServer *m_owner;
    };

    class QueryService final : public cme::runtime::v1::RuntimeQuery::Service
    {
    public:
        explicit QueryService(RuntimeGrpcServer *owner) : m_owner(owner) {}
        grpc::Status GetComponentStatus(grpc::ServerContext *, const cme::runtime::v1::ComponentStatusRequest *,
                                        cme::runtime::v1::ComponentStatusResponse *) override;
        grpc::Status ListComponentCatalog(grpc::ServerContext *, const cme::runtime::v1::ComponentCatalogRequest *,
                                          cme::runtime::v1::ComponentCatalogResponse *) override;
        grpc::Status ListComponentDiscoveryLifecycle(
            grpc::ServerContext *, const cme::runtime::v1::ComponentDiscoveryLifecycleRequest *,
            cme::runtime::v1::ComponentDiscoveryLifecycleResponse *) override;
        grpc::Status ListEventHistory(grpc::ServerContext *, const cme::runtime::v1::QueryRequest *,
                                      cme::runtime::v1::EventHistoryResponse *) override;
        grpc::Status ListRuntimeLogs(grpc::ServerContext *, const cme::runtime::v1::QueryRequest *,
                                     cme::runtime::v1::RuntimeLogsResponse *) override;
        grpc::Status ListRejections(grpc::ServerContext *, const cme::runtime::v1::QueryRequest *,
                                    cme::runtime::v1::RejectionsResponse *) override;
    private:
        RuntimeGrpcServer *m_owner;
    };

    RuntimeEngine *m_engine;
    DeploymentManager *m_deploymentManager;
    RuntimeService *m_runtimeService;
    RuntimePersistence *m_persistence;
    ComponentCatalog *m_catalog = nullptr;
    ControlService m_controlService;
    EventService m_eventService;
    QueryService m_queryService;
    std::unique_ptr<grpc::Server> m_server;
};

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_RUNTIME_GRPC_SERVER_H
