#include "SampleComponentTypeProvider.h"

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
        QString::fromLatin1(SampleComponentTypeProvider::TypeStart),
        QStringLiteral("Start"),
        QStringLiteral("control"),
        92.0,
        92.0,
        QStringLiteral("#66bb6a"),
        false,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(SampleComponentTypeProvider::TypeProcess),
        QStringLiteral("Process"),
        QStringLiteral("work"),
        164.0,
        100.0,
        QStringLiteral("#4fc3f7"),
        true,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(SampleComponentTypeProvider::TypeStop),
        QStringLiteral("Stop"),
        QStringLiteral("control"),
        92.0,
        92.0,
        QStringLiteral("#ef5350"),
        true,
        false);

    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        QString::fromLatin1(SampleComponentTypeProvider::TypeStart),
        QVariantMap{{QStringLiteral("inputNumber"), 0}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        SampleComponentTypeProvider::TypeProcess,
        QVariantMap{{QStringLiteral("addValue"), 9}, {QStringLiteral("description"), QString()}});

    return bundle;
}

const cme::templates::v1::ComponentTypeTemplateBundle &templateBundle()
{
    static const cme::templates::v1::ComponentTypeTemplateBundle kBundle = buildTemplateBundle();
    return kBundle;
}

} // namespace

QString SampleComponentTypeProvider::providerId() const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::providerId(templateBundle());
}

QStringList SampleComponentTypeProvider::componentTypeIds() const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::componentTypeIds(templateBundle());
}

QVariantMap SampleComponentTypeProvider::componentTypeDescriptor(const QString &componentTypeId) const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::componentTypeDescriptor(
        templateBundle(), componentTypeId);
}

QVariantMap SampleComponentTypeProvider::defaultComponentProperties(const QString &componentTypeId) const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::defaultComponentProperties(
        templateBundle(), componentTypeId);
}