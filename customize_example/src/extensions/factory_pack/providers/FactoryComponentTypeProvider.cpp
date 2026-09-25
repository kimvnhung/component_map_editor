#include "FactoryComponentTypeProvider.h"

#include <extensions/runtime/templates/ComponentTypeTemplateAdapter.h>
#include <extensions/runtime/templates/TemplateProtoHelpers.h>

namespace
{

    cme::templates::v1::ComponentTypeTemplateBundle buildTemplateBundle()
    {
        cme::templates::v1::ComponentTypeTemplateBundle bundle;
        bundle.set_provider_id("factory.componentTypes");
        bundle.set_schema_version("1.0.0");

        *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
                                            QString::fromLatin1(FactoryComponentTypeProvider::TypeFruitProducer),
                                            QStringLiteral("Fruit Producer"),
                                            QStringLiteral("factory_item"),
                                            92.0,
                                            92.0,
                                            QStringLiteral("#66bb6a"),
                                            true,
                                            true);
        *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
                                            QString::fromLatin1(FactoryComponentTypeProvider::TypeStore),
                                            QStringLiteral("Store"),
                                            QStringLiteral("factory_item"),
                                            92.0,
                                            92.0,
                                            QStringLiteral("#ef5350"),
                                            true,
                                            false);
        *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
                                            QString::fromLatin1(FactoryComponentTypeProvider::TypeEmployee),
                                            QStringLiteral("Employee"),
                                            QStringLiteral("factory_item"),
                                            200.0,
                                            108.0,
                                            QStringLiteral("#ffa726"),
                                            true,
                                            true);
        *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
                                            QString::fromLatin1(FactoryComponentTypeProvider::TypeManager),
                                            QStringLiteral("Manager"),
                                            QStringLiteral("factory_item"),
                                            200.0,
                                            108.0,
                                            QStringLiteral("#ffb300"),
                                            true,
                                            true);
        *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
                                            QString::fromLatin1(FactoryComponentTypeProvider::TypeSeller),
                                            QStringLiteral("Seller"),
                                            QStringLiteral("factory_item"),
                                            180.0,
                                            108.0,
                                            QStringLiteral("#7e57c2"),
                                            true,
                                            true);


        *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
                                     QString::fromLatin1(FactoryComponentTypeProvider::TypeFruitProducer),
                                     QVariantMap
        {
            {QStringLiteral("price"), 1},
            {QStringLiteral("productionRate"), 1},
            {QStringLiteral("buySizePerTime"), 1},
        });

        *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
                                     QString::fromLatin1(FactoryComponentTypeProvider::TypeStore),
                                     QVariantMap{});

        *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
                                     QString::fromLatin1(FactoryComponentTypeProvider::TypeEmployee),
        QVariantMap{{QStringLiteral("performancePerSecond"), 10},
        });

        *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
                                     FactoryComponentTypeProvider::TypeSeller,
        QVariantMap{{QStringLiteral("performancePerSecond"), 20}});

        *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
                                     FactoryComponentTypeProvider::TypeManager,
        QVariantMap{{QStringLiteral("capital"), 0},
            {QStringLiteral("buyAmount"), 10}});

        return bundle;
    }

    const cme::templates::v1::ComponentTypeTemplateBundle &templateBundle()
    {
        static const cme::templates::v1::ComponentTypeTemplateBundle kBundle = buildTemplateBundle();
        return kBundle;
    }

} // namespace

QString FactoryComponentTypeProvider::providerId() const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::providerId(templateBundle());
}

QStringList FactoryComponentTypeProvider::componentTypeIds() const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::componentTypeIds(templateBundle());
}

QVariantMap FactoryComponentTypeProvider::componentTypeDescriptor(const QString &componentTypeId) const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::componentTypeDescriptor(
               templateBundle(), componentTypeId);
}

QVariantMap FactoryComponentTypeProvider::defaultComponentProperties(const QString &componentTypeId) const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::defaultComponentProperties(
               templateBundle(), componentTypeId);
}