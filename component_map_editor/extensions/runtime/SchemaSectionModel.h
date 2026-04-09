#ifndef SCHEMASECTIONMODEL_H
#define SCHEMASECTIONMODEL_H

#include <QAbstractListModel>
#include <QVector>

#include "SchemaFieldDefinition.h"

namespace cme::runtime {

struct SchemaSectionDefinition {
    QString id;
    QString title;
    QVector<SchemaFieldDefinition> fields;
};

class SchemaFieldListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        KeyRole = Qt::UserRole + 1,
        TitleRole,
        TypeRole,
        TypeEnumRole,
        WidgetRole,
        WidgetEnumRole,
        RequiredRole,
        DefaultValueRole,
        SectionRole,
        OrderRole,
        HintRole,
        PlaceholderRole,
        OptionsSourceRole,
        OptionsSourceEnumRole,
        OptionsRole,
        VisibleWhenRole,
        ValidationRole,
        ValidRole,
        SchemaErrorRole
    };

    explicit SchemaFieldListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setFields(const QVector<SchemaFieldDefinition> &fields);
    Q_INVOKABLE int size() const;
    Q_INVOKABLE QVariantMap rowAt(int index) const;

private:
    QVector<SchemaFieldDefinition> m_fields;
};

class SchemaSectionListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        FieldsModelRole
    };

    explicit SchemaSectionListModel(QObject *parent = nullptr);
    ~SchemaSectionListModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setSections(const QVector<SchemaSectionDefinition> &sections);
    Q_INVOKABLE int size() const;
    Q_INVOKABLE QVariantMap rowAt(int index) const;

private:
    struct SectionEntry {
        QString id;
        QString title;
        SchemaFieldListModel *fieldsModel = nullptr;
    };

    QVector<SectionEntry> m_sections;
};

} // namespace cme::runtime

#endif // SCHEMASECTIONMODEL_H
