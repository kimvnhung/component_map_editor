#include "ComponentModel.h"

ComponentModel::ComponentModel(QObject *parent)
    : QObject(parent)
{}

ComponentModel::ComponentModel(const QString &id, const QString &title,
                               qreal x, qreal y,
                               const QString &color, const QString &type,
                               QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_title(title)
    , m_x(x)
    , m_y(y)
    , m_color(color)
    , m_type(type)
{}

bool ComponentModel::setDynamicProperty(const QString &name, const QVariant &value)
{
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty())
        return false;

    const QByteArray utf8Name = trimmedName.toUtf8();
    return QObject::setProperty(utf8Name.constData(), value);
}

bool ComponentModel::hasDynamicProperty(const QString &name) const
{
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty())
        return false;

    const QByteArray utf8Name = trimmedName.toUtf8();
    return QObject::dynamicPropertyNames().contains(utf8Name);
}

QVariant ComponentModel::dynamicPropertyValue(const QString &name) const
{
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty())
        return {};

    const QByteArray utf8Name = trimmedName.toUtf8();
    return QObject::property(utf8Name.constData());
}

bool ComponentModel::applyRuntimeDescriptor(const QVariantMap &descriptor)
{
    const QString identity = descriptor.value(QStringLiteral("identity")).toString().trimmed();
    const QString version = descriptor.value(QStringLiteral("version")).toString().trimmed();
    if (identity.isEmpty() || version.isEmpty())
        return false;

    setComponentIdentity(identity);
    setComponentVersion(version);
    setType(identity);

    if (descriptor.contains(QStringLiteral("title")))
        setTitle(descriptor.value(QStringLiteral("title")).toString());
    if (descriptor.contains(QStringLiteral("icon")))
        setIcon(descriptor.value(QStringLiteral("icon")).toString());
    if (descriptor.contains(QStringLiteral("shape")))
        setShape(descriptor.value(QStringLiteral("shape")).toString());
    if (descriptor.contains(QStringLiteral("defaultWidth")))
        setWidth(descriptor.value(QStringLiteral("defaultWidth")).toReal());
    if (descriptor.contains(QStringLiteral("defaultHeight")))
        setHeight(descriptor.value(QStringLiteral("defaultHeight")).toReal());
    if (descriptor.contains(QStringLiteral("properties")))
        setProperties(descriptor.value(QStringLiteral("properties")).toList());
    if (descriptor.contains(QStringLiteral("outputProperties")))
        setOutputProperties(descriptor.value(QStringLiteral("outputProperties")).toList());
    if (descriptor.contains(QStringLiteral("ports")))
        setPorts(descriptor.value(QStringLiteral("ports")).toList());
    if (descriptor.contains(QStringLiteral("execution")))
        setExecutionMetadata(descriptor.value(QStringLiteral("execution")).toMap());
    if (descriptor.contains(QStringLiteral("metadataSnapshot")))
        setMetadataSnapshot(descriptor.value(QStringLiteral("metadataSnapshot")).toString());

    const QVariantMap defaults = descriptor.value(QStringLiteral("defaultProperties")).toMap();
    for (auto it = defaults.constBegin(); it != defaults.constEnd(); ++it)
        setDynamicProperty(it.key(), it.value());
    return true;
}

QVariantMap ComponentModel::runtimeDescriptor() const
{
    QVariantMap result;
    result.insert(QStringLiteral("identity"), m_componentIdentity);
    result.insert(QStringLiteral("version"), m_componentVersion);
    result.insert(QStringLiteral("properties"), m_properties);
    result.insert(QStringLiteral("outputProperties"), m_outputProperties);
    result.insert(QStringLiteral("ports"), m_ports);
    result.insert(QStringLiteral("execution"), m_executionMetadata);
    if (!m_metadataSnapshot.isEmpty())
        result.insert(QStringLiteral("metadataSnapshot"), m_metadataSnapshot);
    return result;
}

QString ComponentModel::id() const { return m_id; }
void ComponentModel::setId(const QString &id)
{
    if (m_id == id) return;
    m_id = id;
    emit idChanged();
}

QString ComponentModel::title() const { return m_title; }
void ComponentModel::setTitle(const QString &title)
{
    if (m_title == title) return;
    m_title = title;
    emit titleChanged();
}

QString ComponentModel::content() const { return m_content; }
void ComponentModel::setContent(const QString &content)
{
    if (m_content == content) return;
    m_content = content;
    emit contentChanged();
}

QString ComponentModel::icon() const { return m_icon; }
void ComponentModel::setIcon(const QString &icon)
{
    if (m_icon == icon) return;
    m_icon = icon;
    emit iconChanged();
}

qreal ComponentModel::x() const { return m_x; }
void ComponentModel::setX(qreal x)
{
    if (qFuzzyCompare(m_x, x)) return;
    m_x = x;
    emit xChanged();
}

qreal ComponentModel::y() const { return m_y; }
void ComponentModel::setY(qreal y)
{
    if (qFuzzyCompare(m_y, y)) return;
    m_y = y;
    emit yChanged();
}

qreal ComponentModel::width() const { return m_width; }
void ComponentModel::setWidth(qreal width)
{
    if (width < 1.0)
        width = 1.0;
    if (qFuzzyCompare(m_width, width)) return;
    m_width = width;
    emit widthChanged();
}

qreal ComponentModel::height() const { return m_height; }
void ComponentModel::setHeight(qreal height)
{
    if (height < 1.0)
        height = 1.0;
    if (qFuzzyCompare(m_height, height)) return;
    m_height = height;
    emit heightChanged();
}

QString ComponentModel::shape() const { return m_shape; }
void ComponentModel::setShape(const QString &shape)
{
    const QString normalized = (shape == QStringLiteral("rectangle"))
        ? QStringLiteral("rectangle")
        : QStringLiteral("rounded");

    if (m_shape == normalized) return;
    m_shape = normalized;
    emit shapeChanged();
}

QString ComponentModel::color() const { return m_color; }
void ComponentModel::setColor(const QString &color)
{
    if (m_color == color) return;
    m_color = color;
    emit colorChanged();
}

QString ComponentModel::type() const { return m_type; }
void ComponentModel::setType(const QString &type)
{
    if (m_type == type) return;
    m_type = type;
    emit typeChanged();
}

QString ComponentModel::componentIdentity() const { return m_componentIdentity; }
void ComponentModel::setComponentIdentity(const QString &identity)
{
    if (m_componentIdentity == identity) return;
    m_componentIdentity = identity;
    emit componentIdentityChanged();
}

QString ComponentModel::componentVersion() const { return m_componentVersion; }
void ComponentModel::setComponentVersion(const QString &version)
{
    if (m_componentVersion == version) return;
    m_componentVersion = version;
    emit componentVersionChanged();
}

QString ComponentModel::metadataSnapshot() const { return m_metadataSnapshot; }
void ComponentModel::setMetadataSnapshot(const QString &snapshot)
{
    if (m_metadataSnapshot == snapshot) return;
    m_metadataSnapshot = snapshot;
    emit metadataSnapshotChanged();
}

QVariantList ComponentModel::properties() const { return m_properties; }
void ComponentModel::setProperties(const QVariantList &properties)
{
    if (m_properties == properties) return;
    m_properties = properties;
    emit propertiesChanged();
}

QVariantList ComponentModel::outputProperties() const { return m_outputProperties; }
void ComponentModel::setOutputProperties(const QVariantList &properties)
{
    if (m_outputProperties == properties) return;
    m_outputProperties = properties;
    emit outputPropertiesChanged();
}

QVariantList ComponentModel::ports() const { return m_ports; }
void ComponentModel::setPorts(const QVariantList &ports)
{
    if (m_ports == ports) return;
    m_ports = ports;
    emit portsChanged();
}

QVariantMap ComponentModel::executionMetadata() const { return m_executionMetadata; }
void ComponentModel::setExecutionMetadata(const QVariantMap &metadata)
{
    if (m_executionMetadata == metadata) return;
    m_executionMetadata = metadata;
    emit executionMetadataChanged();
}
