#ifndef IACTIONPROVIDER_H
#define IACTIONPROVIDER_H

#include <QString>
#include <QStringList>
#include <QtPlugin>
#include <QVariantMap>

class IActionProvider
{
public:
    virtual ~IActionProvider() = default;

    virtual QString providerId() const = 0;
    virtual QStringList actionIds() const = 0;

    // Returns metadata for presenting an action in UI.
    // Expected keys include: id, title, category, icon, enabled.
    virtual QVariantMap actionDescriptor(const QString &actionId) const = 0;

    // Produces a command request map consumed by core command gateway.
    // Must not mutate graph state directly.
    virtual bool invokeAction(const QString &actionId,
                              const QVariantMap &context,
                              QVariantMap *commandRequest,
                              QString *error) const = 0;
};

#define COMPONENT_MAP_EDITOR_IID_ACTION_PROVIDER "ComponentMapEditor.Extensions.IActionProvider/1.0"
Q_DECLARE_INTERFACE(IActionProvider, COMPONENT_MAP_EDITOR_IID_ACTION_PROVIDER)

#endif // IACTIONPROVIDER_H
