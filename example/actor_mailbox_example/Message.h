#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <unordered_map>

class Message
{
public:
    Message(const std::unordered_map<std::string, std::string> &tokens = {},
            const std::unordered_map<std::string, std::string> &componentSnapshot = {});

    void setActorId(const std::string &actorId);
    std::string getActorId() const;

    void setTokens(const std::unordered_map<std::string, std::string> &tokens);
    void setComponentSnapshot(const std::unordered_map<std::string, std::string> &snapshot);
    std::unordered_map<std::string, std::string> getTokens() const;
    std::unordered_map<std::string, std::string> getComponentSnapshot() const;

private:
    std::string m_actorId;
    std::unordered_map<std::string, std::string> m_tokens;
    std::unordered_map<std::string, std::string> m_componentSnapshot;
};

#endif // MESSAGE_H
