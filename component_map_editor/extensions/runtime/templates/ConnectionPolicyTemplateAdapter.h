#pragma once

#include <functional>
#include <optional>

#include <QString>
#include <QVariantMap>

#include "policy.pb.h"
#include "provider_templates.pb.h"

namespace cme::runtime::templates {

struct ConnectionPolicyDecision {
    bool allowed = false;
    QString reason;
};

class ConnectionPolicyTemplateAdapter
{
public:
    using StrategyCallback = std::function<std::optional<ConnectionPolicyDecision>(
        const cme::ConnectionPolicyContext &context,
        const ConnectionPolicyDecision &baseDecision)>;

    static QString providerId(const cme::templates::v1::ConnectionPolicyTemplateBundle &bundle);

    static ConnectionPolicyDecision evaluate(
        const cme::templates::v1::ConnectionPolicyTemplateBundle &bundle,
        const cme::ConnectionPolicyContext &context,
        const StrategyCallback &callback = StrategyCallback());

    static QVariantMap normalizeConnectionProperties(
        const cme::templates::v1::ConnectionPolicyTemplateBundle &bundle,
        const QVariantMap &rawProperties,
        const cme::ConnectionPolicyContext &context);

private:
    static bool ruleMatches(const cme::templates::v1::ConnectionPolicyRuleTemplate &rule,
                            const cme::ConnectionPolicyContext &context);
};

} // namespace cme::runtime::templates
