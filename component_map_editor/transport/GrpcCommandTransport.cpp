#include "GrpcCommandTransport.h"

#include <QStringList>

#include "adapters/CommandAdapter.h"
#include "services/CommandGateway.h"

namespace {

QVariantMap buildGatewayCommandRequest(const cme::GraphCommandRequest &request)
{
    QVariantMap command;

    switch (request.payload_case()) {
    case cme::GraphCommandRequest::kAddComponent: {
        const cme::AddComponentRequest &add = request.add_component();
        command.insert(QStringLiteral("command"), cme::adapter::commandTypeToString(cme::COMMAND_TYPE_ADD_COMPONENT));
        command.insert(QStringLiteral("id"), QString::fromStdString(add.component_id()));
        command.insert(QStringLiteral("typeId"), QString::fromStdString(add.type_id()));
        command.insert(QStringLiteral("x"), add.x());
        command.insert(QStringLiteral("y"), add.y());
        break;
    }
    case cme::GraphCommandRequest::kRemoveComponent: {
        const cme::RemoveComponentRequest &remove = request.remove_component();
        command.insert(QStringLiteral("command"), cme::adapter::commandTypeToString(cme::COMMAND_TYPE_REMOVE_COMPONENT));
        command.insert(QStringLiteral("id"), QString::fromStdString(remove.component_id()));
        break;
    }
    case cme::GraphCommandRequest::kMoveComponent: {
        const cme::MoveComponentRequest &move = request.move_component();
        command.insert(QStringLiteral("command"), cme::adapter::commandTypeToString(cme::COMMAND_TYPE_MOVE_COMPONENT));
        command.insert(QStringLiteral("id"), QString::fromStdString(move.component_id()));
        command.insert(QStringLiteral("x"), move.new_x());
        command.insert(QStringLiteral("y"), move.new_y());
        break;
    }
    case cme::GraphCommandRequest::kAddConnection: {
        const cme::AddConnectionRequest &add = request.add_connection();
        command.insert(QStringLiteral("command"), cme::adapter::commandTypeToString(cme::COMMAND_TYPE_ADD_CONNECTION));
        command.insert(QStringLiteral("id"), QString::fromStdString(add.connection_id()));
        command.insert(QStringLiteral("sourceId"), QString::fromStdString(add.source_id()));
        command.insert(QStringLiteral("targetId"), QString::fromStdString(add.target_id()));
        break;
    }
    case cme::GraphCommandRequest::kRemoveConnection: {
        const cme::RemoveConnectionRequest &remove = request.remove_connection();
        command.insert(QStringLiteral("command"), cme::adapter::commandTypeToString(cme::COMMAND_TYPE_REMOVE_CONNECTION));
        command.insert(QStringLiteral("id"), QString::fromStdString(remove.connection_id()));
        break;
    }
    case cme::GraphCommandRequest::kSetComponentProperty: {
        const cme::SetComponentPropertyRequest &set = request.set_component_property();
        command.insert(QStringLiteral("command"), cme::adapter::commandTypeToString(cme::COMMAND_TYPE_SET_COMPONENT_PROPERTY));
        command.insert(QStringLiteral("id"), QString::fromStdString(set.component_id()));
        command.insert(QStringLiteral("property"), QString::fromStdString(set.key()));
        command.insert(QStringLiteral("value"), QString::fromStdString(set.value()));
        break;
    }
    case cme::GraphCommandRequest::kSetConnectionProperty: {
        const cme::SetConnectionPropertyRequest &set = request.set_connection_property();
        command.insert(QStringLiteral("command"), cme::adapter::commandTypeToString(cme::COMMAND_TYPE_SET_CONNECTION_PROPERTY));
        command.insert(QStringLiteral("id"), QString::fromStdString(set.connection_id()));
        command.insert(QStringLiteral("property"), QString::fromStdString(set.key()));
        command.insert(QStringLiteral("value"), QString::fromStdString(set.value()));
        break;
    }
    case cme::GraphCommandRequest::PAYLOAD_NOT_SET:
        break;
    }

    return command;
}

cme::ErrorCode inferErrorCode(const QString &error)
{
    const QString msg = error.toLower();
    if (msg.contains(QStringLiteral("unknown command")))
        return cme::ERROR_CODE_UNKNOWN_COMMAND;
    if (msg.contains(QStringLiteral("required")) || msg.contains(QStringLiteral("missing")))
        return cme::ERROR_CODE_MISSING_FIELD;
    if (msg.contains(QStringLiteral("capability")) || msg.contains(QStringLiteral("lacks 'graph.mutate'")))
        return cme::ERROR_CODE_CAPABILITY_DENIED;
    if (msg.contains(QStringLiteral("post-command invariant violation")))
        return cme::ERROR_CODE_POST_INVARIANT_VIOLATED;
    if (msg.contains(QStringLiteral("policy")))
        return cme::ERROR_CODE_POLICY_REJECTED;
    return cme::ERROR_CODE_UNSPECIFIED;
}

} // namespace

namespace cme::transport {

InProcessTypedCommandExecutor::InProcessTypedCommandExecutor(CommandGateway *gateway,
                                                             const QString &actor)
    : m_gateway(gateway)
    , m_actor(actor)
{}

bool InProcessTypedCommandExecutor::execute(const cme::GraphCommandRequest &request,
                                            cme::GraphCommandResult *outResult,
                                            QString *error)
{
    if (!m_gateway) {
        const QString msg = QStringLiteral("InProcessTypedCommandExecutor: gateway is null");
        if (outResult) {
            outResult->set_success(false);
            outResult->set_error_code(cme::ERROR_CODE_UNSPECIFIED);
            outResult->set_error_message(msg.toStdString());
        }
        if (error)
            *error = msg;
        return false;
    }

    const QVariantMap commandRequest = buildGatewayCommandRequest(request);
    if (commandRequest.isEmpty()) {
        const QString msg = QStringLiteral("InProcessTypedCommandExecutor: request payload is not set");
        if (outResult) {
            outResult->set_success(false);
            outResult->set_error_code(cme::ERROR_CODE_UNKNOWN_COMMAND);
            outResult->set_error_message(msg.toStdString());
        }
        if (error)
            *error = msg;
        return false;
    }

    QString execError;
    const bool ok = (m_actor == QStringLiteral("__system__"))
                        ? m_gateway->executeSystemCommand(commandRequest, &execError)
                        : m_gateway->executeRequest(m_actor, commandRequest, &execError);

    if (outResult) {
        outResult->set_success(ok);
        outResult->set_error_code(ok ? cme::ERROR_CODE_UNSPECIFIED : inferErrorCode(execError));
        outResult->set_error_message(execError.toStdString());
    }

    if (!ok && error)
        *error = execError;

    return ok;
}

GrpcCommandTransportFacade::GrpcCommandTransportFacade(ITypedCommandExecutor *inProcessExecutor,
                                                       IGrpcCommandClient *grpcClient)
    : m_inProcessExecutor(inProcessExecutor)
    , m_grpcClient(grpcClient)
{}

void GrpcCommandTransportFacade::setFeatureFlags(const CommandTransportFeatureFlags &flags)
{
    m_flags = flags;
}

CommandTransportFeatureFlags GrpcCommandTransportFacade::featureFlags() const
{
    return m_flags;
}

bool GrpcCommandTransportFacade::execute(const cme::GraphCommandRequest &request,
                                         cme::GraphCommandResult *outResult,
                                         QString *error,
                                         const CommandCallOptions &options)
{
    if (options.cancelRequested && options.cancelRequested->load()) {
        fillTransportError(cme::ERROR_CODE_UNSPECIFIED,
                           QStringLiteral("gRPC command execution cancelled"),
                           outResult,
                           error);
        return false;
    }

    if (!m_flags.grpcTransportEnabled || !m_grpcClient)
        return executeInProcess(request, outResult, error);

    QString grpcError;
    const GrpcCallStatus status = m_grpcClient->execute(request, options, outResult, &grpcError);

    switch (status) {
    case GrpcCallStatus::Ok:
        return outResult ? outResult->success() : grpcError.isEmpty();
    case GrpcCallStatus::Cancelled:
        fillTransportError(cme::ERROR_CODE_UNSPECIFIED,
                           grpcError.isEmpty() ? QStringLiteral("gRPC call cancelled") : grpcError,
                           outResult,
                           error);
        return false;
    case GrpcCallStatus::DeadlineExceeded:
        fillTransportError(cme::ERROR_CODE_UNSPECIFIED,
                           grpcError.isEmpty() ? QStringLiteral("gRPC deadline exceeded") : grpcError,
                           outResult,
                           error);
        return false;
    case GrpcCallStatus::Failed:
    case GrpcCallStatus::Unavailable:
        if (m_flags.fallbackToInProcessOnGrpcFailure)
            return executeInProcess(request, outResult, error);

        fillTransportError(cme::ERROR_CODE_UNSPECIFIED,
                           grpcError.isEmpty() ? QStringLiteral("gRPC transport unavailable") : grpcError,
                           outResult,
                           error);
        return false;
    }

    fillTransportError(cme::ERROR_CODE_UNSPECIFIED,
                       QStringLiteral("gRPC transport returned unknown status"),
                       outResult,
                       error);
    return false;
}

bool GrpcCommandTransportFacade::executeInProcess(const cme::GraphCommandRequest &request,
                                                  cme::GraphCommandResult *outResult,
                                                  QString *error)
{
    if (!m_inProcessExecutor) {
        fillTransportError(cme::ERROR_CODE_UNSPECIFIED,
                           QStringLiteral("In-process executor is not configured"),
                           outResult,
                           error);
        return false;
    }

    return m_inProcessExecutor->execute(request, outResult, error);
}

void GrpcCommandTransportFacade::fillTransportError(cme::ErrorCode errorCode,
                                                    const QString &message,
                                                    cme::GraphCommandResult *outResult,
                                                    QString *error)
{
    if (outResult) {
        outResult->set_success(false);
        outResult->set_error_code(errorCode);
        outResult->set_error_message(message.toStdString());
    }
    if (error)
        *error = message;
}

} // namespace cme::transport
