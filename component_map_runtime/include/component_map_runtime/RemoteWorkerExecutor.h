#ifndef COMPONENT_MAP_RUNTIME_REMOTE_WORKER_EXECUTOR_H
#define COMPONENT_MAP_RUNTIME_REMOTE_WORKER_EXECUTOR_H

#include "component_map_runtime/ComponentContract.h"

#include "execution.pb.h"

#include <QByteArray>
#include <QVariantMap>

namespace cme::runtime
{

class RemoteWorkerTransport
{
public:
    virtual ~RemoteWorkerTransport() = default;

    virtual bool execute(const cme::RemoteExecutionRequest &request,
                         cme::RemoteExecutionResult *result,
                         QString *error) = 0;
    virtual bool cancel(const cme::RemoteExecutionCancelRequest &request,
                        cme::RemoteExecutionCancelResponse *response,
                        QString *error) = 0;
};

class RemoteWorkerExecutor
{
public:
    explicit RemoteWorkerExecutor(RemoteWorkerTransport *transport = nullptr)
        : m_transport(transport)
    {
    }

    void setTransport(RemoteWorkerTransport *transport) { m_transport = transport; }

    bool execute(const ComponentDescriptor &descriptor,
                 const QString &requestId,
                 const QString &graphInstanceId,
                 const QByteArray &inputPayload,
                 const QVariantMap &configuration,
                 cme::RemoteExecutionResult *result,
                 QString *error = nullptr) const;

    bool cancel(const QString &requestId,
                const QString &cancellationToken,
                cme::RemoteExecutionCancelResponse *response = nullptr,
                QString *error = nullptr) const;

private:
    static void setError(QString *error, const QString &message);
    static void populateRequest(const ComponentDescriptor &descriptor,
                                const QString &requestId,
                                const QString &graphInstanceId,
                                const QByteArray &inputPayload,
                                const QVariantMap &configuration,
                                int attempt,
                                cme::RemoteExecutionRequest *request);
    static bool validDescriptor(const ComponentDescriptor &descriptor, QString *error);

    RemoteWorkerTransport *m_transport = nullptr;
};

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_REMOTE_WORKER_EXECUTOR_H
