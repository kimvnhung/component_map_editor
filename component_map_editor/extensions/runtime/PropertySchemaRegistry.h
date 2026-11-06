#ifndef PROPERTYSCHEMAREGISTRY_H
#define PROPERTYSCHEMAREGISTRY_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QQmlEngine>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "SchemaSectionModel.h"
#include "public_api.pb.h"

class PropertySchemaRegistry : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit PropertySchemaRegistry(QObject *parent = nullptr);

    // Rebuilds the in-memory schema cache from registered providers.
    void rebuildFromRegistry(const ExtensionContractRegistry &registry);

    // Runtime descriptors are authoritative after a successful catalog refresh.
    // Provider schemas remain available as the offline fallback until then.
    void replaceFromRuntimeCatalog(const QVariantList &descriptors);
    bool hasRuntimeCatalog() const { return m_hasRuntimeCatalog; }

    Q_INVOKABLE bool hasTarget(const QString &targetId) const;
    Q_INVOKABLE QVariantList schemaRowsForTarget(const QString &targetId) const;
    Q_INVOKABLE QVariantList componentSchemaRows(const QString &componentIdentity) const;

    // Typed external entrypoint. Preferred for integrations outside the library.
    bool schemaForTargetTyped(const QString &targetId,
                              cme::publicapi::v1::PropertySchemaResponse *out,
                              QString *error = nullptr) const;

    // Typed model entrypoint for QML integrations.

    Q_INVOKABLE QObject *typedSectionModelForTarget(const QString &targetId);


    // Typed schema entrypoints.
    bool componentSchemaTyped(const QString &componentTypeId,
                              cme::publicapi::v1::PropertySchemaResponse *out,
                              QString *error = nullptr) const;
    bool connectionSchemaTyped(const QString &connectionTypeId,
                               cme::publicapi::v1::PropertySchemaResponse *out,
                               QString *error = nullptr) const;



signals:
    void schemasChanged();

private:
    static QVariantMap normalizeFieldRow(const QVariantMap &raw, const QString &targetId);
    static QVariantList normalizeRows(const QVariantList &rows, const QString &targetId);
    static QVariantList sectionizeRows(const QVariantList &rows);
    static QVector<cme::runtime::SchemaSectionDefinition> sectionizeTypedRows(const QVariantList &rows,
                                                                               const QString &targetId);

    static QVariantList fallbackComponentRows();
    static QVariantList fallbackConnectionRows();

    QVariantList resolvedSchemaRows(const QString &targetId) const;

    QHash<QString, QVariantList> m_rowsByTarget;
    QHash<QString, QVariantList> m_runtimeRowsByIdentity;
    QHash<QString, cme::runtime::SchemaSectionListModel *> m_sectionModelsByTarget;
    bool m_hasRuntimeCatalog = false;
};

#endif // PROPERTYSCHEMAREGISTRY_H
