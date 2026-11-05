#include <QtTest>

#include "EditorFacade.h"
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
        descriptor.insert(QStringLiteral("defaultProperties"),
                         QVariantMap{{QStringLiteral("mode"), QStringLiteral("safe")}});

        facade.replaceComponentTypesFromRuntimeCatalog(QVariantList{descriptor});

        QVERIFY(facade.typeRegistry()->hasRuntimeCatalog());
        QCOMPARE(facade.typeRegistry()->componentTypeIds(),
                 QStringList{QStringLiteral("runtime.network.transform")});
        QCOMPARE(facade.typeRegistry()->componentTypeDescriptor(
                     QStringLiteral("runtime.network.transform")), descriptor);
        QCOMPARE(facade.typeRegistry()->defaultComponentProperties(
                     QStringLiteral("runtime.network.transform")).value(QStringLiteral("mode")),
                 QVariant(QStringLiteral("safe")));
        QVERIFY(!facade.typeRegistry()->hasComponentType(
            QLatin1String(SampleComponentTypeProvider::TypeProcess)));
    }
};

QTEST_MAIN(tst_EditorFacadeContract)
#include "tst_EditorFacadeContract.moc"
