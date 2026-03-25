#ifndef PROPERTYSCHEMAREGISTRY_H
#define PROPERTYSCHEMAREGISTRY_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QQmlEngine>

#include "extensions/contracts/ExtensionContractRegistry.h"

class PropertySchemaRegistry : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit PropertySchemaRegistry(QObject *parent = nullptr);

    // Rebuilds the in-memory schema cache from registered providers.
    void rebuildFromRegistry(const ExtensionContractRegistry &registry);

    Q_INVOKABLE bool hasTarget(const QString &targetId) const;

    // Returns normalized field rows for a target.
    Q_INVOKABLE QVariantList schemaForTarget(const QString &targetId) const;

    // Returns normalized section list:
    // [ { id, title, fields:[...normalized rows...] }, ... ]
    Q_INVOKABLE QVariantList sectionedSchemaForTarget(const QString &targetId) const;

    Q_INVOKABLE QVariantList componentSchema(const QString &componentTypeId) const;
    Q_INVOKABLE QVariantList connectionSchema(const QString &connectionTypeId) const;

signals:
    void schemasChanged();

private:
    static QVariantMap normalizeFieldRow(const QVariantMap &raw, const QString &targetId);
    static QVariantList normalizeRows(const QVariantList &rows, const QString &targetId);
    static QVariantList sectionizeRows(const QVariantList &rows);

    static QVariantList fallbackComponentRows();
    static QVariantList fallbackConnectionRows();

    QVariantList resolvedSchemaRows(const QString &targetId) const;

    QHash<QString, QVariantList> m_rowsByTarget;
};

#endif // PROPERTYSCHEMAREGISTRY_H
