#include "Message.h"

Message::Message(const std::unordered_map<std::string, std::string> &tokens,
                 const std::unordered_map<std::string, std::string> &componentSnapshot)
    : m_tokens(tokens), m_componentSnapshot(componentSnapshot) {}

void Message::setActorId(const std::string &actorId)
{
    m_actorId = actorId;
}

std::string Message::getActorId() const
{
    return m_actorId;
}

void Message::setTokens(const std::unordered_map<std::string, std::string> &tokens)
{
    m_tokens = tokens;
}

void Message::setComponentSnapshot(const std::unordered_map<std::string, std::string> &snapshot)
{
    m_componentSnapshot = snapshot;
}

std::unordered_map<std::string, std::string> Message::getTokens() const
{
    return m_tokens;
}

std::unordered_map<std::string, std::string> Message::getComponentSnapshot() const
{
    return m_componentSnapshot;
}
