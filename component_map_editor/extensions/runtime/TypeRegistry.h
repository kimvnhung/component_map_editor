#ifndef TYPEREGISTRY_H
#define TYPEREGISTRY_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QQmlEngine>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/IComponentTypeProvider.h"
#include "extensions/contracts/IConnectionPolicyProvider.h"
#include "extensions/contracts/IConnectionPolicyProviderV2.h"
#include "policy.pb.h"

/**
 * TypeRegistry builds an O(1) lookup cache from all IComponentTypeProvider and
 * IConnectionPolicyProvider instances registered in an ExtensionContractRegistry.
 *
 * Call rebuildFromRegistry() once after all extension packs are loaded.
 * After that, all lookup methods are O(1) hash lookups.
 *
 * Exposed to QML so the presentation layer can enumerate available component types
 * without any hardcoded lists in core code.
 */
class TypeRegistry : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    // Ordered list of component-type descriptor maps — bind this in QML for a
    // dynamic component-type palette. Each map contains the keys returned by
    // IComponentTypeProvider::componentTypeDescriptor().
    Q_PROPERTY(QVariantList componentTypeDescriptors READ componentTypeDescriptors
                   NOTIFY componentTypesChanged FINAL)

    // Flat list of registered component type IDs (ordering follows provider
    // registration order, then type iteration order within each provider).
    Q_PROPERTY(QStringList componentTypeIds READ componentTypeIds
                   NOTIFY componentTypesChanged FINAL)

public:
    explicit TypeRegistry(QObject *parent = nullptr);

    /**
     * Rebuild the internal cache from @p registry.
     * Thread-unsafe — call from the main thread only.
    * Emits componentTypesChanged() if the set of component types changes.
     */
    void rebuildFromRegistry(const ExtensionContractRegistry &registry);

    // ---------- Component-type lookups (O(1) after rebuild) -----------------

    Q_INVOKABLE bool hasComponentType(const QString &typeId) const;
    Q_INVOKABLE QVariantMap componentTypeDescriptor(const QString &typeId) const;
    Q_INVOKABLE QVariantMap defaultComponentProperties(const QString &typeId) const;

    QStringList  componentTypeIds()         const;
    QVariantList componentTypeDescriptors() const;

    // ---------- Connection-policy queries -----------------------------------

    /**
     * Returns true if all registered IConnectionPolicyProvider instances allow
     * the connection.  First-deny wins: as soon as one provider denies,
     * @p reason (if non-null) is populated and the method returns false.
     *
     * If no policy provider is registered, every connection is allowed
     * (open-world assumption — the library never blocks operations it knows
     * nothing about).
     */
    bool canConnect(const QString &srcTypeId,
                    const QString &tgtTypeId,
                    const QVariantMap &context = {},
                    QString *reason = nullptr) const;

    // Typed external entrypoint. Preferred for integrations outside the library.
    bool canConnect(const cme::ConnectionPolicyContext &context,
                    QString *reason = nullptr) const;

    /**
     * Runs rawProperties through each registered policy provider's
     * normalizeConnectionProperties() in registration order and returns the
     * final merged result.  Later providers may override keys set by earlier
     * ones.
     */
    QVariantMap normalizeConnectionProperties(const QString &srcTypeId,
                                              const QString &tgtTypeId,
                                              const QVariantMap &rawProps = {}) const;

    // Typed external entrypoint. Preferred for integrations outside the library.
    QVariantMap normalizeConnectionProperties(const cme::ConnectionPolicyContext &context,
                                              const QVariantMap &rawProps = {}) const;

signals:
    void componentTypesChanged();

private:
    // Component-type cache (populated by rebuildFromRegistry)
    QHash<QString, QVariantMap> m_descriptors; // typeId → descriptor map
    QHash<QString, QVariantMap> m_defaults;    // typeId → default property map
    QStringList                 m_orderedTypeIds;

    // Connection-policy provider list (ordered; non-owning, typed canonical path)
    QList<const IConnectionPolicyProviderV2 *> m_connectionPoliciesV2;
};

#endif // TYPEREGISTRY_H
