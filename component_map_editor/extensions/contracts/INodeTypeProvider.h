#ifndef INODETYPEPROVIDER_H
#define INODETYPEPROVIDER_H

#include <QString>
#include <QStringList>
#include <QtPlugin>
#include <QVariantMap>

class INodeTypeProvider
{
public:
    virtual ~INodeTypeProvider() = default;

    virtual QString providerId() const = 0;
    virtual QStringList nodeTypeIds() const = 0;

    // Returns metadata for a node type. Expected keys include:
    // id, title, category, ports, constraints.
    virtual QVariantMap nodeTypeDescriptor(const QString &nodeTypeId) const = 0;

    // Returns default property values for new nodes of this type.
    virtual QVariantMap defaultNodeProperties(const QString &nodeTypeId) const = 0;
};

#define COMPONENT_MAP_EDITOR_IID_NODE_TYPE_PROVIDER "ComponentMapEditor.Extensions.INodeTypeProvider/1.0"
Q_DECLARE_INTERFACE(INodeTypeProvider, COMPONENT_MAP_EDITOR_IID_NODE_TYPE_PROVIDER)

#endif // INODETYPEPROVIDER_H
