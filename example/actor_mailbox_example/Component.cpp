#include "Component.h"

#include <QThread>
#include <base_log.h>

std::string new_id(GraphItemType type)
{
    static int componentCounter = 0;
    static int connectionCounter = 0;
    return std::string((type == Component_Type ? "component" : "connection")) + "_" + std::to_string((
                type == Component_Type ? componentCounter++ :
                connectionCounter++));
}

Component::Component(const std::string &id, const std::string &title, const std::string &type)
    : m_id(id)
    , m_type(type)
    , m_title(title)
{}

std::string Component::getId() const { return m_id; }
std::string Component::getType() const { return m_type; }
std::string Component::getTitle() const { return m_title; }
void Component::addProperty(const std::string &key, const std::string &value) { m_properties[key] = value; }
void Component::removeProperty(const std::string &key) { m_properties.erase(key); }
std::string Component::getProperty(const std::string &key) const
{
    auto it = m_properties.find(key);
    return (it != m_properties.end()) ? it->second : "";
}

Connection::Connection(const std::string &id, const std::string &sourceComponentId,
                       const std::string &targetComponentId)
    : m_id(id), m_sourceComponentId(sourceComponentId), m_targetComponentId(targetComponentId) {}
std::string Connection::getId() const { return m_id; }
std::string Connection::getSourceComponentId() const { return m_sourceComponentId; }
std::string Connection::getTargetComponentId() const { return m_targetComponentId; }
void Connection::setSourceComponentId(const std::string &sourceComponentId) { m_sourceComponentId = sourceComponentId; }
void Connection::setTargetComponentId(const std::string &targetComponentId) { m_targetComponentId = targetComponentId; }
void Connection::addProperty(const std::string &key, const std::string &value) { m_properties[key] = value; }
void Connection::removeProperty(const std::string &key) { m_properties.erase(key); }
std::string Connection::getProperty(const std::string &key) const
{
    auto it = m_properties.find(key);
    return (it != m_properties.end()) ? it->second : "";
}

ClockComponent::ClockComponent(const std::string &id)
    : Component(id, "clock", "Clock") {}

bool ClockComponent::execute(Tokens &outputTokens, const Tokens &inputTokens, const Tokens &componentSnapshot)
{
    int tick = 0;

    if (inputTokens.find("tick") != inputTokens.end())
    {
        std::string tickValue = inputTokens.at("tick");
        tick = std::stoi(tickValue);
    }

    QThread::msleep(1000); // Simulate a clock tick every second
    outputTokens["tick"] = std::to_string(tick + 1);
    return true;
}

PrinterComponent::PrinterComponent(const std::string &id)
    : Component(id, "printer", "Printer") {}

bool PrinterComponent::execute(Tokens &outputTokens, const Tokens &inputTokens, const Tokens &componentSnapshot)
{
    if (inputTokens.find("tick") != inputTokens.end())
    {
        std::string tickValue = inputTokens.at("tick");
        LOGDF("PrinterComponent received tick: {}", tickValue);
        outputTokens["printed"] = "Printed tick: " + tickValue;
        outputTokens["tick"] = tickValue;
        return true;
    }

    return false;
}

FruitProducerComponent::FruitProducerComponent(const std::string &id, const std::string title, int price,
        double productionRate)
    : Component(id, "fruit_producer", title)
    , m_price(price)
    , m_productionRate(productionRate)
{

}

bool FruitProducerComponent::execute(Tokens &outputTokens, const Tokens &inputTokens, const Tokens &componentSnapshot)
{
    int buy = 0;

    if (inputTokens.find("buy") != inputTokens.end())
    {
        if (inputTokens.find("producer_id") != inputTokens.end())
        {
            std::string producerId = inputTokens.at("producer_id");

            if (producerId == getId())
            {
                buy = std::stoi(inputTokens.at("buy"));
                LOGDF("FruitProducerComponent {} received buy request: {}", getId(), buy);
                int produced = static_cast<int>(buy / m_price);
                outputTokens["produced"] = std::to_string(produced);
                outputTokens["fruit_type"] = getTitle();
                QThread::msleep(static_cast<int>(produced / m_productionRate * 1000));
                LOGDF("FruitProducerComponent {} produced: {} for fruit type: {}", getId(), produced, getTitle());
            }
        }
    }
    else
    {
        // Trigger to buy
        outputTokens["request_buy"] = getId();
    }

    return true;
}

StoreComponent::StoreComponent(const std::string &id, const std::string title, int washAmount, int sellAmount)
    : Component(id, "store", title)
    , m_washAmount(washAmount)
    , m_sellAmount(sellAmount)
{
}

bool StoreComponent::execute(Tokens &outputTokens, const Tokens &inputTokens, const Tokens &componentSnapshot)
{
    // Check if request from EmployeeComponent
    if (inputTokens.find("washed") != inputTokens.end())
    {
        int washed = std::stoi(inputTokens.at("washed"));
        std::string fruitType = inputTokens.at("fruit_type");
        int lastStock = m_products[fruitType];

        if (lastStock <= 0)
        {
            lastStock = 0;
        }

        m_products[fruitType] = lastStock + washed;
        LOGDF("StoreComponent received washed: {} for fruit type: {}. Updated stock: {}", washed, fruitType,
              m_products[fruitType]);
    }
    else if (inputTokens.find("produced") != inputTokens.end())
    {
        int produced = std::stoi(inputTokens.at("produced"));
        std::string fruitType = inputTokens.at("fruit_type");
        m_rawMaterials[fruitType] += produced;
    }
    else if (inputTokens.find("request_wash") != inputTokens.end())
    {
        std::string employeeId = inputTokens.at("request_wash");
        int total = 0;
        bool isSent = false;

        for (const auto &pair : m_rawMaterials)
        {
            const std::string &fruitType = pair.first;
            int rawAmount = pair.second;
            total += rawAmount;

            if (rawAmount >= m_washAmount)
            {
                m_rawMaterials[fruitType] -= m_washAmount;
                outputTokens["wash"] = std::to_string(m_washAmount);
                outputTokens["fruit_type"] = fruitType;
                outputTokens["employee_id"] = employeeId;
                LOGDF("StoreComponent sending washed: {} for fruit type: {} to employee: {}", m_washAmount,
                      fruitType, employeeId);
                isSent = true;
                break; // Only wash one type of fruit at a time
            }
        }

        if (!isSent)
        {
            if (total > 0)
            {
                for (const auto &pair : m_rawMaterials)
                {
                    const std::string &fruitType = pair.first;
                    int rawAmount = pair.second;

                    if (rawAmount > 0)
                    {
                        m_rawMaterials[fruitType] -= rawAmount;
                        outputTokens["wash"] = std::to_string(rawAmount);
                        outputTokens["fruit_type"] = fruitType;
                        outputTokens["employee_id"] = employeeId;
                        LOGDF("StoreComponent sending washed: {} for fruit type: {} to employee: {}", rawAmount,
                              fruitType, employeeId);
                        break; // Only wash one type of fruit at a time
                    }
                }
            }
        }
    }
    else if (inputTokens.find("request_sell") != inputTokens.end())
    {
        std::string employeeId = inputTokens.at("request_sell");
        int total = 0;
        bool isSent = false;

        for (const auto &pair : m_products)
        {
            const std::string &fruitType = pair.first;
            int productAmount = pair.second;
            total += productAmount;

            if (productAmount >= m_sellAmount)
            {
                m_products[fruitType] -= m_sellAmount;
                outputTokens["sell"] = std::to_string(m_sellAmount);
                outputTokens["fruit_type"] = fruitType;
                outputTokens["employee_id"] = employeeId;
                LOGDF("StoreComponent sending sold: {} for fruit type: {} to employee: {}", m_sellAmount,
                      fruitType, employeeId);
                isSent = true;
                break; // Only sell one type of fruit at a time
            }
        }

        if (!isSent)
        {
            if (total > 0)
            {
                for (const auto &pair : m_products)
                {
                    const std::string &fruitType = pair.first;
                    int productAmount = pair.second;

                    if (productAmount > 0)
                    {
                        m_products[fruitType] -= productAmount;
                        outputTokens["sell"] = std::to_string(productAmount);
                        outputTokens["fruit_type"] = fruitType;
                        outputTokens["employee_id"] = employeeId;
                        LOGDF("StoreComponent sending sold: {} for fruit type: {} to employee: {}", productAmount,
                              fruitType, employeeId);
                        break; // Only sell one type of fruit at a time
                    }
                }
            }
        }
    }

    return true;
}

EmployeeComponent::EmployeeComponent(const std::string &id, const std::string title,
                                     std::unordered_map<std::string, double> performanceMetrics)
    : Component(id, "employee", title)
    , m_performanceMetrics(performanceMetrics)
{
}

bool EmployeeComponent::execute(Tokens &outputTokens, const Tokens &inputTokens, const Tokens &componentSnapshot)
{
    if (inputTokens.find("wash") != inputTokens.end())
    {
        int washed = std::stoi(inputTokens.at("wash"));
        std::string fruitType = inputTokens.at("fruit_type");
        std::string employeeId = inputTokens.at("employee_id");

        if (employeeId == getId())
        {
            double performance = m_performanceMetrics[fruitType];
            int processed = static_cast<int>(washed / performance * 1000);
            QThread::msleep(processed);
            outputTokens["washed"] = std::to_string(washed);
            outputTokens["fruit_type"] = fruitType;
            LOGDF("EmployeeComponent {} processed washed: {} for fruit type: {} with performance: {}", getId(),
                  washed, fruitType, performance);
        }
    }
    else
    {
        // Request wash or sell from StoreComponent
        outputTokens["request_wash"] = getId();
    }

    return true;
}

SellerComponent::SellerComponent(const std::string &id, const std::string title,
                                 std::unordered_map<std::string, double> performanceMetrics,
                                 std::unordered_map<std::string, double> pricingStrategy)
    : Component(id, "seller", title)
    , m_performanceMetrics(performanceMetrics)
    , m_pricingStrategy(pricingStrategy)
{
}

bool SellerComponent::execute(Tokens &outputTokens, const Tokens &inputTokens, const Tokens &componentSnapshot)
{
    if (inputTokens.find("sell") != inputTokens.end())
    {
        int sold = std::stoi(inputTokens.at("sell"));
        std::string fruitType = inputTokens.at("fruit_type");
        std::string employeeId = inputTokens.at("employee_id");

        if (employeeId == getId())
        {
            double performance = m_performanceMetrics[fruitType];
            double price = m_pricingStrategy[fruitType];
            int processed = static_cast<int>(sold / performance * 1000);
            QThread::msleep(processed);
            outputTokens["revenue"] = std::to_string(sold * price);
            LOGDF("SellerComponent {} processed sold: {} for fruit type: {} with performance: {} and revenue: {}", getId(),
                  sold, fruitType, performance, sold * price);
        }
    }
    else
    {
        // Request sell from StoreComponent
        outputTokens["request_sell"] = getId();
    }

    return true;
}

ManagerComponent::ManagerComponent(const std::string &id, const std::string title, int buyAmount)
    : Component(id, "manager", title)
    , m_buyAmount(buyAmount)
{
}

bool ManagerComponent::execute(Tokens &outputTokens, const Tokens &inputTokens, const Tokens &componentSnapshot)
{
    if (inputTokens.find("revenue") != inputTokens.end())
    {
        double revenue = std::stod(inputTokens.at("revenue"));
        m_capital += revenue;
        LOGDF("ManagerComponent {} received revenue: {}. Updated capital: {}", getId(), revenue, m_capital);
    }
    else if (inputTokens.find("request_buy") != inputTokens.end())
    {
        std::string producerId = inputTokens.at("request_buy");

        if (m_capital >= m_buyAmount)
        {
            m_capital -= m_buyAmount;
            outputTokens["buy"] = std::to_string(m_buyAmount);
            outputTokens["producer_id"] = producerId;
            LOGDF("ManagerComponent {} requested buy: {} from producer: {}. Updated capital: {}", getId(),
                  m_buyAmount, producerId, m_capital);
        }
        else
        {
            LOGDF("ManagerComponent {} has insufficient capital: {} to buy from producer: {}", getId(),
                  m_capital, producerId);
        }
    }
    else if (inputTokens.find("init_budget") != inputTokens.end())
    {
        m_capital = std::stoi(inputTokens.at("init_budget"));
        LOGDF("ManagerComponent {} initialized budget: {}", getId(), m_capital);
    }

    return true;
}