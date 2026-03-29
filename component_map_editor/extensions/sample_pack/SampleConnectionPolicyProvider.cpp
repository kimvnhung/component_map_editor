#include "SampleConnectionPolicyProvider.h"

#include "extensions/runtime/templates/ConnectionPolicyTemplateAdapter.h"
#include "extensions/runtime/templates/TemplateProtoHelpers.h"
#include "SampleComponentTypeProvider.h"
#include "provider_templates.pb.h"

namespace {

cme::templates::v1::ConnectionPolicyTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::ConnectionPolicyTemplateBundle bundle;
    bundle.set_provider_id("sample.workflow.connectionPolicy");
    bundle.set_schema_version("1.0.0");
    bundle.set_default_allowed(false);
    bundle.set_default_reason("Unknown connection between types '%1' and '%2'.");
    bundle.set_normalized_type_key("type");
    bundle.set_normalized_type_value("flow");
    *bundle.mutable_transport() = cme::runtime::templates::makeConnectionTransportMetadataTemplate(
        QString(), QString(), QStringLiteral("broadcast"), QStringLiteral("preserve-per-edge"));

    *bundle.add_rules() = cme::runtime::templates::makeConnectionPolicyRuleTemplate(
        QString(),
        QString::fromLatin1(SampleComponentTypeProvider::TypeStart),
        false,
        QStringLiteral("Start component does not accept incoming connections."));
    *bundle.add_rules() = cme::runtime::templates::makeConnectionPolicyRuleTemplate(
        QString::fromLatin1(SampleComponentTypeProvider::TypeStop),
        QString(),
        false,
        QStringLiteral("Stop component does not have outgoing connections."));
    *bundle.add_rules() = cme::runtime::templates::makeConnectionPolicyRuleTemplate(
        QString::fromLatin1(SampleComponentTypeProvider::TypeStart),
        QString::fromLatin1(SampleComponentTypeProvider::TypeProcess),
        true,
        QString());
    *bundle.add_rules() = cme::runtime::templates::makeConnectionPolicyRuleTemplate(
        QString::fromLatin1(SampleComponentTypeProvider::TypeStart),
        QString(),
        false,
        QStringLiteral("Start component can only connect to process components."));
    *bundle.add_rules() = cme::runtime::templates::makeConnectionPolicyRuleTemplate(
        QString::fromLatin1(SampleComponentTypeProvider::TypeProcess),
        QString::fromLatin1(SampleComponentTypeProvider::TypeProcess),
        true,
        QString());
    *bundle.add_rules() = cme::runtime::templates::makeConnectionPolicyRuleTemplate(
        QString::fromLatin1(SampleComponentTypeProvider::TypeProcess),
        QString::fromLatin1(SampleComponentTypeProvider::TypeStop),
        true,
        QString());
    *bundle.add_rules() = cme::runtime::templates::makeConnectionPolicyRuleTemplate(
        QString::fromLatin1(SampleComponentTypeProvider::TypeProcess),
        QString(),
        false,
        QStringLiteral("Process component can connect to process or stop components only."));

    return bundle;
}

const cme::templates::v1::ConnectionPolicyTemplateBundle &templateBundle()
{
    static const cme::templates::v1::ConnectionPolicyTemplateBundle kBundle = buildTemplateBundle();
    return kBundle;
}

} // namespace

QString SampleConnectionPolicyProvider::providerId() const
{
    return cme::runtime::templates::ConnectionPolicyTemplateAdapter::providerId(templateBundle());
}

bool SampleConnectionPolicyProvider::canConnect(const cme::ConnectionPolicyContext &context,
                                                QString *reason) const
{
    cme::runtime::templates::ConnectionPolicyDecision decision =
        cme::runtime::templates::ConnectionPolicyTemplateAdapter::evaluate(templateBundle(), context);

    if (!decision.allowed && decision.reason.contains(QStringLiteral("%1")))
        decision.reason = cme::runtime::templates::formatUnknownConnectionReason(context, decision.reason);

    if (reason)
        *reason = decision.reason;
    return decision.allowed;
}

QVariantMap SampleConnectionPolicyProvider::normalizeConnectionProperties(
    const cme::ConnectionPolicyContext &context,
    const QVariantMap &rawProperties) const
{
    return cme::runtime::templates::ConnectionPolicyTemplateAdapter::normalizeConnectionProperties(
        templateBundle(), rawProperties, context);
}
