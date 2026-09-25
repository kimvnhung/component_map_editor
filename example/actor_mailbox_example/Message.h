#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <unordered_map>

class Message
{
public:
    Message(const std::unordered_map<std::string, std::string> &payload = {});

    void setPayload(const std::unordered_map<std::string, std::string> &payload);
    void addPayloadField(const std::string &key, const std::string &value);
    void removePayloadField(const std::string &key);
    std::unordered_map<std::string, std::string> getPayload() const;
    std::string getPayloadField(const std::string &key) const;
private:
    std::unordered_map<std::string, std::string> m_payload{};
};

#endif // MESSAGE_H
