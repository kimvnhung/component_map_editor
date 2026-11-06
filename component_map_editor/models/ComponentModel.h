#ifndef COMPONENTMODEL_H
#define COMPONENTMODEL_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QQmlEngine>

class ComponentModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged FINAL)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(QString content READ content WRITE setContent NOTIFY contentChanged FINAL)
    Q_PROPERTY(QString icon READ icon WRITE setIcon NOTIFY iconChanged FINAL)
    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged FINAL)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged FINAL)
    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged FINAL)
    Q_PROPERTY(qreal height READ height WRITE setHeight NOTIFY heightChanged FINAL)
    Q_PROPERTY(QString shape READ shape WRITE setShape NOTIFY shapeChanged FINAL)
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged FINAL)
    Q_PROPERTY(QString type READ type WRITE setType NOTIFY typeChanged FINAL)
    Q_PROPERTY(QString componentIdentity READ componentIdentity WRITE setComponentIdentity NOTIFY componentIdentityChanged FINAL)
    Q_PROPERTY(QString componentVersion READ componentVersion WRITE setComponentVersion NOTIFY componentVersionChanged FINAL)
    Q_PROPERTY(QString metadataSnapshot READ metadataSnapshot WRITE setMetadataSnapshot NOTIFY metadataSnapshotChanged FINAL)
    Q_PROPERTY(QVariantList properties READ properties WRITE setProperties NOTIFY propertiesChanged FINAL)
    Q_PROPERTY(QVariantList outputProperties READ outputProperties WRITE setOutputProperties NOTIFY outputPropertiesChanged FINAL)
    Q_PROPERTY(QVariantList ports READ ports WRITE setPorts NOTIFY portsChanged FINAL)
    Q_PROPERTY(QVariantMap executionMetadata READ executionMetadata WRITE setExecutionMetadata NOTIFY executionMetadataChanged FINAL)

public:
    explicit ComponentModel(QObject *parent = nullptr);
    ComponentModel(const QString &id, const QString &title,
                   qreal x, qreal y,
                   const QString &color = QStringLiteral("#4fc3f7"),
                   const QString &type = QStringLiteral("default"),
                   QObject *parent = nullptr);

    Q_INVOKABLE bool setDynamicProperty(const QString &name, const QVariant &value);
    Q_INVOKABLE bool hasDynamicProperty(const QString &name) const;
    Q_INVOKABLE QVariant dynamicPropertyValue(const QString &name) const;
    Q_INVOKABLE bool applyRuntimeDescriptor(const QVariantMap &descriptor);
    Q_INVOKABLE QVariantMap runtimeDescriptor() const;

    QString id() const;
    void setId(const QString &id);
    QString title() const;
    void setTitle(const QString &title);
    QString content() const;
    void setContent(const QString &content);
    QString icon() const;
    void setIcon(const QString &icon);
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

    QString componentIdentity() const;
    void setComponentIdentity(const QString &identity);
    QString componentVersion() const;
    void setComponentVersion(const QString &version);
    QString metadataSnapshot() const;
    void setMetadataSnapshot(const QString &snapshot);
    QVariantList properties() const;
    void setProperties(const QVariantList &properties);
    QVariantList outputProperties() const;
    void setOutputProperties(const QVariantList &properties);
    QVariantList ports() const;
    void setPorts(const QVariantList &ports);
    QVariantMap executionMetadata() const;
    void setExecutionMetadata(const QVariantMap &metadata);

signals:
    void idChanged();
    void titleChanged();
    void contentChanged();
    void iconChanged();
    void xChanged();
    void yChanged();
    void widthChanged();
    void heightChanged();
    void shapeChanged();
    void colorChanged();
    void typeChanged();
    void componentIdentityChanged();
    void componentVersionChanged();
    void metadataSnapshotChanged();
    void propertiesChanged();
    void outputPropertiesChanged();
    void portsChanged();
    void executionMetadataChanged();

private:
    QString m_id;
    QString m_title;
    QString m_content;
    QString m_icon;
    qreal m_x = 0.0;
    qreal m_y = 0.0;
    qreal m_width = 96.0;
    qreal m_height = 96.0;
    QString m_shape{ QStringLiteral("rounded") };
    QString m_color{ QStringLiteral("#4fc3f7") };
    QString m_type{ QStringLiteral("default") };
    QString m_componentIdentity;
    QString m_componentVersion;
    QString m_metadataSnapshot;
    QVariantList m_properties;
    QVariantList m_outputProperties;
    QVariantList m_ports;
    QVariantMap m_executionMetadata;
};

#endif // COMPONENTMODEL_H
