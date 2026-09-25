#include "Message.h"

Message::Message(const std::unordered_map<std::string, std::string> &payload)
    : m_payload(payload) {}

void Message::setPayload(const std::unordered_map<std::string, std::string> &payload)
{
    m_payload = payload;
}

void Message::addPayloadField(const std::string &key, const std::string &value)
{
    m_payload[key] = value;
}

void Message::removePayloadField(const std::string &key)
{
    m_payload.erase(key);
}

std::unordered_map<std::string, std::string> Message::getPayload() const
{
    return m_payload;
}

std::string Message::getPayloadField(const std::string &key) const
{
    auto it = m_payload.find(key);
    return (it != m_payload.end()) ? it->second : "";
}