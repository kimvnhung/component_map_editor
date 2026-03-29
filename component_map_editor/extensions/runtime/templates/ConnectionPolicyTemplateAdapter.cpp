#include "ConnectionPolicyTemplateAdapter.h"

namespace cme::runtime::templates {

QString ConnectionPolicyTemplateAdapter::providerId(const cme::templates::v1::ConnectionPolicyTemplateBundle &bundle)
{
    return QString::fromStdString(bundle.provider_id());
}

ConnectionPolicyDecision ConnectionPolicyTemplateAdapter::evaluate(
    const cme::templates::v1::ConnectionPolicyTemplateBundle &bundle,
    const cme::ConnectionPolicyContext &context,
    const StrategyCallback &callback)
{
    ConnectionPolicyDecision decision;
    decision.allowed = bundle.default_allowed();
    decision.reason = QString::fromStdString(bundle.default_reason());

    for (const cme::templates::v1::ConnectionPolicyRuleTemplate &rule : bundle.rules()) {
        if (!ruleMatches(rule, context))
            continue;

        decision.allowed = rule.allowed();
        decision.reason = QString::fromStdString(rule.reason());
        break;
    }

    if (callback) {
        const std::optional<ConnectionPolicyDecision> overrideDecision = callback(context, decision);
        if (overrideDecision.has_value())
            return overrideDecision.value();
    }

    return decision;
}

QVariantMap ConnectionPolicyTemplateAdapter::normalizeConnectionProperties(
    const cme::templates::v1::ConnectionPolicyTemplateBundle &bundle,
    const QVariantMap &rawProperties,
    const cme::ConnectionPolicyContext &context)
{
    Q_UNUSED(context)

    QVariantMap result = rawProperties;
    const QString normalizedTypeKey = QString::fromStdString(bundle.normalized_type_key());
    const QString normalizedTypeValue = QString::fromStdString(bundle.normalized_type_value());

    if (!normalizedTypeKey.isEmpty() && !result.contains(normalizedTypeKey))
        result.insert(normalizedTypeKey, normalizedTypeValue);

    return result;
}

bool ConnectionPolicyTemplateAdapter::ruleMatches(
    const cme::templates::v1::ConnectionPolicyRuleTemplate &rule,
    const cme::ConnectionPolicyContext &context)
{
    if (!rule.source_type_id().empty() && rule.source_type_id() != context.source_type_id())
        return false;

    if (!rule.target_type_id().empty() && rule.target_type_id() != context.target_type_id())
        return false;

    if (rule.has_min_target_incoming() && context.target_incoming_count() < rule.min_target_incoming())
        return false;

    if (rule.has_max_target_incoming() && context.target_incoming_count() > rule.max_target_incoming())
        return false;

    if (rule.has_min_source_outgoing() && context.source_outgoing_count() < rule.min_source_outgoing())
        return false;

    if (rule.has_max_source_outgoing() && context.source_outgoing_count() > rule.max_source_outgoing())
        return false;

    return true;
}

} // namespace cme::runtime::templates
