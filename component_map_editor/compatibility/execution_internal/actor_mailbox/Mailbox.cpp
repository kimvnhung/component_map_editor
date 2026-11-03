#include "Mailbox.h"

namespace cme::actor
{

Mailbox::Mailbox(size_t capacity, BackpressurePolicy policy)
    : m_capacity(capacity)
    , m_policy(policy)
{
}

bool Mailbox::enqueue(Message &&msg)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_queue.size() >= m_capacity)
    {
        switch (m_policy)
        {
            case BackpressurePolicy::DROP_OLDEST:
                m_queue.pop_front();
                break;

            case BackpressurePolicy::BLOCKING:
            case BackpressurePolicy::DROP_NEWEST:
            case BackpressurePolicy::THROTTLE:
                return false;
        }
    }

    m_queue.push_back(std::move(msg));
    return true;
}

size_t Mailbox::dequeueBatch(std::vector<Message> &out, size_t maxCount)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const size_t count = std::min(maxCount, m_queue.size());

    for (size_t i = 0; i < count; ++i)
    {
        out.push_back(std::move(m_queue.front()));
        m_queue.pop_front();
    }

    return count;
}

bool Mailbox::empty() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

size_t Mailbox::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

} // namespace cme::actor
