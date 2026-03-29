#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include "provider_templates.pb.h"

namespace cme::runtime::templates {

class PropertySchemaTemplateAdapter
{
public:
    static QString providerId(const cme::templates::v1::PropertySchemaTemplateBundle &bundle);

    static QStringList schemaTargets(const cme::templates::v1::PropertySchemaTemplateBundle &bundle);

    static QVariantList schemaForTarget(const cme::templates::v1::PropertySchemaTemplateBundle &bundle,
                                        const QString &targetId);

private:
    static QVariant valueToVariant(const google::protobuf::Value &value);

    static QVariantMap fieldToVariantMap(const cme::templates::v1::PropertySchemaFieldTemplate &field);
};

} // namespace cme::runtime::templates
