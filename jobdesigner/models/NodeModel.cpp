#include "NodeModel.h"

NodeModel::NodeModel(QObject *parent)
    : QObject(parent)
{}

NodeModel::NodeModel(const QString &id, const QString &label,
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

QString NodeModel::id() const { return m_id; }
void NodeModel::setId(const QString &id)
{
    if (m_id == id) return;
    m_id = id;
    emit idChanged();
}

QString NodeModel::label() const { return m_label; }
void NodeModel::setLabel(const QString &label)
{
    if (m_label == label) return;
    m_label = label;
    emit labelChanged();
}

qreal NodeModel::x() const { return m_x; }
void NodeModel::setX(qreal x)
{
    if (qFuzzyCompare(m_x, x)) return;
    m_x = x;
    emit xChanged();
}

qreal NodeModel::y() const { return m_y; }
void NodeModel::setY(qreal y)
{
    if (qFuzzyCompare(m_y, y)) return;
    m_y = y;
    emit yChanged();
}

QString NodeModel::color() const { return m_color; }
void NodeModel::setColor(const QString &color)
{
    if (m_color == color) return;
    m_color = color;
    emit colorChanged();
}

QString NodeModel::type() const { return m_type; }
void NodeModel::setType(const QString &type)
{
    if (m_type == type) return;
    m_type = type;
    emit typeChanged();
}
