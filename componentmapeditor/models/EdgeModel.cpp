#include "EdgeModel.h"

EdgeModel::EdgeModel(QObject *parent)
    : QObject(parent)
{}

EdgeModel::EdgeModel(const QString &id, const QString &sourceId,
                     const QString &targetId, const QString &label,
                     QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_sourceId(sourceId)
    , m_targetId(targetId)
    , m_label(label)
{}

QString EdgeModel::id() const { return m_id; }
void EdgeModel::setId(const QString &id)
{
    if (m_id == id) return;
    m_id = id;
    emit idChanged();
}

QString EdgeModel::sourceId() const { return m_sourceId; }
void EdgeModel::setSourceId(const QString &sourceId)
{
    if (m_sourceId == sourceId) return;
    m_sourceId = sourceId;
    emit sourceIdChanged();
}

QString EdgeModel::targetId() const { return m_targetId; }
void EdgeModel::setTargetId(const QString &targetId)
{
    if (m_targetId == targetId) return;
    m_targetId = targetId;
    emit targetIdChanged();
}

QString EdgeModel::label() const { return m_label; }
void EdgeModel::setLabel(const QString &label)
{
    if (m_label == label) return;
    m_label = label;
    emit labelChanged();
}
