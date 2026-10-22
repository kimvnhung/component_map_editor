#include "SchemaOptionsSpec.h"

#include <QVariantList>

namespace cme::runtime
{

SchemaOptionsSpec SchemaOptionsSpec::none()
{
    return SchemaOptionsSpec{};
}

QString schemaOptionsSourceToString(SchemaOptionsSource value)
{
    switch (value)
    {
        case SchemaOptionsSource::None:
            return QString();
        case SchemaOptionsSource::TokenRef:
            return QStringLiteral("tokenKeys");
        case SchemaOptionsSource::TokenKeyOptions:
            return QStringLiteral("tokenKeyOptions");
        case SchemaOptionsSource::Custom:
            return QStringLiteral("custom");
        case SchemaOptionsSource::Defined:
            return QStringLiteral("defined");
    }
    return QString();
}

SchemaOptionsSource schemaOptionsSourceFromString(const QString& value)
{
    const QString key = value.trimmed();
    if (key.isEmpty())
        return SchemaOptionsSource::None;
    if (key == QStringLiteral("tokenKeys"))
        return SchemaOptionsSource::TokenRef;
    if (key == QStringLiteral("tokenKeyOptions"))
        return SchemaOptionsSource::TokenKeyOptions;
    if (key == QStringLiteral("custom"))
        return SchemaOptionsSource::Custom;
    if (key == QStringLiteral("defined"))
        return SchemaOptionsSource::Defined;
    return SchemaOptionsSource::Custom;
}

QVariantMap schemaOptionsSpecToMap(const SchemaOptionsSpec& spec)
{
    QVariantMap map;
    map.insert(QStringLiteral("source"), schemaOptionsSourceToString(spec.source));
    map.insert(QStringLiteral("allowUnset"), spec.allowUnset);
    map.insert(QStringLiteral("unsetLabel"), spec.unsetLabel);
    map.insert(QStringLiteral("fallbackToTextWhenEmpty"), spec.fallbackToTextWhenEmpty);
    map.insert(QStringLiteral("items"), spec.items);
    map.insert(QStringLiteral("sourceKey"), spec.sourceKey);
    map.insert(QStringLiteral("textRole"), spec.textRole);
    map.insert(QStringLiteral("valueRole"), spec.valueRole);
    return map;
}

namespace Options
{
SchemaOptionsSpec tokenKeys()
{
    return SchemaOptionsSpec::none();
}

SchemaOptionsSpec tokenRef(QString fallbackFieldKey)
{
    SchemaOptionsSpec spec;
    spec.source = SchemaOptionsSource::TokenRef;
    spec.allowUnset = true;
    spec.unsetLabel = std::move(fallbackFieldKey);
    spec.sourceKey = "tokenRef";
    spec.textRole = "text";
    spec.valueRole = "value";

    return spec;
}

SchemaOptionsSpec custom(QVariantList items)
{
    return SchemaOptionsSpec{SchemaOptionsSource::Custom, false, QString(), true, {}, "custom", "text", "value"};
}

SchemaOptionsSpec defined(QString sourceKey, QString textRole, QString valueRole)
{
    SchemaOptionsSpec spec;
    spec.source = SchemaOptionsSource::Defined;
    spec.sourceKey = sourceKey;
    // Note: All defined options will have textRole = "text" and valueRole = "value" by default
    spec.textRole = textRole;
    spec.valueRole = valueRole;
    return spec;
}
} // namespace Options
} // namespace cme::runtime