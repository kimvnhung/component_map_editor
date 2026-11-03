#include <QtTest>

#include "component_map_runtime/BuiltinFactoryExtension.h"
#include "component_map_runtime/RuntimeHandlerRegistry.h"

class tst_RuntimeExtensionPhase4 : public QObject
{
    Q_OBJECT

private slots:
    void builtinFactoryPackRegistersDefinitions();
    void duplicateComponentRegistrationIsRejected();
    void registryDoesNotDependOnEditorTypes();
};

void tst_RuntimeExtensionPhase4::builtinFactoryPackRegistersDefinitions()
{
    cme::runtime::RuntimeHandlerRegistry registry;
    QString error;
    QVERIFY2(registry.registerExtension(cme::runtime::makeBuiltinFactoryExtension(), &error),
             qPrintable(error));
    const QStringList types = {
        QStringLiteral("factory/manager"),
        QStringLiteral("factory/fruit_producer"),
        QStringLiteral("factory/store"),
        QStringLiteral("factory/wash_machine"),
        QStringLiteral("factory/seller")};
    for (const QString &type : types)
    {
        QVERIFY(registry.contains(type));
        const auto definition = registry.definition(type);
        QCOMPARE(definition.componentType, type);
        QVERIFY(!definition.handlerId.isEmpty());
        QCOMPARE(definition.stateSchemaVersion, 1);
        QCOMPARE(registry.extensionFor(type), QStringLiteral("component-map.factory"));
        QVERIFY(registry.create(type));
    }
}

void tst_RuntimeExtensionPhase4::duplicateComponentRegistrationIsRejected()
{
    cme::runtime::RuntimeHandlerRegistry registry;
    QString error;
    QVERIFY(registry.registerExtension(cme::runtime::makeBuiltinFactoryExtension(), &error));
    QVERIFY(!registry.registerExtension(cme::runtime::makeBuiltinFactoryExtension(), &error));
    QVERIFY(error.contains(QStringLiteral("Duplicate")));
}

void tst_RuntimeExtensionPhase4::registryDoesNotDependOnEditorTypes()
{
    cme::runtime::RuntimeHandlerRegistry registry;
    const auto extension = cme::runtime::makeBuiltinFactoryExtension();
    QVERIFY(extension.components.size() >= 5);
    QVERIFY(!extension.factories.isEmpty());
    QVERIFY(!registry.contains(QStringLiteral("factory/manager")));
}

QTEST_MAIN(tst_RuntimeExtensionPhase4)
#include "tst_RuntimeExtensionPhase4.moc"
