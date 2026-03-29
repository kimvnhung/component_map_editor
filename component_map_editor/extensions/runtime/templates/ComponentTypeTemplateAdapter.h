#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

#include "provider_templates.pb.h"

namespace cme::runtime::templates {

class ComponentTypeTemplateAdapter
{
public:
    static QString providerId(const cme::templates::v1::ComponentTypeTemplateBundle &bundle);

    static QStringList componentTypeIds(const cme::templates::v1::ComponentTypeTemplateBundle &bundle);

    static QVariantMap componentTypeDescriptor(
        const cme::templates::v1::ComponentTypeTemplateBundle &bundle,
        const QString &componentTypeId);

    static QVariantMap defaultComponentProperties(
        const cme::templates::v1::ComponentTypeTemplateBundle &bundle,
        const QString &componentTypeId);

private:
    static QVariant valueToVariant(const google::protobuf::Value &value);
};

} // namespace cme::runtime::templates
