#include "SchemaOptionsSpec.h"

#include <QVariantList>

namespace cme::runtime
{

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
    // TODO: Need improve later, just hard code for building
    return SchemaOptionsSpec::fromSource("tokenKeys");
}

SchemaOptionsSpec tokenRef(QString unsetLabel)
{
    SchemaOptionsSpec spec = SchemaOptionsSpec::fromSource("tokenKeyOptions");
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
    spec.sourceId = QStringLiteral("fixed");
    spec.items = std::move(items);
    return spec;
}
} // namespace Options
} // namespace cme::runtime