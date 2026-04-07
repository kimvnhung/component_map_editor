#include <QtTest>

#include "extensions/contracts/ExtensionApiVersion.h"
#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/IPropertySchemaProvider.h"
#include "extensions/runtime/PropertySchemaRegistry.h"
#include "extensions/runtime/SchemaFieldDefinition.h"
#include "extensions/sample_pack/SampleExtensionPack.h"

class InvalidPropertySchemaProvider : public IPropertySchemaProvider
{
public:
    QString providerId() const override { return QStringLiteral("invalid.schema.provider"); }

    QStringList schemaTargets() const override
    {
        return { QStringLiteral("component/invalid") };
    }

    QVariantList propertySchema(const QString &) const override
    {
        return {
            QVariantMap{
                { QStringLiteral("title"), QStringLiteral("Broken Entry") },
                { QStringLiteral("editor"), QStringLiteral("textfield") }
            }
        };
    }
};

class tst_PropertySchemaRegistry : public QObject
{
    Q_OBJECT

private slots:
    void samplePackProvidesSectionedProcessSchema()
    {
        ExtensionContractRegistry contracts(ExtensionApiVersion{1, 0, 0});
        SampleExtensionPack pack;
        QVERIFY(pack.registerAll(contracts));

        PropertySchemaRegistry schemas;
        schemas.rebuildFromRegistry(contracts);

        const QVariantList sections = schemas.sectionedSchemaForTarget(QStringLiteral("component/process"));
        QVERIFY(!sections.isEmpty());

        bool hasBehaviorSection = false;
        bool hasAddValueField = false;

        for (const QVariant &sectionValue : sections) {
            const QVariantMap section = sectionValue.toMap();
            const QString title = section.value(QStringLiteral("title")).toString();
            if (title == QStringLiteral("Behavior"))
                hasBehaviorSection = true;

            const QVariantList fields = section.value(QStringLiteral("fields")).toList();
            for (const QVariant &fieldValue : fields) {
                const QVariantMap field = fieldValue.toMap();
                if (field.value(QStringLiteral("key")).toString() == QStringLiteral("addValue")) {
                    hasAddValueField = true;
                    QCOMPARE(field.value(QStringLiteral("widget")).toString(), QStringLiteral("spinbox"));
                }
            }
        }

        QVERIFY(hasBehaviorSection);
        QVERIFY(hasAddValueField);
    }

    void unknownTargetFallsBackToBuiltInSchema()
    {
        ExtensionContractRegistry contracts(ExtensionApiVersion{1, 0, 0});
        PropertySchemaRegistry schemas;
        schemas.rebuildFromRegistry(contracts);

        const QVariantList rows = schemas.schemaForTarget(QStringLiteral("component/nonexistent"));
        QVERIFY(!rows.isEmpty());

        bool hasTitle = false;
        bool hasWidth = false;
        for (const QVariant &rowValue : rows) {
            const QVariantMap row = rowValue.toMap();
            const QString key = row.value(QStringLiteral("key")).toString();
            hasTitle = hasTitle || key == QStringLiteral("title");
            hasWidth = hasWidth || key == QStringLiteral("width");
        }

        QVERIFY(hasTitle);
        QVERIFY(hasWidth);
    }

    void invalidSchemaRowsProduceSchemaErrorFallback()
    {
        ExtensionContractRegistry contracts(ExtensionApiVersion{1, 0, 0});
        InvalidPropertySchemaProvider invalidProvider;
        QVERIFY(contracts.registerPropertySchemaProvider(&invalidProvider));

        PropertySchemaRegistry schemas;
        schemas.rebuildFromRegistry(contracts);

        const QVariantList rows = schemas.schemaForTarget(QStringLiteral("component/invalid"));
        QVERIFY(!rows.isEmpty());

        const QVariantMap row = rows.first().toMap();
        QCOMPARE(row.value(QStringLiteral("widget")).toString(), QStringLiteral("schema_error"));
        QVERIFY(!row.value(QStringLiteral("valid")).toBool());
        QVERIFY(!row.value(QStringLiteral("schemaError")).toString().isEmpty());
    }

    void samplePackConnectionSchemaIncludesTokenKeyOptionsSource()
    {
        ExtensionContractRegistry contracts(ExtensionApiVersion{1, 0, 0});
        SampleExtensionPack pack;
        QVERIFY(pack.registerAll(contracts));

        PropertySchemaRegistry schemas;
        schemas.rebuildFromRegistry(contracts);

        const QVariantList rows = schemas.schemaForTarget(QStringLiteral("connection/flow"));
        QVERIFY(!rows.isEmpty());

        bool foundTokenKey = false;
        for (const QVariant &rowValue : rows) {
            const QVariantMap row = rowValue.toMap();
            if (row.value(QStringLiteral("key")).toString() != QStringLiteral("tokenKey"))
                continue;

            foundTokenKey = true;
            QCOMPARE(row.value(QStringLiteral("widget")).toString(), QStringLiteral("dropdown"));
            QCOMPARE(row.value(QStringLiteral("optionsSource")).toString(), QStringLiteral("tokenKeys"));
            break;
        }

        QVERIFY(foundTokenKey);
    }

    void schemaFieldEnums_roundTripStrings()
    {
        QCOMPARE(cme::runtime::schemaFieldWidgetFromString(QStringLiteral("dropdown")),
                 cme::runtime::SchemaFieldWidget::Dropdown);
        QCOMPARE(cme::runtime::schemaFieldWidgetToString(cme::runtime::SchemaFieldWidget::SpinBox),
                 QStringLiteral("spinbox"));

        QCOMPARE(cme::runtime::schemaFieldTypeFromString(QStringLiteral("integer")),
                 cme::runtime::SchemaFieldType::Number);
        QCOMPARE(cme::runtime::schemaFieldTypeToString(cme::runtime::SchemaFieldType::Boolean),
                 QStringLiteral("bool"));

        QCOMPARE(cme::runtime::schemaOptionsSourceFromString(QStringLiteral("tokenKeys")),
                 cme::runtime::SchemaOptionsSource::TokenKeys);
        QCOMPARE(cme::runtime::schemaOptionsSourceToString(cme::runtime::SchemaOptionsSource::TokenKeyOptions),
                 QStringLiteral("tokenKeyOptions"));
        QCOMPARE(cme::runtime::schemaOptionsSourceFromString(QStringLiteral("customSource")),
                 cme::runtime::SchemaOptionsSource::Custom);

        QCOMPARE(cme::runtime::schemaFieldSectionFromString(QStringLiteral("Context")),
             cme::runtime::SchemaFieldSection::Context);
        QCOMPARE(cme::runtime::schemaFieldSectionToString(cme::runtime::SchemaFieldSection::Fallback),
             QStringLiteral("Fallback"));
    }

    void legacyAdapter_defaultsAndValidation()
    {
        const QVariantMap legacyRow = {
            { QStringLiteral("key"), QStringLiteral("inputA") },
            { QStringLiteral("editor"), QStringLiteral("dropdown") },
            { QStringLiteral("defaultValue"), QStringLiteral("token.a") }
        };

        const cme::runtime::SchemaFieldDefinition field =
            cme::runtime::SchemaFieldLegacyAdapter::fromLegacyRow(legacyRow,
                                                                   QStringLiteral("component/math/add"));
        QVERIFY(field.valid);
        QCOMPARE(field.title, QStringLiteral("inputA"));
        QCOMPARE(field.widget, cme::runtime::SchemaFieldWidget::Dropdown);
        QCOMPARE(field.optionsSource, cme::runtime::SchemaOptionsSource::None);

        const QVariantMap normalized = cme::runtime::SchemaFieldLegacyAdapter::toNormalizedRow(field);
        QCOMPARE(normalized.value(QStringLiteral("widget")).toString(), QStringLiteral("dropdown"));
        QCOMPARE(normalized.value(QStringLiteral("title")).toString(), QStringLiteral("inputA"));
        QVERIFY(normalized.value(QStringLiteral("schemaError")).toString().isEmpty());
    }

    void typedSectionModel_isExposedAlongsideLegacySections()
    {
        ExtensionContractRegistry contracts(ExtensionApiVersion{1, 0, 0});
        SampleExtensionPack pack;
        QVERIFY(pack.registerAll(contracts));

        PropertySchemaRegistry schemas;
        schemas.rebuildFromRegistry(contracts);

        QObject *typedModel = schemas.typedSectionModelForTarget(QStringLiteral("component/process"));
        QVERIFY(typedModel != nullptr);

        const QVariantList legacySections = schemas.sectionedSchemaForTarget(QStringLiteral("component/process"));
        QVERIFY(!legacySections.isEmpty());
    }
};

QTEST_MAIN(tst_PropertySchemaRegistry)
#include "tst_PropertySchemaRegistry.moc"
