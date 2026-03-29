#include "customizecomponenttypeprovider.h"

#include "extensions/runtime/templates/ComponentTypeTemplateAdapter.h"
#include "extensions/runtime/templates/TemplateProtoHelpers.h"
#include "provider_templates.pb.h"

namespace {

cme::templates::v1::ComponentTypeTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::ComponentTypeTemplateBundle bundle;
    bundle.set_provider_id("sample.workflow.componentTypes");
    bundle.set_schema_version("1.0.0");

    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeStart),
        QStringLiteral("Start"),
        QStringLiteral("control"),
        92.0,
        92.0,
        QStringLiteral("#66bb6a"),
        false,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeCondition),
        QStringLiteral("Condition"),
        QStringLiteral("control"),
        164.0,
        100.0,
        QStringLiteral("#ffca28"),
        true,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeProcess),
        QStringLiteral("Process"),
        QStringLiteral("work"),
        164.0,
        100.0,
        QStringLiteral("#4fc3f7"),
        true,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeStop),
        QStringLiteral("Stop"),
        QStringLiteral("control"),
        92.0,
        92.0,
        QStringLiteral("#ef5350"),
        true,
        false);

    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeStart),
        QVariantMap{{QStringLiteral("inputNumber"), 0}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeProcess,
        QVariantMap{{QStringLiteral("addValue"), 9}, {QStringLiteral("description"), QString()}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeCondition,
        QVariantMap{{QStringLiteral("condition"), QString()}});

    return bundle;
}

const cme::templates::v1::ComponentTypeTemplateBundle &templateBundle()
{
    static const cme::templates::v1::ComponentTypeTemplateBundle kBundle = buildTemplateBundle();
    return kBundle;
}

} // namespace

QString CustomizeComponentTypeProvider::providerId() const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::providerId(templateBundle());
}

QStringList CustomizeComponentTypeProvider::componentTypeIds() const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::componentTypeIds(templateBundle());
}

QVariantMap CustomizeComponentTypeProvider::componentTypeDescriptor(const QString &componentTypeId) const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::componentTypeDescriptor(
        templateBundle(), componentTypeId);
}

QVariantMap CustomizeComponentTypeProvider::defaultComponentProperties(const QString &componentTypeId) const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::defaultComponentProperties(
        templateBundle(), componentTypeId);
}