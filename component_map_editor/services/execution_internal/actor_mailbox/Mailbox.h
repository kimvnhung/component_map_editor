#ifndef CME_ACTOR_MAILBOX_H
#define CME_ACTOR_MAILBOX_H

#include <QHash>
#include <QString>
#include <QVariantMap>

#include <deque>
#include <mutex>
#include <vector>

namespace cme::actor
{

// One firing of a component: all incoming edges are already join-gated/merged by the
// scheduler before this message is built, so a DAG actor receives exactly one message
// per execution (see ActorScheduler's join-gate router).
struct Message
{
    QString componentId;
    QHash<QString, QVariantMap> incomingTokens; // keyed by connectionId, same shape as cme::execution::IncomingTokens
};

enum class BackpressurePolicy
{
    BLOCKING,      // sender blocks until enqueued (not implemented in the mutex-based impl; treated as DROP_NEWEST)
    DROP_OLDEST,   // drop oldest message if full
    DROP_NEWEST,   // drop this message if full
    THROTTLE       // return false; sender retries or drops
};

class IMailbox
{
public:
    virtual ~IMailbox() = default;

    // Enqueue a message (thread-safe). Returns false if rejected by backpressure policy.
    virtual bool enqueue(Message &&msg) = 0;

    // Dequeue up to maxCount messages (thread-safe). Returns number of messages dequeued.
    virtual size_t dequeueBatch(std::vector<Message> &out, size_t maxCount) = 0;

    virtual bool empty() const = 0;
    virtual size_t size() const = 0;

    virtual size_t capacity() const = 0;
    virtual BackpressurePolicy backpressurePolicy() const = 0;
};

class Mailbox : public IMailbox
{
public:
    explicit Mailbox(size_t capacity = 1024, BackpressurePolicy policy = BackpressurePolicy::DROP_NEWEST);

    bool enqueue(Message &&msg) override;
    size_t dequeueBatch(std::vector<Message> &out, size_t maxCount) override;
    bool empty() const override;
    size_t size() const override;
    size_t capacity() const override { return m_capacity; }
    BackpressurePolicy backpressurePolicy() const override { return m_policy; }

private:
    mutable std::mutex m_mutex;
    std::deque<Message> m_queue;
    size_t m_capacity;
    BackpressurePolicy m_policy;
};

} // namespace cme::actor

#endif // CME_ACTOR_MAILBOX_H
