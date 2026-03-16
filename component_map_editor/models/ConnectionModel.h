#ifndef CONNECTIONMODEL_H
#define CONNECTIONMODEL_H

#include <QObject>
#include <QString>
#include <QQmlEngine>

class ConnectionModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged FINAL)
    Q_PROPERTY(QString sourceId READ sourceId WRITE setSourceId NOTIFY sourceIdChanged FINAL)
    Q_PROPERTY(QString targetId READ targetId WRITE setTargetId NOTIFY targetIdChanged FINAL)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged FINAL)
    Q_PROPERTY(Side sourceSide READ sourceSide WRITE setSourceSide NOTIFY sourceSideChanged FINAL)
    Q_PROPERTY(Side targetSide READ targetSide WRITE setTargetSide NOTIFY targetSideChanged FINAL)

public:
    enum Side {
        SideAuto = -1,
        SideTop = 0,
        SideRight = 1,
        SideBottom = 2,
        SideLeft = 3
    };
    Q_ENUM(Side)

    explicit ConnectionModel(QObject *parent = nullptr);
    ConnectionModel(const QString &id, const QString &sourceId,
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

    Side sourceSide() const;
    void setSourceSide(Side sourceSide);

    Side targetSide() const;
    void setTargetSide(Side targetSide);

signals:
    void idChanged();
    void sourceIdChanged();
    void targetIdChanged();
    void labelChanged();
    void sourceSideChanged();
    void targetSideChanged();

private:
    QString m_id;
    QString m_sourceId;
    QString m_targetId;
    QString m_label;
    Side m_sourceSide = SideAuto;
    Side m_targetSide = SideAuto;
};

#endif // CONNECTIONMODEL_H
