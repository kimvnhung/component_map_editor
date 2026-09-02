#include "filecomponenttypeprovider.h"


#include <extensions/runtime/templates/ComponentTypeTemplateAdapter.h>
#include <extensions/runtime/templates/TemplateProtoHelpers.h>
#include <provider_templates.pb.h>

cme::templates::v1::ComponentTypeTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::ComponentTypeTemplateBundle bundle;
    bundle.set_provider_id("file.component.types.provider");
    bundle.set_schema_version("1.0.0");

    // *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
    //                                     QString::fromLatin1(CustomizeComponentTypeProvider::TypeStart),
    //                                     QStringLiteral("Start"),
    //                                     QStringLiteral("control"),
    //                                     92.0,
    //                                     92.0,
    //                                     QStringLiteral("#66bb6a"),
    //                                     false,
    //                                     true);



    // *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
    //                              QString::fromLatin1(CustomizeComponentTypeProvider::TypeStart),
    // QVariantMap{{QStringLiteral("inputNumber"), 0}});


    return bundle;
}

QString FileComponentTypeProvider::providerId() const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::providerId(buildTemplateBundle());
}


QStringList FileComponentTypeProvider::componentTypeIds() const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::componentTypeIds(buildTemplateBundle());
}

QVariantMap FileComponentTypeProvider::componentTypeDescriptor(const QString &componentTypeId) const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::componentTypeDescriptor(
               buildTemplateBundle(), componentTypeId);
}

QVariantMap FileComponentTypeProvider::defaultComponentProperties(const QString &componentTypeId) const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::defaultComponentProperties(
               buildTemplateBundle(), componentTypeId);
}
