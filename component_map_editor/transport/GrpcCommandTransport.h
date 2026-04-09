#ifndef GRPCCOMMANDTRANSPORT_H
#define GRPCCOMMANDTRANSPORT_H

#include <QString>
#include <atomic>

#include "command.pb.h"

class CommandGateway;

namespace cme::transport {

// Runtime flags so transport can be toggled without coupling to domain services.
struct CommandTransportFeatureFlags {
    // false => always use in-process typed core executor.
    bool grpcTransportEnabled = false;

    // true => if grpc transport is unavailable/fails, fallback to in-process.
    bool fallbackToInProcessOnGrpcFailure = true;
};

// Per-call execution options for transport behavior.
struct CommandCallOptions {
    // 0 means no deadline budget enforced by this facade.
    int timeoutMs = 0;

    // Optional cooperative cancel flag shared by caller.
    std::atomic_bool *cancelRequested = nullptr;
};

// Abstract typed core executor boundary (domain path).
class ITypedCommandExecutor
{
public:
    virtual ~ITypedCommandExecutor() = default;

    virtual bool execute(const cme::GraphCommandRequest &request,
                         cme::GraphCommandResult *outResult,
                         QString *error) = 0;
};

// In-process implementation that delegates to CommandGateway.
class InProcessTypedCommandExecutor : public ITypedCommandExecutor
{
public:
    explicit InProcessTypedCommandExecutor(CommandGateway *gateway,
                                           const QString &actor = QStringLiteral("__system__"));

    bool execute(const cme::GraphCommandRequest &request,
                 cme::GraphCommandResult *outResult,
                 QString *error) override;

private:
    CommandGateway *m_gateway = nullptr;
    QString m_actor;
};

// Transport-facing status values modeled after gRPC outcomes.
enum class GrpcCallStatus {
    Ok,
    Failed,
    Cancelled,
    DeadlineExceeded,
    Unavailable
};

// Abstract gRPC client adapter boundary.
class IGrpcCommandClient
{
public:
    virtual ~IGrpcCommandClient() = default;

    virtual GrpcCallStatus execute(const cme::GraphCommandRequest &request,
                                   const CommandCallOptions &options,
                                   cme::GraphCommandResult *outResult,
                                   QString *error) = 0;
};

// Facade that selects transport mode and maps gRPC outcomes to typed results.
class GrpcCommandTransportFacade
{
public:
    explicit GrpcCommandTransportFacade(ITypedCommandExecutor *inProcessExecutor,
                                        IGrpcCommandClient *grpcClient = nullptr);

    void setFeatureFlags(const CommandTransportFeatureFlags &flags);
    CommandTransportFeatureFlags featureFlags() const;

    bool execute(const cme::GraphCommandRequest &request,
                 cme::GraphCommandResult *outResult,
                 QString *error = nullptr,
                 const CommandCallOptions &options = {});

private:
    bool executeInProcess(const cme::GraphCommandRequest &request,
                          cme::GraphCommandResult *outResult,
                          QString *error);

    static void fillTransportError(cme::ErrorCode errorCode,
                                   const QString &message,
                                   cme::GraphCommandResult *outResult,
                                   QString *error);

    ITypedCommandExecutor *m_inProcessExecutor = nullptr;
    IGrpcCommandClient *m_grpcClient = nullptr;
    CommandTransportFeatureFlags m_flags;
};

} // namespace cme::transport

#endif // GRPCCOMMANDTRANSPORT_H
