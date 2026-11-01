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

class RuntimeClient
{
public:
    explicit RuntimeClient(const QString &address = QStringLiteral("127.0.0.1:50051"));

    QString address() const;
    void setAddress(const QString &address);

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
    bool checkAddress(QString *error) const;
    static bool checkRpc(const grpc::Status &status, QString *error);
    static bool checkApplication(bool success, const QString &code,
                                const QString &message, QString *error);

    QString m_address;
    mutable std::shared_ptr<grpc::Channel> m_channel;
    mutable std::unique_ptr<cme::runtime::v1::RuntimeControl::Stub> m_control;
    mutable std::unique_ptr<cme::runtime::v1::RuntimeEvents::Stub> m_events;
    mutable std::unique_ptr<cme::runtime::v1::RuntimeQuery::Stub> m_query;
};

} // namespace cme::editor

#endif // RUNTIMECLIENT_H
