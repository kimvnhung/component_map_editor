#include "SampleConnectionPolicyProvider.h"

#include "extensions/runtime/templates/ConnectionPolicyTemplateAdapter.h"
#include "SampleComponentTypeProvider.h"
#include "provider_templates.pb.h"

namespace {

cme::templates::v1::ConnectionPolicyRuleTemplate makeRule(const char *sourceTypeId,
                                                          const char *targetTypeId,
                                                          bool allowed,
                                                          const char *reason)
{
    cme::templates::v1::ConnectionPolicyRuleTemplate rule;
    rule.set_source_type_id(sourceTypeId);
    rule.set_target_type_id(targetTypeId);
    rule.set_allowed(allowed);
    rule.set_reason(reason);
    return rule;
}

cme::templates::v1::ConnectionPolicyTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::ConnectionPolicyTemplateBundle bundle;
    bundle.set_provider_id("sample.workflow.connectionPolicy");
    bundle.set_schema_version("1.0.0");
    bundle.set_default_allowed(false);
    bundle.set_default_reason("Unknown connection between types '%1' and '%2'.");
    bundle.set_normalized_type_key("type");
    bundle.set_normalized_type_value("flow");

    *bundle.add_rules() = makeRule("", SampleComponentTypeProvider::TypeStart, false,
                                   "Start component does not accept incoming connections.");
    *bundle.add_rules() = makeRule(SampleComponentTypeProvider::TypeStop, "", false,
                                   "Stop component does not have outgoing connections.");
    *bundle.add_rules() = makeRule(SampleComponentTypeProvider::TypeStart,
                                   SampleComponentTypeProvider::TypeProcess,
                                   true,
                                   "");
    *bundle.add_rules() = makeRule(SampleComponentTypeProvider::TypeStart,
                                   "",
                                   false,
                                   "Start component can only connect to process components.");
    *bundle.add_rules() = makeRule(SampleComponentTypeProvider::TypeProcess,
                                   SampleComponentTypeProvider::TypeProcess,
                                   true,
                                   "");
    *bundle.add_rules() = makeRule(SampleComponentTypeProvider::TypeProcess,
                                   SampleComponentTypeProvider::TypeStop,
                                   true,
                                   "");
    *bundle.add_rules() = makeRule(SampleComponentTypeProvider::TypeProcess,
                                   "",
                                   false,
                                   "Process component can connect to process or stop components only.");

    return bundle;
}

const cme::templates::v1::ConnectionPolicyTemplateBundle &templateBundle()
{
    static const cme::templates::v1::ConnectionPolicyTemplateBundle kBundle = buildTemplateBundle();
    return kBundle;
}

QString formatUnknownReason(const cme::ConnectionPolicyContext &context,
                            const QString &pattern)
{
    const QString sourceTypeId = QString::fromStdString(context.source_type_id());
    const QString targetTypeId = QString::fromStdString(context.target_type_id());
    return pattern.arg(sourceTypeId, targetTypeId);
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
        decision.reason = formatUnknownReason(context, decision.reason);

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
