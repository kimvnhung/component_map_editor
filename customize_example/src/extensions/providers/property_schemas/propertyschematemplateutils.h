#ifndef CUSTOMIZE_PROPERTYSCHEMAS_PROPERTYSCHEMATEMPLATEUTILS_H
#define CUSTOMIZE_PROPERTYSCHEMAS_PROPERTYSCHEMATEMPLATEUTILS_H

#include <initializer_list>

#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include "extensions/runtime/SchemaFieldDefinition.h"
#include "provider_templates.pb.h"

namespace customize::property_schemas {

cme::templates::v1::PropertySchemaFieldTemplate makeField(
    const char *key,
    const char *type,
    const char *title,
    bool required,
    const QVariant &defaultValue,
    const char *editor,
    const char *section,
    int order,
    const QString &hint = QString(),
    const QVariantMap &validation = {},
    const QVariantMap &visibleWhen = {},
    const QVariantList &options = {},
    const QVariantMap &extra = {});

cme::templates::v1::PropertySchemaFieldTemplate makeField(
    const char *key,
    cme::runtime::SchemaFieldType type,
    const char *title,
    bool required,
    const QVariant &defaultValue,
    cme::runtime::SchemaFieldWidget widget,
    const char *section,
    int order,
    const QString &hint = QString(),
    const QVariantMap &validation = {},
    const QVariantMap &visibleWhen = {},
    const QVariantList &options = {},
    cme::runtime::SchemaOptionsSource optionsSource = cme::runtime::SchemaOptionsSource::None,
    const QString &customOptionsSource = QString(),
    const QVariantMap &extra = {});

void addTarget(
    cme::templates::v1::PropertySchemaTemplateBundle *bundle,
    const char *targetId,
    const std::initializer_list<cme::templates::v1::PropertySchemaFieldTemplate> &fields);

} // namespace customize::property_schemas

#endif // CUSTOMIZE_PROPERTYSCHEMAS_PROPERTYSCHEMATEMPLATEUTILS_H
