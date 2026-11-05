#include <QtTest>

#include "EditorFacade.h"
#include "models/ComponentModel.h"
#include "extensions/sample_pack/SampleComponentTypeProvider.h"

class tst_EditorFacadeContract : public QObject
{
    Q_OBJECT

private slots:
    void genericAuthoringApiCreatesClonesConfiguresConnectsAndValidates()
    {
        ComponentMapEditorFacade facade;
        SampleComponentTypeProvider provider;
        QString error;
        QVERIFY2(facade.registerComponentType(&provider, &error), qPrintable(error));

        const QString sourceId = facade.createComponent(
            QLatin1String(SampleComponentTypeProvider::TypeProcess), 10.0, 20.0);
        QVERIFY(!sourceId.isEmpty());

        const QString cloneId = facade.cloneComponent(sourceId, 50.0, 25.0);
        QVERIFY(!cloneId.isEmpty());
        QVERIFY(cloneId != sourceId);

        QVERIFY(facade.setComponentProperty(sourceId, QStringLiteral("threshold"), 42));
        QCOMPARE(facade.graph()->componentById(sourceId)->property("threshold").toInt(), 42);

        const QString connectionId = facade.connectPorts(sourceId, cloneId);
        QVERIFY(!connectionId.isEmpty());
        QCOMPARE(facade.graph()->connectionCount(), 1);

        QVERIFY(facade.validateGraph());
        QVERIFY(facade.removeComponent(cloneId));
        QCOMPARE(facade.graph()->componentCount(), 1);
    }

    void runtimeCatalogReplacesProviderOwnedComponentMetadata()
    {
        ComponentMapEditorFacade facade;
        SampleComponentTypeProvider provider;
        QString error;
        QVERIFY2(facade.registerComponentType(&provider, &error), qPrintable(error));
        QVERIFY(facade.typeRegistry()->hasComponentType(
            QLatin1String(SampleComponentTypeProvider::TypeProcess)));

        QVariantMap descriptor;
        descriptor.insert(QStringLiteral("identity"), QStringLiteral("runtime.network.transform"));
        descriptor.insert(QStringLiteral("version"), QStringLiteral("2.1.0"));
        descriptor.insert(QStringLiteral("source"), QStringLiteral("network"));
        descriptor.insert(QStringLiteral("title"), QStringLiteral("Runtime Transform"));
        descriptor.insert(QStringLiteral("properties"), QVariantList{
            QVariantMap{{QStringLiteral("id"), QStringLiteral("mode")},
                        {QStringLiteral("type"), QStringLiteral("string")},
                        {QStringLiteral("required"), true}}});
        descriptor.insert(QStringLiteral("outputProperties"), QVariantList{
            QVariantMap{{QStringLiteral("id"), QStringLiteral("result")},
                        {QStringLiteral("type"), QStringLiteral("string")}}});
        descriptor.insert(QStringLiteral("ports"), QVariantList{
            QVariantMap{{QStringLiteral("id"), QStringLiteral("input")},
                        {QStringLiteral("direction"), QStringLiteral("input")},
                        {QStringLiteral("payloadType"), QStringLiteral("event")}}});
        descriptor.insert(QStringLiteral("execution"),
                         QVariantMap{{QStringLiteral("mode"), 2},
                                     {QStringLiteral("protocol"), QStringLiteral("grpc")}});
        descriptor.insert(QStringLiteral("defaultProperties"),
                         QVariantMap{{QStringLiteral("mode"), QStringLiteral("safe")} });

        facade.replaceComponentTypesFromRuntimeCatalog(QVariantList{descriptor});

        QVERIFY(facade.typeRegistry()->hasRuntimeCatalog());
        QCOMPARE(facade.typeRegistry()->componentTypeIds(),
                 QStringList{QStringLiteral("runtime.network.transform")});
        QCOMPARE(facade.typeRegistry()->componentTypeDescriptor(
                     QStringLiteral("runtime.network.transform")), descriptor);

        ComponentModel component;
        QVERIFY(component.applyRuntimeDescriptor(descriptor));
        QCOMPARE(component.componentIdentity(), QStringLiteral("runtime.network.transform"));
        QCOMPARE(component.componentVersion(), QStringLiteral("2.1.0"));
        QCOMPARE(component.properties().size(), 1);
        QCOMPARE(component.outputProperties().size(), 1);
        QCOMPARE(component.ports().size(), 1);
        QCOMPARE(component.executionMetadata().value(QStringLiteral("protocol")),
                 QVariant(QStringLiteral("grpc")));
        QCOMPARE(component.dynamicPropertyValue(QStringLiteral("mode")),
                 QVariant(QStringLiteral("safe")));
        QCOMPARE(component.runtimeDescriptor().value(QStringLiteral("identity")),
                 QVariant(QStringLiteral("runtime.network.transform")));
        QVERIFY(!facade.typeRegistry()->hasComponentType(
            QLatin1String(SampleComponentTypeProvider::TypeProcess)));
    }

    void runtimeDescriptorRequiresIdentityAndVersion()
    {
        ComponentModel component;
        QVERIFY(!component.applyRuntimeDescriptor(
            QVariantMap{{QStringLiteral("identity"), QStringLiteral("missing.version")}}));
    }

    void runtimeDescriptorCanDriveNodeMetadata()
    {
        ComponentModel component;
        component.setId(QStringLiteral("node-1"));
        QVERIFY(component.applyRuntimeDescriptor(
            QVariantMap{{QStringLiteral("identity"), QStringLiteral("builtin.transform")},
                        {QStringLiteral("version"), QStringLiteral("1.0.0")},
                        {QStringLiteral("defaultWidth"), 160.0},
                        {QStringLiteral("defaultHeight"), 90.0}}));
        QCOMPARE(component.type(), QStringLiteral("builtin.transform"));
        QCOMPARE(component.width(), 160.0);
        QCOMPARE(component.height(), 90.0);
    }

    void runtimeMetadataPropertiesAreAvailableToQml()
    {
        ComponentModel component;
        QVERIFY(component.metaObject()->indexOfProperty("componentIdentity") >= 0);
        QVERIFY(component.metaObject()->indexOfProperty("componentVersion") >= 0);
        QVERIFY(component.metaObject()->indexOfProperty("properties") >= 0);
        QVERIFY(component.metaObject()->indexOfProperty("outputProperties") >= 0);
        QVERIFY(component.metaObject()->indexOfProperty("ports") >= 0);
        QVERIFY(component.metaObject()->indexOfProperty("executionMetadata") >= 0);
    }
};

QTEST_MAIN(tst_EditorFacadeContract)
#include "tst_EditorFacadeContract.moc"
