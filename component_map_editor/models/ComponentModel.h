#ifndef COMPONENTMODEL_H
#define COMPONENTMODEL_H

#include <QObject>
#include <QString>
#include <QQmlEngine>

class ComponentModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged FINAL)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged FINAL)
    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged FINAL)
    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged FINAL)
    Q_PROPERTY(qreal height READ height WRITE setHeight NOTIFY heightChanged FINAL)
    Q_PROPERTY(QString shape READ shape WRITE setShape NOTIFY shapeChanged FINAL)
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(QString type READ type WRITE setType NOTIFY typeChanged FINAL)

public:
    explicit ComponentModel(QObject *parent = nullptr);
    ComponentModel(const QString &id, const QString &label,
                   qreal x, qreal y,
                   const QString &color = QStringLiteral("#4fc3f7"),
                   const QString &type = QStringLiteral("default"),
                   QObject *parent = nullptr);

    QString id() const;
    void setId(const QString &id);

    QString label() const;
    void setLabel(const QString &label);

    qreal x() const;
    void setX(qreal x);

    qreal y() const;
    void setY(qreal y);

    qreal width() const;
    void setWidth(qreal width);

    qreal height() const;
    void setHeight(qreal height);

    QString shape() const;
    void setShape(const QString &shape);

    QString color() const;
    void setColor(const QString &color);

    QString type() const;
    void setType(const QString &type);

signals:
    void idChanged();
    void labelChanged();
    void xChanged();
    void yChanged();
    void widthChanged();
    void heightChanged();
    void shapeChanged();
    void colorChanged();
    void typeChanged();

private:
    QString m_id;
    QString m_label;
    qreal m_x = 0.0;
    qreal m_y = 0.0;
    qreal m_width = 120.0;
    qreal m_height = 40.0;
    QString m_shape{ QStringLiteral("rounded") };
    QString m_color{ QStringLiteral("#4fc3f7") };
    QString m_type{ QStringLiteral("default") };
};

#endif // COMPONENTMODEL_H
