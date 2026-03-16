#include "ConnectionModel.h"

ConnectionModel::ConnectionModel(QObject *parent)
    : QObject(parent)
{}

ConnectionModel::ConnectionModel(const QString &id, const QString &sourceId,
                                 const QString &targetId, const QString &label,
                                 QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_sourceId(sourceId)
    , m_targetId(targetId)
    , m_label(label)
{}

QString ConnectionModel::id() const { return m_id; }
void ConnectionModel::setId(const QString &id)
{
    if (m_id == id) return;
    m_id = id;
    emit idChanged();
}

QString ConnectionModel::sourceId() const { return m_sourceId; }
void ConnectionModel::setSourceId(const QString &sourceId)
{
    if (m_sourceId == sourceId) return;
    m_sourceId = sourceId;
    emit sourceIdChanged();
}

QString ConnectionModel::targetId() const { return m_targetId; }
void ConnectionModel::setTargetId(const QString &targetId)
{
    if (m_targetId == targetId) return;
    m_targetId = targetId;
    emit targetIdChanged();
}

QString ConnectionModel::label() const { return m_label; }
void ConnectionModel::setLabel(const QString &label)
{
    if (m_label == label) return;
    m_label = label;
    emit labelChanged();
}

ConnectionModel::Side ConnectionModel::sourceSide() const { return m_sourceSide; }
void ConnectionModel::setSourceSide(ConnectionModel::Side sourceSide)
{
    if (m_sourceSide == sourceSide) return;
    m_sourceSide = sourceSide;
    emit sourceSideChanged();
}

ConnectionModel::Side ConnectionModel::targetSide() const { return m_targetSide; }
void ConnectionModel::setTargetSide(ConnectionModel::Side targetSide)
{
    if (m_targetSide == targetSide) return;
    m_targetSide = targetSide;
    emit targetSideChanged();
}
