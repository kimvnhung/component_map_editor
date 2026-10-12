#include "SchemaOptionsSpec.h"

#include <QVariantList>

SchemaOptionsSpec SchemaOptionsSpec::none()
{
    return SchemaOptionsSpec{};
}

SchemaOptionsSpec SchemaOptionsSpec::fromSource(QString id)
{
    SchemaOptionsSpec spec;
    spec.sourceId = std::move(id);
    return spec;
}

namespace Options
{
SchemaOptionsSpec tokenKeys()
{
    return SchemaOptionsSpec::fromSource(OptionsSources::TokenKeys);
}

SchemaOptionsSpec tokenRef(QString unsetLabel)
{
    SchemaOptionsSpec spec = SchemaOptionsSpec::fromSource(OptionsSources::TokenKeyOptions);
    spec.allowUnset = true;
    spec.unsetLabel = std::move(unsetLabel);
    return spec;
}

SchemaOptionsSpec custom(QString sourceId)
{
    return SchemaOptionsSpec::fromSource(std::move(sourceId));
}

SchemaOptionsSpec fixed(QVariantList items)
{
    SchemaOptionsSpec spec;
    // spec.sourceId = QStringLiteral("fixed");
    // spec.extra.insert(QStringLiteral("items"), std::move(items));
    return spec;
}
} // namespace Options