#include "ActionInvocationService.h"

#include "adapters/CommandAdapter.h"
#include "extensions/runtime/PublicApiContractAdapter.h"

ActionInvocationService::ActionInvocationService(QObject *parent)
    : QObject(parent)
{
}

void ActionInvocationService::setActionProviders(const QList<const IActionProvider *> &providers)
{
    m_providers = providers;
    m_actionIndex.clear();

    for (const IActionProvider *provider : m_providers) {
        if (!provider)
            continue;
        const QStringList actionIds = provider->actionIds();
        for (const QString &actionId : actionIds) {
            if (actionId.isEmpty() || m_actionIndex.contains(actionId))
                continue;
            m_actionIndex.insert(actionId, provider);
        }
    }
}

void ActionInvocationService::rebuildFromRegistry(const ExtensionContractRegistry &registry)
{
    setActionProviders(registry.actionProviders());
}

const IActionProvider *ActionInvocationService::providerForAction(const QString &actionId) const
{
    return m_actionIndex.value(actionId, nullptr);
}

bool ActionInvocationService::invokeAction(const QString &actionId,
                                           const QVariantMap &context,
                                           QVariantMap *commandRequest,
                                           QString *error) const
{
    const IActionProvider *provider = providerForAction(actionId);
    if (!provider) {
        if (error)
            *error = QStringLiteral("No action provider found for action '%1'").arg(actionId);
        return false;
    }

    return provider->invokeAction(actionId, context, commandRequest, error);
}

bool ActionInvocationService::invokeActionTyped(
    const cme::publicapi::v1::ActionInvokeRequest &request,
    cme::publicapi::v1::ActionInvokeResponse *response,
    QString *error) const
{
    if (!response) {
        if (error)
            *error = QStringLiteral("ActionInvokeResponse output pointer is null");
        return false;
    }

    response->Clear();

    const QString actionId = QString::fromStdString(request.action_id());
    QVariantMap contextMap;
    google::protobuf::Struct contextStruct;
    for (const auto &kv : request.context().values())
        (*contextStruct.mutable_fields())[kv.first] = kv.second;
    contextMap = cme::runtime::PublicApiContractAdapter::protoStructToVariantMap(contextStruct);

    QVariantMap commandRequest;
    QString invokeError;
    const bool ok = invokeAction(actionId, contextMap, &commandRequest, &invokeError);
    response->mutable_status()->set_success(ok);
    if (!ok) {
        response->mutable_status()->set_error_code("ACTION_INVOKE_FAILED");
        response->mutable_status()->set_error_message(invokeError.toStdString());
        if (error)
            *error = invokeError;
        return false;
    }

    cme::GraphCommandRequest typedCommand;
    const cme::adapter::ConversionError convErr =
        cme::adapter::variantMapToGraphCommandRequest(commandRequest, typedCommand);
    if (convErr.has_error) {
        const QString conversionMessage =
            QStringLiteral("Action command conversion failed for action '%1': %2")
                .arg(actionId, convErr.error_message);
        response->mutable_status()->set_success(false);
        response->mutable_status()->set_error_code("ACTION_COMMAND_CONVERSION_FAILED");
        response->mutable_status()->set_error_message(conversionMessage.toStdString());
        if (error)
            *error = conversionMessage;
        return false;
    }

    *response->mutable_command_request() = typedCommand;
    return true;
}
