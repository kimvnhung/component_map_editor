#ifndef MAILBOX_H
#define MAILBOX_H

#include <QString>
#include <QVariantMap>

#include <cstdint>
#include <deque>
#include <vector>
#include <mutex>

struct Message
{
    uint64_t id;
    QString sourceId;
    QVariantMap tokens;
};

enum class BackpressurePolicy
{
    BLOCKING,           // sender blocks until enqueued
    DROP_OLDEST,        // drop oldest message if full
    DROP_NEWEST,        // drop this message if full
    THROTTLE            // return false; sender retries or drops
};

class IMailbox
{
public:
    virtual ~IMailbox() = default;

    // Enqueue a message (thread-safe)
    // Return: true if enqueued, false if rejected (backpressure)
    virtual bool enqueue(Message&& msg) = 0;

    // Dequeue up to maxCount messages (thread-safe)
    // Return: number of messages dequeued
    virtual size_t dequeueBatch(std::vector<Message> &out, size_t maxCount) = 0;

    // Query state (approximate, may be stale)
    virtual bool empty() const = 0;
    virtual size_t size() const = 0;

    // Capacity and backpressure config
    virtual size_t capacity() const = 0;
    virtual BackpressurePolicy backpressurePolicy() const = 0;
};

// Concrete impl: mutex-based mailbox (PoC → production)
class MailboxImpl : public IMailbox
{
public:
    explicit MailboxImpl(size_t capacity = 1024,
                         BackpressurePolicy policy = BackpressurePolicy::DROP_NEWEST);

    bool enqueue(Message&& msg) override;
    size_t dequeueBatch(std::vector<Message> &out, size_t maxCount) override;
    bool empty() const override;
    size_t size() const override;
    size_t capacity() const override { return capacity_; }
    BackpressurePolicy backpressurePolicy() const override { return policy_; }

private:
    mutable std::mutex mu_;
    std::deque<Message> queue_;
    size_t capacity_;
    BackpressurePolicy policy_;
};
#endif // MAILBOX_H
