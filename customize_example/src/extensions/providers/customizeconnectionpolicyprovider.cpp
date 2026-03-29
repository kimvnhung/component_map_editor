#include "customizeconnectionpolicyprovider.h"

#include "extensions/runtime/templates/ConnectionPolicyTemplateAdapter.h"
#include "extensions/runtime/templates/TemplateProtoHelpers.h"
#include "customizecomponenttypeprovider.h"
#include "provider_templates.pb.h"

namespace {

cme::templates::v1::ConnectionPolicyTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::ConnectionPolicyTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.connectionPolicy");
    bundle.set_schema_version("1.0.0");
    bundle.set_default_allowed(true);
    bundle.set_default_reason("Unknown connection between types '%1' and '%2'.");
    bundle.set_normalized_type_key("type");
    bundle.set_normalized_type_value("flow");

    *bundle.add_rules() = cme::runtime::templates::makeConnectionPolicyRuleTemplate(
        QString(),
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeStart),
        false,
        QStringLiteral("Start component does not accept incoming connections."));
    *bundle.add_rules() = cme::runtime::templates::makeConnectionPolicyRuleTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeStop),
        QString(),
        false,
        QStringLiteral("Stop component does not have outgoing connections."));

    return bundle;
}

const cme::templates::v1::ConnectionPolicyTemplateBundle &templateBundle()
{
    static const cme::templates::v1::ConnectionPolicyTemplateBundle kBundle = buildTemplateBundle();
    return kBundle;
}

std::optional<cme::runtime::templates::ConnectionPolicyDecision> customizePolicyStrategy(
    const cme::ConnectionPolicyContext &context,
    const cme::runtime::templates::ConnectionPolicyDecision &baseDecision)
{
    if (!baseDecision.allowed)
        return std::nullopt;

    const QString sourceTypeId = QString::fromStdString(context.source_type_id());
    const QString targetTypeId = QString::fromStdString(context.target_type_id());
    const int targetIncomingCount = context.target_incoming_count();
    const int sourceOutgoingCount = context.source_outgoing_count();

    if (targetTypeId == QLatin1String(CustomizeComponentTypeProvider::TypeCondition)) {
        if (targetIncomingCount >= 1) {
            return cme::runtime::templates::ConnectionPolicyDecision{
                false,
                QStringLiteral("Condition component accepts only one incoming connection.")
            };
        }

        return cme::runtime::templates::ConnectionPolicyDecision{ true, QString() };
    }

    if (sourceTypeId == QLatin1String(CustomizeComponentTypeProvider::TypeProcess)) {
        if (sourceOutgoingCount >= 1) {
            return cme::runtime::templates::ConnectionPolicyDecision{
                false,
                QStringLiteral("Process component accepts only one outgoing connection.")
            };
        }

        return cme::runtime::templates::ConnectionPolicyDecision{ true, QString() };
    }

    if (targetTypeId == QLatin1String(CustomizeComponentTypeProvider::TypeProcess)) {
        if (targetIncomingCount >= 1) {
            return cme::runtime::templates::ConnectionPolicyDecision{
                false,
                QStringLiteral("Process component accepts only one incoming connection.")
            };
        }

        return cme::runtime::templates::ConnectionPolicyDecision{ true, QString() };
    }

    return std::nullopt;
}

} // namespace

QString CustomizeConnectionPolicyProvider::providerId() const
{
    return cme::runtime::templates::ConnectionPolicyTemplateAdapter::providerId(templateBundle());
}

bool CustomizeConnectionPolicyProvider::canConnect(const cme::ConnectionPolicyContext &context,
                                                   QString *reason) const
{
    cme::runtime::templates::ConnectionPolicyDecision decision =
        cme::runtime::templates::ConnectionPolicyTemplateAdapter::evaluate(
            templateBundle(), context, customizePolicyStrategy);

    if (!decision.allowed && decision.reason.contains(QStringLiteral("%1")))
        decision.reason = cme::runtime::templates::formatUnknownConnectionReason(context, decision.reason);

    if (reason)
        *reason = decision.reason;
    return decision.allowed;
}

QVariantMap CustomizeConnectionPolicyProvider::normalizeConnectionProperties(
    const cme::ConnectionPolicyContext &context,
    const QVariantMap &rawProperties) const
{
    return cme::runtime::templates::ConnectionPolicyTemplateAdapter::normalizeConnectionProperties(
        templateBundle(), rawProperties, context);
}