#include <QtTest>

#include "extensions/runtime/templates/ConnectionPolicyTemplateAdapter.h"

class tst_Phase3ConnectionTransportMetadata : public QObject
{
    Q_OBJECT

private slots:
    void normalizeConnectionProperties_addsTransportDefaultsWhenOmitted();
    void normalizeConnectionProperties_preservesExplicitTransportMetadata();
    void validateTransportMetadata_invalidDescriptorProducesDeterministicDiagnostics();
};

void tst_Phase3ConnectionTransportMetadata::normalizeConnectionProperties_addsTransportDefaultsWhenOmitted()
{
    cme::templates::v1::ConnectionPolicyTemplateBundle bundle;
    bundle.set_provider_id("phase3.connectionPolicy");
    bundle.set_schema_version("1.0.0");
    bundle.set_normalized_type_key("type");
    bundle.set_normalized_type_value("flow");

    cme::ConnectionPolicyContext context;
    const QVariantMap result =
        cme::runtime::templates::ConnectionPolicyTemplateAdapter::normalizeConnectionProperties(
            bundle, {}, context);

    QCOMPARE(result.value(QStringLiteral("type")).toString(), QStringLiteral("flow"));
    QCOMPARE(result.value(QStringLiteral("transportMode")).toString(), QStringLiteral("broadcast"));
    QCOMPARE(result.value(QStringLiteral("mergeHint")).toString(), QStringLiteral("preserve-per-edge"));
}

void tst_Phase3ConnectionTransportMetadata::normalizeConnectionProperties_preservesExplicitTransportMetadata()
{
    cme::templates::v1::ConnectionPolicyTemplateBundle bundle;
    bundle.set_provider_id("phase3.connectionPolicy");
    bundle.set_schema_version("1.0.0");
    bundle.set_normalized_type_key("type");
    bundle.set_normalized_type_value("flow");
    bundle.mutable_transport()->set_payload_schema_id("workflow.connection.payload");
    bundle.mutable_transport()->set_payload_type("workflow/flow");
    bundle.mutable_transport()->set_transport_mode("broadcast");
    bundle.mutable_transport()->set_merge_hint("preserve-per-edge");

    cme::ConnectionPolicyContext context;
    const QVariantMap result =
        cme::runtime::templates::ConnectionPolicyTemplateAdapter::normalizeConnectionProperties(
            bundle, {}, context);

    QCOMPARE(result.value(QStringLiteral("payloadSchemaId")).toString(),
             QStringLiteral("workflow.connection.payload"));
    QCOMPARE(result.value(QStringLiteral("payloadType")).toString(),
             QStringLiteral("workflow/flow"));
    QCOMPARE(result.value(QStringLiteral("transportMode")).toString(), QStringLiteral("broadcast"));
    QCOMPARE(result.value(QStringLiteral("mergeHint")).toString(), QStringLiteral("preserve-per-edge"));
}

void tst_Phase3ConnectionTransportMetadata::validateTransportMetadata_invalidDescriptorProducesDeterministicDiagnostics()
{
    cme::templates::v1::ConnectionPolicyTemplateBundle bundle;
    bundle.set_provider_id("phase3.connectionPolicy.invalid");
    bundle.set_schema_version("1.0.0");
    bundle.mutable_transport()->set_payload_type("bad payload type");
    bundle.mutable_transport()->set_transport_mode("fanout");
    bundle.mutable_transport()->set_merge_hint("merge-all");

    const cme::runtime::templates::ConnectionTransportValidationResult result =
        cme::runtime::templates::ConnectionPolicyTemplateAdapter::validateTransportMetadata(bundle);

    QCOMPARE(result.valid, false);
    QCOMPARE(result.diagnostics.size(), 4);
    QCOMPARE(result.diagnostics.at(0),
             QStringLiteral("transport.payload_schema_id is required when transport.payload_type is set."));
    QCOMPARE(result.diagnostics.at(1),
             QStringLiteral("transport.payload_type must match ^[A-Za-z_][A-Za-z0-9_.:/-]*$."));
    QCOMPARE(result.diagnostics.at(2),
             QStringLiteral("transport.transport_mode must be 'broadcast' when specified."));
    QCOMPARE(result.diagnostics.at(3),
             QStringLiteral("transport.merge_hint must be 'preserve-per-edge' when specified."));
}

QTEST_MAIN(tst_Phase3ConnectionTransportMetadata)
#include "tst_Phase3ConnectionTransportMetadata.moc"
