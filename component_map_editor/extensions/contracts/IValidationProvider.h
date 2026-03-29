#ifndef IVALIDATIONPROVIDER_H
#define IVALIDATIONPROVIDER_H

#include <QString>
#include <QtPlugin>
#include <QVariantList>
#include <QVariantMap>

class IValidationProvider
{
public:
    virtual ~IValidationProvider() = default;

#if defined(__cplusplus) && __cplusplus >= 201402L
    [[deprecated("IValidationProvider (V1) is deprecated. Use IValidationProviderV2 with GraphSnapshot/GraphValidationResult.")]]
#endif
    virtual QString providerId() const = 0;

    // Returns validation issues. Expected keys per issue:
    // code, severity, message, entityType, entityId.
#if defined(__cplusplus) && __cplusplus >= 201402L
    [[deprecated("validateGraph(QVariantMap) is deprecated. Use IValidationProviderV2::validateGraph with typed proto contracts.")]]
#endif
    virtual QVariantList validateGraph(const QVariantMap &graphSnapshot) const = 0;
};

#define COMPONENT_MAP_EDITOR_IID_VALIDATION_PROVIDER "ComponentMapEditor.Extensions.IValidationProvider/1.0"
Q_DECLARE_INTERFACE(IValidationProvider, COMPONENT_MAP_EDITOR_IID_VALIDATION_PROVIDER)

#endif // IVALIDATIONPROVIDER_H
