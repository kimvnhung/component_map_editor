#include "ComponentModel.h"

ComponentModel::ComponentModel(QObject *parent)
    : QObject(parent)
{}

ComponentModel::ComponentModel(const QString &id, const QString &label,
                               qreal x, qreal y,
                               const QString &color, const QString &type,
                               QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_label(label)
    , m_x(x)
    , m_y(y)
    , m_color(color)
    , m_type(type)
{}

QString ComponentModel::id() const { return m_id; }
void ComponentModel::setId(const QString &id)
{
    if (m_id == id) return;
    m_id = id;
    emit idChanged();
}

QString ComponentModel::label() const { return m_label; }
void ComponentModel::setLabel(const QString &label)
{
    if (m_label == label) return;
    m_label = label;
    emit labelChanged();
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
