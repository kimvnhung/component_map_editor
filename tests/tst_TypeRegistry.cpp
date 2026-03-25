#include <QtTest>

#include "extensions/contracts/ExtensionApiVersion.h"
#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/runtime/TypeRegistry.h"
#include "extensions/sample_pack/SampleComponentTypeProvider.h"
#include "extensions/sample_pack/SampleConnectionPolicyProvider.h"

class StubComponentTypeProvider : public IComponentTypeProvider
{
public:
    explicit StubComponentTypeProvider(const QString &providerId,
                                       const QStringList &typeIds)
        : m_providerId(providerId)
        , m_typeIds(typeIds)
    {}

    QString providerId() const override { return m_providerId; }
    QStringList componentTypeIds() const override { return m_typeIds; }

    QVariantMap componentTypeDescriptor(const QString &typeId) const override
    {
        return QVariantMap{
            { QStringLiteral("id"),            typeId },
            { QStringLiteral("title"),         typeId + QLatin1String(" Title") },
            { QStringLiteral("defaultWidth"),  100.0 },
            { QStringLiteral("defaultHeight"), 60.0 },
            { QStringLiteral("defaultColor"),  QStringLiteral("#aabbcc") }
        };
    }

    QVariantMap defaultComponentProperties(const QString &) const override
    {
        return {};
    }

private:
    QString m_providerId;
    QStringList m_typeIds;
};

class DenyAllConnectionPolicyProvider : public IConnectionPolicyProvider
{
public:
    QString providerId() const override { return QStringLiteral("deny.all"); }

    bool canConnect(const QString &, const QString &, const QVariantMap &, QString *reason) const override
    {
        if (reason)
            *reason = QStringLiteral("DenyAll: connection refused.");
        return false;
    }

    QVariantMap normalizeConnectionProperties(const QString &, const QString &, const QVariantMap &raw) const override
    {
        return raw;
    }
};

static ExtensionApiVersion coreV1() { return {1, 0, 0}; }

class tst_TypeRegistry : public QObject
{
    Q_OBJECT

private slots:
    void emptyRegistryHasNoComponentTypes()
    {
        ExtensionContractRegistry reg(coreV1());
        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);
        QVERIFY(tr.componentTypeIds().isEmpty());
        QVERIFY(tr.componentTypeDescriptors().isEmpty());
        QVERIFY(!tr.hasComponentType(QStringLiteral("start")));
    }

    void emptyRegistryAllowsAllConnections()
    {
        ExtensionContractRegistry reg(coreV1());
        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);
        QVERIFY(tr.canConnect(QStringLiteral("a"), QStringLiteral("b")));
    }

    void singleProviderExposesAllTypes()
    {
        SampleComponentTypeProvider provider;
        ExtensionContractRegistry reg(coreV1());
        QVERIFY(reg.registerComponentTypeProvider(&provider));

        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);

        QCOMPARE(tr.componentTypeIds().size(), 3);
        QVERIFY(tr.hasComponentType(QLatin1String(SampleComponentTypeProvider::TypeStart)));
        QVERIFY(tr.hasComponentType(QLatin1String(SampleComponentTypeProvider::TypeProcess)));
        QVERIFY(tr.hasComponentType(QLatin1String(SampleComponentTypeProvider::TypeStop)));
    }

    void descriptorLookupReturnsCorrectMap()
    {
        SampleComponentTypeProvider provider;
        ExtensionContractRegistry reg(coreV1());
        reg.registerComponentTypeProvider(&provider);

        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);

        const QVariantMap processDesc = tr.componentTypeDescriptor(QLatin1String(SampleComponentTypeProvider::TypeProcess));
        QCOMPARE(processDesc.value(QStringLiteral("id")).toString(),
                 QLatin1String(SampleComponentTypeProvider::TypeProcess));
        QCOMPARE(processDesc.value(QStringLiteral("defaultWidth")).toReal(), 164.0);
        QCOMPARE(processDesc.value(QStringLiteral("defaultHeight")).toReal(), 100.0);
        QCOMPARE(processDesc.value(QStringLiteral("defaultColor")).toString(),
                 QStringLiteral("#4fc3f7"));
    }

    void defaultPropertiesFromProviderReturned()
    {
        SampleComponentTypeProvider provider;
        ExtensionContractRegistry reg(coreV1());
        reg.registerComponentTypeProvider(&provider);

        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);

        const QVariantMap processDefaults = tr.defaultComponentProperties(
            QLatin1String(SampleComponentTypeProvider::TypeProcess));
        QCOMPARE(processDefaults.value(QStringLiteral("addValue")).toInt(), 9);
        QVERIFY(processDefaults.contains(QStringLiteral("description")));
    }

    void unknownTypeDescriptorIsEmpty()
    {
        SampleComponentTypeProvider provider;
        ExtensionContractRegistry reg(coreV1());
        reg.registerComponentTypeProvider(&provider);

        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);

        QVERIFY(!tr.hasComponentType(QStringLiteral("nonexistent")));
        QVERIFY(tr.componentTypeDescriptor(QStringLiteral("nonexistent")).isEmpty());
        QVERIFY(tr.defaultComponentProperties(QStringLiteral("nonexistent")).isEmpty());
    }

    void multipleProvidersAggregateTypes()
    {
        StubComponentTypeProvider pA(QStringLiteral("provider.a"),
                                     { QStringLiteral("alpha"), QStringLiteral("beta") });
        StubComponentTypeProvider pB(QStringLiteral("provider.b"),
                                     { QStringLiteral("gamma") });

        ExtensionContractRegistry reg(coreV1());
        QVERIFY(reg.registerComponentTypeProvider(&pA));
        QVERIFY(reg.registerComponentTypeProvider(&pB));

        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);

        QCOMPARE(tr.componentTypeIds().size(), 3);
        QVERIFY(tr.hasComponentType(QStringLiteral("alpha")));
        QVERIFY(tr.hasComponentType(QStringLiteral("beta")));
        QVERIFY(tr.hasComponentType(QStringLiteral("gamma")));
    }

    void firstRegistrationWinsOnDuplicateTypeId()
    {
        StubComponentTypeProvider pA(QStringLiteral("provider.a"), { QStringLiteral("alpha") });
        StubComponentTypeProvider pB(QStringLiteral("provider.b"), { QStringLiteral("alpha") });

        ExtensionContractRegistry reg(coreV1());
        QVERIFY(reg.registerComponentTypeProvider(&pA));
        QVERIFY(reg.registerComponentTypeProvider(&pB));

        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);

        QCOMPARE(tr.componentTypeIds().size(), 1);
        QCOMPARE(tr.componentTypeDescriptor(QStringLiteral("alpha"))
                     .value(QStringLiteral("defaultColor")).toString(),
                 QStringLiteral("#aabbcc"));
    }

    void connectionPolicyAllowsWhenProviderAllows()
    {
        SampleConnectionPolicyProvider cp;
        ExtensionContractRegistry reg(coreV1());
        reg.registerConnectionPolicyProvider(&cp);

        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);

        QVERIFY(tr.canConnect(QLatin1String(SampleComponentTypeProvider::TypeStart),
                              QLatin1String(SampleComponentTypeProvider::TypeProcess)));
    }

    void connectionPolicyRejectsWhenProviderDenies()
    {
        SampleConnectionPolicyProvider cp;
        ExtensionContractRegistry reg(coreV1());
        reg.registerConnectionPolicyProvider(&cp);

        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);

        QString reason;
        QVERIFY(!tr.canConnect(QLatin1String(SampleComponentTypeProvider::TypeStop),
                               QLatin1String(SampleComponentTypeProvider::TypeProcess),
                               {}, &reason));
        QVERIFY(!reason.isEmpty());
    }

    void connectionPolicyRejectsConnectToStart()
    {
        SampleConnectionPolicyProvider cp;
        ExtensionContractRegistry reg(coreV1());
        reg.registerConnectionPolicyProvider(&cp);

        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);

        QVERIFY(!tr.canConnect(QLatin1String(SampleComponentTypeProvider::TypeProcess),
                               QLatin1String(SampleComponentTypeProvider::TypeStart)));
    }

    void multipleProvidersFirstDenyWins()
    {
        SampleConnectionPolicyProvider allowCp;
        DenyAllConnectionPolicyProvider denyCp;

        ExtensionContractRegistry reg(coreV1());
        reg.registerConnectionPolicyProvider(&allowCp);
        reg.registerConnectionPolicyProvider(&denyCp);

        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);

        QString reason;
        QVERIFY(!tr.canConnect(QLatin1String(SampleComponentTypeProvider::TypeStart),
                               QLatin1String(SampleComponentTypeProvider::TypeProcess),
                               {}, &reason));
        QVERIFY(reason.contains(QStringLiteral("DenyAll")));
    }

    void normalizeAddsFlowType()
    {
        SampleConnectionPolicyProvider cp;
        ExtensionContractRegistry reg(coreV1());
        reg.registerConnectionPolicyProvider(&cp);

        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);

        const QVariantMap result = tr.normalizeConnectionProperties(
            QLatin1String(SampleComponentTypeProvider::TypeProcess),
            QLatin1String(SampleComponentTypeProvider::TypeProcess));
        QCOMPARE(result.value(QStringLiteral("type")).toString(),
                 QStringLiteral("flow"));
    }

    void normalizeWithNoProvidersReturnsOriginal()
    {
        ExtensionContractRegistry reg(coreV1());
        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);

        const QVariantMap input = { { QStringLiteral("label"), QStringLiteral("hello") } };
        const QVariantMap out = tr.normalizeConnectionProperties(
            QStringLiteral("x"), QStringLiteral("y"), input);
        QCOMPARE(out, input);
    }

    void componentTypeDescriptorsPropertyMatchesHash()
    {
        SampleComponentTypeProvider provider;
        ExtensionContractRegistry reg(coreV1());
        reg.registerComponentTypeProvider(&provider);

        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);

        const QVariantList descriptors = tr.componentTypeDescriptors();
        QCOMPARE(descriptors.size(), tr.componentTypeIds().size());
        for (const QVariant &value : descriptors) {
            const QVariantMap map = value.toMap();
            const QString id = map.value(QStringLiteral("id")).toString();
            QVERIFY(!id.isEmpty());
            QVERIFY(tr.hasComponentType(id));
        }
    }

    void rebuildClearsPreviousCache()
    {
        SampleComponentTypeProvider provider;
        ExtensionContractRegistry reg1(coreV1());
        reg1.registerComponentTypeProvider(&provider);

        TypeRegistry tr;
        tr.rebuildFromRegistry(reg1);
        QCOMPARE(tr.componentTypeIds().size(), 3);

        ExtensionContractRegistry reg2(coreV1());
        tr.rebuildFromRegistry(reg2);
        QVERIFY(tr.componentTypeIds().isEmpty());
        QVERIFY(!tr.hasComponentType(QLatin1String(SampleComponentTypeProvider::TypeProcess)));
    }

    void componentTypesChangedEmittedOnRebuildWithNewTypes()
    {
        SampleComponentTypeProvider provider;
        ExtensionContractRegistry reg(coreV1());
        reg.registerComponentTypeProvider(&provider);

        TypeRegistry tr;
        QSignalSpy spy(&tr, &TypeRegistry::componentTypesChanged);
        tr.rebuildFromRegistry(reg);
        QCOMPARE(spy.count(), 1);
    }

    void componentTypesChangedNotEmittedIfTypesUnchanged()
    {
        SampleComponentTypeProvider provider;
        ExtensionContractRegistry reg(coreV1());
        reg.registerComponentTypeProvider(&provider);

        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);

        QSignalSpy spy(&tr, &TypeRegistry::componentTypesChanged);
        tr.rebuildFromRegistry(reg);
        QCOMPARE(spy.count(), 0);
    }

    void componentTypeIdsFollowProviderRegistrationOrder()
    {
        // Register providers in a specific order and verify that the resulting
        // componentTypeIds list reflects that order (provider A types first, then B).
        StubComponentTypeProvider pA(QStringLiteral("provider.a"),
                                     { QStringLiteral("alpha"), QStringLiteral("beta") });
        StubComponentTypeProvider pB(QStringLiteral("provider.b"),
                                     { QStringLiteral("gamma"), QStringLiteral("delta") });

        ExtensionContractRegistry reg(coreV1());
        QVERIFY(reg.registerComponentTypeProvider(&pA));
        QVERIFY(reg.registerComponentTypeProvider(&pB));

        TypeRegistry tr;
        tr.rebuildFromRegistry(reg);

        const QStringList ids = tr.componentTypeIds();
        QCOMPARE(ids.size(), 4);
        // Provider A types must appear before provider B types
        QVERIFY(ids.indexOf(QStringLiteral("alpha"))  < ids.indexOf(QStringLiteral("gamma")));
        QVERIFY(ids.indexOf(QStringLiteral("beta"))   < ids.indexOf(QStringLiteral("gamma")));
        QVERIFY(ids.indexOf(QStringLiteral("alpha"))  < ids.indexOf(QStringLiteral("delta")));
        QVERIFY(ids.indexOf(QStringLiteral("beta"))   < ids.indexOf(QStringLiteral("delta")));
    }
};

QTEST_MAIN(tst_TypeRegistry)
#include "tst_TypeRegistry.moc"
