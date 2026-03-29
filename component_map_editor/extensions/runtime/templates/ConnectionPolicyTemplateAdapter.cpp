#include "ConnectionPolicyTemplateAdapter.h"

#include <QRegularExpression>

namespace cme::runtime::templates {

namespace {

bool isValidTransportIdentifier(const QString &value)
{
    static const QRegularExpression kPattern(
        QStringLiteral("^[A-Za-z_][A-Za-z0-9_.:/-]*$"));
    return kPattern.match(value).hasMatch();
}

} // namespace

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
    const ConnectionTransportValidationResult validation = validateTransportMetadata(bundle);

    QString transportMode = defaultTransportMode();
    QString mergeHint = defaultMergeHint();

    if (bundle.has_transport()) {
        const cme::templates::v1::ConnectionTransportMetadataTemplate &transport = bundle.transport();
        if (!QString::fromStdString(transport.transport_mode()).trimmed().isEmpty()
            && validation.valid) {
            transportMode = QString::fromStdString(transport.transport_mode());
        }
        if (!QString::fromStdString(transport.merge_hint()).trimmed().isEmpty()
            && validation.valid) {
            mergeHint = QString::fromStdString(transport.merge_hint());
        }

        const QString payloadSchemaId = QString::fromStdString(transport.payload_schema_id());
        const QString payloadType = QString::fromStdString(transport.payload_type());

        if (!payloadSchemaId.isEmpty() && validation.valid && !result.contains(QStringLiteral("payloadSchemaId")))
            result.insert(QStringLiteral("payloadSchemaId"), payloadSchemaId);

        if (!payloadType.isEmpty() && validation.valid && !result.contains(QStringLiteral("payloadType")))
            result.insert(QStringLiteral("payloadType"), payloadType);
    }

    if (!normalizedTypeKey.isEmpty() && !result.contains(normalizedTypeKey))
        result.insert(normalizedTypeKey, normalizedTypeValue);

    if (!result.contains(QStringLiteral("transportMode")))
        result.insert(QStringLiteral("transportMode"), transportMode);

    if (!result.contains(QStringLiteral("mergeHint")))
        result.insert(QStringLiteral("mergeHint"), mergeHint);

    return result;
}

ConnectionTransportValidationResult ConnectionPolicyTemplateAdapter::validateTransportMetadata(
    const cme::templates::v1::ConnectionPolicyTemplateBundle &bundle)
{
    ConnectionTransportValidationResult result;

    if (!bundle.has_transport())
        return result;

    const cme::templates::v1::ConnectionTransportMetadataTemplate &transport = bundle.transport();
    const QString payloadSchemaId = QString::fromStdString(transport.payload_schema_id());
    const QString payloadType = QString::fromStdString(transport.payload_type());
    const QString transportMode = QString::fromStdString(transport.transport_mode());
    const QString mergeHint = QString::fromStdString(transport.merge_hint());

    auto addDiagnostic = [&result](const QString &message) {
        result.valid = false;
        result.diagnostics.append(message);
    };

    if (!payloadType.trimmed().isEmpty() && payloadSchemaId.trimmed().isEmpty()) {
        addDiagnostic(QStringLiteral(
            "transport.payload_schema_id is required when transport.payload_type is set."));
    }

    if (!payloadSchemaId.trimmed().isEmpty() && !isValidTransportIdentifier(payloadSchemaId.trimmed())) {
        addDiagnostic(QStringLiteral(
            "transport.payload_schema_id must match ^[A-Za-z_][A-Za-z0-9_.:/-]*$."));
    }

    if (!payloadType.trimmed().isEmpty() && !isValidTransportIdentifier(payloadType.trimmed())) {
        addDiagnostic(QStringLiteral(
            "transport.payload_type must match ^[A-Za-z_][A-Za-z0-9_.:/-]*$."));
    }

    if (!transportMode.trimmed().isEmpty() && transportMode != defaultTransportMode()) {
        addDiagnostic(QStringLiteral(
            "transport.transport_mode must be 'broadcast' when specified."));
    }

    if (!mergeHint.trimmed().isEmpty() && mergeHint != defaultMergeHint()) {
        addDiagnostic(QStringLiteral(
            "transport.merge_hint must be 'preserve-per-edge' when specified."));
    }

    return result;
}

QString ConnectionPolicyTemplateAdapter::defaultTransportMode()
{
    return QStringLiteral("broadcast");
}

QString ConnectionPolicyTemplateAdapter::defaultMergeHint()
{
    return QStringLiteral("preserve-per-edge");
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
