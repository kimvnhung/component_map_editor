#include "Mailbox.h"

MailboxImpl::MailboxImpl(size_t capacity, BackpressurePolicy policy)
    : capacity_(capacity), policy_(policy)
{
}

bool MailboxImpl::enqueue(Message&& msg)
{
    std::lock_guard<std::mutex> lock(mu_);

    if (queue_.size() >= capacity_)
    {
        switch (policy_)
        {
            case BackpressurePolicy::BLOCKING:
                // In a real implementation, you might block here until space is available.
                return false;

            case BackpressurePolicy::DROP_OLDEST:
                queue_.pop_front();
                break;

            case BackpressurePolicy::DROP_NEWEST:
                return false;

            case BackpressurePolicy::THROTTLE:
                return false;
        }
    }

    queue_.push_back(std::move(msg));
    return true;
}

size_t MailboxImpl::dequeueBatch(std::vector<Message> &out, size_t maxCount)
{
    std::lock_guard<std::mutex> lock(mu_);
    size_t count = std::min(maxCount, queue_.size());

    for (size_t i = 0; i < count; ++i)
    {
        out.push_back(std::move(queue_.front()));
        queue_.pop_front();
    }

    return count;
}

bool MailboxImpl::empty() const
{
    std::lock_guard<std::mutex> lock(mu_);
    return queue_.empty();
}

size_t MailboxImpl::size() const
{
    std::lock_guard<std::mutex> lock(mu_);
    return queue_.size();
}

