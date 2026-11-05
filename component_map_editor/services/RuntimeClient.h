#ifndef RUNTIMECLIENT_H
#define RUNTIMECLIENT_H

#include <QByteArray>
#include <QString>

#include <control.grpc.pb.h>
#include <event.grpc.pb.h>
#include <query.grpc.pb.h>
#include <grpcpp/grpcpp.h>

#include <memory>

namespace cme::editor
{

enum class RuntimeChannelState
{
    Idle,
    Connecting,
    Ready,
    TransientFailure,
    Shutdown,
    Unknown
};

QString runtimeChannelStateName(RuntimeChannelState state);

class RuntimeCancellation
{
public:
    void cancel() { m_cancelled = true; }
    bool isCancelled() const { return m_cancelled; }
private:
    bool m_cancelled = false;
};

class RuntimeClient
{
public:
    explicit RuntimeClient(const QString &address = QStringLiteral("127.0.0.1:50051"));

    QString address() const;
    void setAddress(const QString &address);
    int timeoutMs() const { return m_timeoutMs; }
    void setTimeoutMs(int timeoutMs);
    QString expectedProtocolVersion() const { return m_expectedProtocolVersion; }
    void setExpectedProtocolVersion(const QString &version);
    RuntimeChannelState channelState() const;
    bool reconnect(QString *error = nullptr) const;
    void cancelPendingCalls() const;
    QString lastError() const { return m_lastError; }

    bool deployGraph(const cme::runtime::v1::DeploymentGraph &graph,
                     cme::runtime::v1::ControlResponse *response = nullptr,
                     QString *error = nullptr) const;
    bool activateGraph(const QString &graphVersion,
                       cme::runtime::v1::ControlResponse *response = nullptr,
                       QString *error = nullptr) const;
    bool startRuntime(cme::runtime::v1::ControlResponse *response = nullptr,
                      QString *error = nullptr) const;
    bool stopRuntime(cme::runtime::v1::ControlResponse *response = nullptr,
                     QString *error = nullptr) const;
    bool runtimeStatus(cme::runtime::v1::RuntimeStatusResponse *response,
                       QString *error = nullptr) const;
    bool publishEvent(const cme::runtime::v1::PublishEventRequest &request,
                      cme::runtime::v1::PublishEventResponse *response = nullptr,
                      QString *error = nullptr) const;
    bool componentStatus(const QString &componentId,
                         cme::runtime::v1::ComponentStatusResponse *response,
                         QString *error = nullptr) const;
    bool componentCatalog(const cme::runtime::v1::ComponentCatalogRequest &request,
                          cme::runtime::v1::ComponentCatalogResponse *response,
                          QString *error = nullptr) const;
    bool componentCatalog(const QString &identityFilter,
                          const QString &sourceFilter,
                          const QString &statusFilter,
                          cme::runtime::v1::ComponentCatalogResponse *response,
                          QString *error = nullptr) const;
    bool eventHistory(int limit,
                      cme::runtime::v1::EventHistoryResponse *response,
                      QString *error = nullptr) const;
    bool runtimeLogs(int limit,
                     cme::runtime::v1::RuntimeLogsResponse *response,
                     QString *error = nullptr) const;
    bool rejections(int limit,
                    cme::runtime::v1::RejectionsResponse *response,
                    QString *error = nullptr) const;

private:
    void rebuildStubs() const;
    void configureContext(grpc::ClientContext *context) const;
    bool checkAddress(QString *error) const;
    static bool checkRpc(const grpc::Status &status, QString *error);
    static bool checkApplication(bool success, const QString &code,
                                 const QString &message, QString *error);
    bool checkProtocol(const QString &actual, QString *error) const;

    QString m_address;
    int m_timeoutMs = 5000;
    QString m_expectedProtocolVersion = QStringLiteral("1.0");
    mutable QString m_lastError;
    mutable std::shared_ptr<grpc::Channel> m_channel;
    mutable std::unique_ptr<cme::runtime::v1::RuntimeControl::Stub> m_control;
    mutable std::unique_ptr<cme::runtime::v1::RuntimeEvents::Stub> m_events;
    mutable std::unique_ptr<cme::runtime::v1::RuntimeQuery::Stub> m_query;
};

} // namespace cme::editor

#endif // RUNTIMECLIENT_H
