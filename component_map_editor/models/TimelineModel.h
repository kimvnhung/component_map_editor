#ifndef TIMELINEMODEL_H
#define TIMELINEMODEL_H

#include <QObject>
#include <QVariantMap>
#include <QAbstractListModel>
#include <deque>

struct TimelineEventEntry
{
    QString event;
    int tick;
    QVariantMap payload;
};

class TimelineStorage
{
public:

    void append(TimelineEventEntry &&);

    const TimelineEventEntry &at(size_t index) const;

    size_t size() const;

private:

    std::deque<TimelineEventEntry> m_events;
};

class TimelineModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles
    {
        EventRole = Qt::UserRole,
        TickRole,
        PayloadRole
    };

    explicit TimelineModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index,
                  int role) const override;

    QHash<int, QByteArray> roleNames() const override;
public:

    void append(const TimelineEventEntry& entry);

    void append(TimelineEventEntry&& entry);

    void clear();

    const TimelineEventEntry &at(int row) const;

private:

    TimelineStorage *m_storage;

    std::vector<size_t> m_visibleRows;
};

#endif // TIMELINEMODEL_H
