#ifndef CUSTOMIZE_PROPERTYSCHEMAS_PROPERTYSCHEMATEMPLATEUTILS_H
#define CUSTOMIZE_PROPERTYSCHEMAS_PROPERTYSCHEMATEMPLATEUTILS_H

#include <initializer_list>

#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

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

cme::templates::v1::PropertySchemaFieldTemplate makeTokenKeyField(
    const char *key,
    const char *title,
    bool required,
    const QVariant &defaultValue,
    const char *section,
    int order,
    const QString &hint = QString());

void addTarget(
    cme::templates::v1::PropertySchemaTemplateBundle *bundle,
    const char *targetId,
    const std::initializer_list<cme::templates::v1::PropertySchemaFieldTemplate> &fields);

} // namespace customize::property_schemas

#endif // CUSTOMIZE_PROPERTYSCHEMAS_PROPERTYSCHEMATEMPLATEUTILS_H
