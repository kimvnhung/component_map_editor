#include "ComponentTypeTemplateAdapter.h"

#include "TemplateProtoHelpers.h"

namespace cme::runtime::templates {

QString ComponentTypeTemplateAdapter::providerId(const cme::templates::v1::ComponentTypeTemplateBundle &bundle)
{
    return QString::fromStdString(bundle.provider_id());
}

QStringList ComponentTypeTemplateAdapter::componentTypeIds(const cme::templates::v1::ComponentTypeTemplateBundle &bundle)
{
    QStringList out;
    out.reserve(bundle.component_types_size());
    for (const cme::templates::v1::ComponentTypeTemplate &type : bundle.component_types()) {
        const QString id = QString::fromStdString(type.id()).trimmed();
        if (!id.isEmpty())
            out.append(id);
    }
    return out;
}

QVariantMap ComponentTypeTemplateAdapter::componentTypeDescriptor(
    const cme::templates::v1::ComponentTypeTemplateBundle &bundle,
    const QString &componentTypeId)
{
    const std::string componentTypeIdStd = componentTypeId.toStdString();
    for (const cme::templates::v1::ComponentTypeTemplate &type : bundle.component_types()) {
        if (type.id() != componentTypeIdStd)
            continue;

        QVariantMap descriptor{
            { QStringLiteral("id"), QString::fromStdString(type.id()) },
            { QStringLiteral("title"), QString::fromStdString(type.title()) },
            { QStringLiteral("category"), QString::fromStdString(type.category()) },
            { QStringLiteral("defaultWidth"), type.default_width() },
            { QStringLiteral("defaultHeight"), type.default_height() },
            { QStringLiteral("defaultColor"), QString::fromStdString(type.default_color()) },
            { QStringLiteral("allowIncoming"), type.allow_incoming() },
            { QStringLiteral("allowOutgoing"), type.allow_outgoing() }
        };

        for (const auto &kv : type.extra()) {
            const QString key = QString::fromStdString(kv.first);
            if (!descriptor.contains(key))
                descriptor.insert(key, protoValueToVariant(kv.second));
        }
        return descriptor;
    }

    return {};
}

QVariantMap ComponentTypeTemplateAdapter::defaultComponentProperties(
    const cme::templates::v1::ComponentTypeTemplateBundle &bundle,
    const QString &componentTypeId)
{
    const std::string componentTypeIdStd = componentTypeId.toStdString();
    for (const cme::templates::v1::ComponentTypeDefaultPropertiesTemplate &defaults : bundle.defaults()) {
        if (defaults.component_type_id() != componentTypeIdStd)
            continue;

        QVariantMap properties;
        for (const auto &kv : defaults.properties())
            properties.insert(QString::fromStdString(kv.first), protoValueToVariant(kv.second));
        return properties;
    }

    return {};
}

} // namespace cme::runtime::templates
