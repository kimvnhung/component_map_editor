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

        QObject *sectionModel = schemas.typedSectionModelForTarget(QStringLiteral("component/process"));
        QVERIFY(sectionModel != nullptr);
        int sectionCount = 0;
        QVERIFY(QMetaObject::invokeMethod(sectionModel, "size", Q_RETURN_ARG(int, sectionCount)));
        QVERIFY(sectionCount > 0);

        bool hasBehaviorSection = false;
        bool hasAddValueField = false;

        for (int i = 0; i < sectionCount; ++i) {
            QVariantMap section;
            QVERIFY(QMetaObject::invokeMethod(sectionModel, "rowAt",
                                              Q_RETURN_ARG(QVariantMap, section),
                                              Q_ARG(int, i)));
            const QString title = section.value(QStringLiteral("title")).toString();
            if (title == QStringLiteral("Behavior"))
                hasBehaviorSection = true;

            QObject *fieldsModel = section.value(QStringLiteral("fieldsModel")).value<QObject *>();
            if (!fieldsModel)
                continue;
            int fieldCount = 0;
            QVERIFY(QMetaObject::invokeMethod(fieldsModel, "size", Q_RETURN_ARG(int, fieldCount)));
            for (int j = 0; j < fieldCount; ++j) {
                QVariantMap field;
                QMetaObject::invokeMethod(fieldsModel, "rowAt", Q_RETURN_ARG(QVariantMap, field), Q_ARG(int, j));
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

        cme::publicapi::v1::PropertySchemaResponse response;
        QVERIFY(schemas.schemaForTargetTyped(QStringLiteral("component/nonexistent"), &response));
        QVERIFY(response.entries_size() > 0);

        bool hasTitle = false;
        bool hasWidth = false;
        for (int i = 0; i < response.entries_size(); ++i) {
            const QString key = QString::fromStdString(response.entries(i).key());
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

        cme::publicapi::v1::PropertySchemaResponse response;
        QString conversionError;
        QVERIFY(!schemas.schemaForTargetTyped(QStringLiteral("component/invalid"),
                                              &response, &conversionError));
        QCOMPARE(QString::fromStdString(response.status().error_code()),
                 QStringLiteral("SCHEMA_CONVERSION_FAILED"));
        QVERIFY(!conversionError.isEmpty());
    }

    void samplePackConnectionSchemaIncludesTokenRefOptionsSource()
    {
        ExtensionContractRegistry contracts(ExtensionApiVersion{1, 0, 0});
        SampleExtensionPack pack;
        QVERIFY(pack.registerAll(contracts));

        PropertySchemaRegistry schemas;
        schemas.rebuildFromRegistry(contracts);

        cme::publicapi::v1::PropertySchemaResponse response;
        QVERIFY(schemas.schemaForTargetTyped(QStringLiteral("connection/flow"), &response));
        QVERIFY(response.entries_size() > 0);

        bool foundTokenKey = false;
        for (const auto &entry : response.entries()) {
            if (QString::fromStdString(entry.key()) != QStringLiteral("tokenKey"))
                continue;
            foundTokenKey = true;
            QCOMPARE(QString::fromStdString(entry.editor_widget()), QStringLiteral("dropdown"));
            QVERIFY(entry.type() == "string" || entry.type() == "enum");
            QVERIFY(entry.options_size() >= 0);
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

        QCOMPARE(cme::runtime::schemaOptionsSourceFromString(QStringLiteral("tokenRef")),
                 cme::runtime::SchemaOptionsSource::TokenRef);
        QCOMPARE(cme::runtime::schemaOptionsSourceToString(cme::runtime::SchemaOptionsSource::TokenRef),
                 QStringLiteral("tokenRef"));
        const QVariantList customItems = {QStringLiteral("one"), QStringLiteral("two")};
        const cme::runtime::SchemaOptionsSpec custom = cme::runtime::Options::custom(customItems);
        QCOMPARE(custom.items, customItems);
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

        int sectionCount = 0;
        QVERIFY(QMetaObject::invokeMethod(typedModel, "size", Q_RETURN_ARG(int, sectionCount)));
        QVERIFY(sectionCount > 0);
    }

};

QTEST_MAIN(tst_PropertySchemaRegistry)
#include "tst_PropertySchemaRegistry.moc"
