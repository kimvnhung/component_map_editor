#include <QtTest>

#include "extensions/contracts/ExtensionApiVersion.h"
#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/IPropertySchemaProvider.h"
#include "extensions/runtime/PropertySchemaRegistry.h"
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
};

QTEST_MAIN(tst_PropertySchemaRegistry)
#include "tst_PropertySchemaRegistry.moc"
