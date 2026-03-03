#ifndef EDGEMODEL_H
#define EDGEMODEL_H

#include <QObject>
#include <QString>
#include <QQmlEngine>

class EdgeModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged FINAL)
    Q_PROPERTY(QString sourceId READ sourceId WRITE setSourceId NOTIFY sourceIdChanged FINAL)
    Q_PROPERTY(QString targetId READ targetId WRITE setTargetId NOTIFY targetIdChanged FINAL)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged FINAL)

public:
    explicit EdgeModel(QObject *parent = nullptr);
    EdgeModel(const QString &id, const QString &sourceId,
              const QString &targetId, const QString &label = QString(),
              QObject *parent = nullptr);

    QString id() const;
    void setId(const QString &id);

    QString sourceId() const;
    void setSourceId(const QString &sourceId);

    QString targetId() const;
    void setTargetId(const QString &targetId);

    QString label() const;
    void setLabel(const QString &label);

signals:
    void idChanged();
    void sourceIdChanged();
    void targetIdChanged();
    void labelChanged();

private:
    QString m_id;
    QString m_sourceId;
    QString m_targetId;
    QString m_label;
};

#endif // EDGEMODEL_H
