#include "TimelineModel.h"

#include <QJsonObject>
#include <QRegularExpression>

#define MAX_VISIBLE_ROWS 1000

void TimelineStorage::append(TimelineEventEntry &&entry)
{
    m_events.push_back(std::move(entry));
}

const TimelineEventEntry &TimelineStorage::at(size_t index) const
{
    return m_events.at(index);
}

size_t TimelineStorage::size() const
{
    return m_events.size();
}

TimelineModel::TimelineModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_storage(new TimelineStorage())
{
}

QString TimelineModel::regexFilter() const
{
    return m_regexFilter;
}

void TimelineModel::setRegexFilter(const QString &regex)
{
    if (m_regexFilter == regex)
    {
        return;
    }

    m_regexFilter = regex;
    emit regexFilterChanged();

    beginResetModel();
    m_visibleRows.clear();

    QRegularExpression regexObj(m_regexFilter);

    for (size_t i = 0; i < m_storage->size(); ++i)
    {
        const auto &item = m_storage->at(i);
        QJsonObject obj = QJsonObject::fromVariantMap(item.payload);
        QJsonDocument doc(obj);
        QString payload = doc.toJson(QJsonDocument::Compact);

        QRegularExpressionMatch match = regexObj.match(payload);

        if (match.hasMatch())
        {
            m_visibleRows.push_back(i);

            if (m_visibleRows.size() > MAX_VISIBLE_ROWS)
            {
                m_visibleRows.erase(m_visibleRows.begin());
            }
        }
    }

    endResetModel();
}

int TimelineModel::rowCount(const QModelIndex &) const
{
    return m_visibleRows.size();
}

QVariant TimelineModel::data(
    const QModelIndex &index,
    int role) const
{
    if (!index.isValid())
        return {};

    const int row = index.row();

    if (row < 0 || row >= m_visibleRows.size())
    {
        return {};
    }

    size_t idx = m_visibleRows[row];
    auto item = m_storage->at(idx);

    switch (role)
    {
        case EventRole:
            return item.event;

        case TickRole:
            return item.tick;

        case PayloadRole:
            return item.payload;
    }

    return {};
}

QHash<int, QByteArray> TimelineModel::roleNames() const
{
    return
    {
        {EventRole, "event"},
        {TickRole, "tick"},
        {PayloadRole, "payload"},
    };
}

void TimelineModel::append(const TimelineEventEntry& entry)
{
    beginInsertRows(QModelIndex(), m_visibleRows.size(), m_visibleRows.size());
    m_storage->append(TimelineEventEntry(entry));
    QJsonObject obj = QJsonObject::fromVariantMap(entry.payload);

    QJsonDocument doc(obj);

    QString payload = doc.toJson(QJsonDocument::Compact);
    QRegularExpression regex(m_regexFilter);
    QRegularExpressionMatch match = regex.match(payload);

    if (match.hasMatch())
    {
        m_visibleRows.push_back(m_storage->size() - 1);

        if (m_visibleRows.size() > MAX_VISIBLE_ROWS)
        {
            m_visibleRows.erase(m_visibleRows.begin());
        }
    }

    endInsertRows();
}

void TimelineModel::clear()
{
    beginResetModel();

    m_storage = new TimelineStorage();
    m_visibleRows.clear();

    endResetModel();
}

const TimelineEventEntry &
TimelineModel::at(int row) const
{
    size_t idx = m_visibleRows[row];
    return m_storage->at(idx);
}


void TimelineModel::append(TimelineEventEntry && entry)
{
    beginInsertRows(QModelIndex(), m_visibleRows.size(), m_visibleRows.size());
    m_storage->append(std::move(entry));
    QJsonObject obj = QJsonObject::fromVariantMap(entry.payload);

    QJsonDocument doc(obj);

    QString payload = doc.toJson(QJsonDocument::Compact);
    QRegularExpression regex(m_regexFilter);
    QRegularExpressionMatch match = regex.match(payload);

    if (match.hasMatch())
    {
        m_visibleRows.push_back(m_storage->size() - 1);

        if (m_visibleRows.size() > MAX_VISIBLE_ROWS)
        {
            m_visibleRows.erase(m_visibleRows.begin());
        }
    }

    endInsertRows();
}
