#ifndef COMPONENT_H
#define COMPONENT_H

#include <string>
#include <unordered_map>

using Tokens = std::unordered_map<std::string, std::string>;

enum GraphItemType
{
    Component_Type,
    Connection_Type
};

std::string new_id(GraphItemType type);

class Component
{
public:
    Component(const std::string &id, const std::string &title, const std::string &type);
    virtual bool execute(Tokens &outputTokens, const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {})
        = 0;
    std::string getId() const;
    std::string getType() const;
    std::string getTitle() const;

    void addProperty(const std::string &key, const std::string &value);
    void removeProperty(const std::string &key);
    std::string getProperty(const std::string &key) const;
    Tokens snapshot() const { return m_properties; }
private:
    std::string m_id{};
    std::string m_type{};
    std::string m_title{};
    std::unordered_map<std::string, std::string> m_properties{};
};

class Connection
{
public:
    Connection(const std::string &id, const std::string &sourceComponentId, const std::string &targetComponentId);
    std::string getId() const;
    std::string getSourceComponentId() const;
    std::string getTargetComponentId() const;

    void setSourceComponentId(const std::string &sourceComponentId);
    void setTargetComponentId(const std::string &targetComponentId);
    void addProperty(const std::string &key, const std::string &value);
    void removeProperty(const std::string &key);
    std::string getProperty(const std::string &key) const;
private:
    std::string m_id{};
    std::string m_sourceComponentId{};
    std::string m_targetComponentId{};
    std::unordered_map<std::string, std::string> m_properties{};
};

class ClockComponent : public Component
{
public:
    ClockComponent(const std::string &id);
    bool execute(Tokens &outputTokens, const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}) override;
};

class PrinterComponent : public Component
{
public:
    PrinterComponent(const std::string &id);
    bool execute(Tokens &outputTokens, const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}) override;
};

class FruitProducerComponent: public Component
{
public:
    FruitProducerComponent(const std::string &id, const std::string title, int price,
                           double productionRate/*number per second*/);
    bool execute(Tokens &outputTokens, const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}) override;
private:
    int m_price;
    double m_productionRate;
};

class StoreComponent: public Component
{
public:
    StoreComponent(const std::string &id, const std::string title, int washAmount = 5, int sellAmount = 10);
    bool execute(Tokens &outputTokens, const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}) override;
private:
    int m_washAmount;
    int m_sellAmount;
    std::unordered_map<std::string, int> m_rawMaterials;
    std::unordered_map<std::string, int> m_products;
};

class EmployeeComponent: public Component
{
public:
    EmployeeComponent(const std::string &id, const std::string title,
                      std::unordered_map<std::string, double> performanceMetrics);
    bool execute(Tokens &outputTokens, const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}) override;
private:
    std::unordered_map<std::string, double> m_performanceMetrics;
};

class SellerComponent: public Component
{
public:
    SellerComponent(const std::string &id, const std::string title,
                    std::unordered_map<std::string, double> performanceMetrics, std::unordered_map<std::string, double> pricingStrategy);
    bool execute(Tokens &outputTokens, const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}) override;
private:
    std::unordered_map<std::string, double> m_performanceMetrics;
    std::unordered_map<std::string, double> m_pricingStrategy;
};

class ManagerComponent: public Component
{
public:
    ManagerComponent(const std::string &id, const std::string title, int buyAmount = 10);
    bool execute(Tokens &outputTokens, const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}) override;
private:
    int m_capital;
    int m_buyAmount;
};

#endif // COMPONENT_H
