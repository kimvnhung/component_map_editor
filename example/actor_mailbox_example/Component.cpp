#include "Component.h"

#include <QThread>
#include <base_log.h>

QString new_id(GraphItemType type)
{
    static int componentCounter = 0;
    static int connectionCounter = 0;
    return QString((type == Component_Type ? "component" : "connection")) + "_" + std::to_string((
                type == Component_Type ? componentCounter++ :
                connectionCounter++));
}

QString token2string(const Tokens &tokens)
{
    QStringList tokenStrings;

    for (auto it = tokens.constBegin(); it != tokens.constEnd(); ++it)
    {
        tokenStrings.append(it.key() + ": " + it.value().toString());
    }

    return "{" + tokenStrings.join(", ") + "}";
}

Component::Component(const QString &id, const QString &title, const QString &type)
    : m_id(id)
    , m_type(type)
    , m_title(title)
{}

QString Component::getId() const { return m_id; }
QString Component::getType() const { return m_type; }
QString Component::getTitle() const { return m_title; }
void Component::addProperty(const QString &key, const QString &value) { m_properties[key] = value; }
void Component::removeProperty(const QString &key) { m_properties.remove(key); }
QString Component::getProperty(const QString &key) const
{
    auto it = m_properties.find(key);
    return (it != m_properties.end()) ? it.value().toString() : "";
}

Connection::Connection(const QString &id, const QString &sourceComponentId,
                       const QString &targetComponentId)
    : m_id(id), m_sourceComponentId(sourceComponentId), m_targetComponentId(targetComponentId) {}
QString Connection::getId() const { return m_id; }
QString Connection::getSourceComponentId() const { return m_sourceComponentId; }
QString Connection::getTargetComponentId() const { return m_targetComponentId; }
void Connection::setSourceComponentId(const QString &sourceComponentId) { m_sourceComponentId = sourceComponentId; }
void Connection::setTargetComponentId(const QString &targetComponentId) { m_targetComponentId = targetComponentId; }
void Connection::addProperty(const QString &key, const QString &value) { m_properties[key] = value; }
void Connection::removeProperty(const QString &key) { m_properties.remove(key); }
QString Connection::getProperty(const QString &key) const
{
    auto it = m_properties.find(key);
    return (it != m_properties.end()) ? it.value().toString() : "";
}

ClockComponent::ClockComponent(const QString &id)
    : Component(id, "clock", "Clock") {}

bool ClockComponent::execute(Tokens &outputTokens, const Tokens &inputTokens, const Tokens &componentSnapshot)
{
    int tick = 0;

    if (inputTokens.find("tick") != inputTokens.end())
    {
        tick = inputTokens.value("tick").toInt();
    }

    QThread::msleep(1000); // Simulate a clock tick every second
    outputTokens["tick"] = QString::number(tick + 1);
    return true;
}

PrinterComponent::PrinterComponent(const QString &id)
    : Component(id, "printer", "Printer") {}

bool PrinterComponent::execute(Tokens &outputTokens, const Tokens &inputTokens, const Tokens &componentSnapshot)
{
    if (inputTokens.find("tick") != inputTokens.end())
    {
        QString tickValue = inputTokens.value("tick").toString();
        outputTokens["printed"] = "Printed tick: " + tickValue;
        outputTokens["tick"] = tickValue;
        return true;
    }

    return false;
}

FruitProducerComponent::FruitProducerComponent(const QString &id, const QString title, int price,
        double productionRate, int buySizePerTime)
    : Component(id, "fruit_producer", title)
    , m_price(price)
    , m_productionRate(productionRate)
    , m_buySizePerTime(buySizePerTime)
    , m_waitingBuyAmount(0)
{

}

bool FruitProducerComponent::execute(Tokens & outputTokens, const Tokens & inputTokens,
                                     const Tokens & componentSnapshot)
{
    LOGDF("FruitProducerComponent {} received input tokens: {}", getId().toStdString(),
          token2string(inputTokens).toStdString());
    int buy = 0;

    if (inputTokens.find("buy") != inputTokens.end())
    {
        if (inputTokens.find("producer_id") != inputTokens.end())
        {
            QString producerId = inputTokens.value("producer_id").toString();

            if (producerId == getId())
            {
                buy = inputTokens.value("buy").toInt();
                LOGDF("FruitProducerComponent {} received buy request: {}", getId().toStdString(), buy);

                if (m_inProducing)
                {
                    m_waitingBuyAmount += buy;
                    LOGDF("FruitProducerComponent {} is currently producing. Accumulated waiting buy amount: {}", getId().toStdString(),
                          m_waitingBuyAmount);
                }
                else
                {
                    m_inProducing.store(true);
                    int totalBuy = buy + m_waitingBuyAmount;
                    int produceAmount = std::min(totalBuy, m_buySizePerTime);
                    int remainingBuy = totalBuy - produceAmount;

                    // Simulate production time based on production rate
                    int productionTimeMs = static_cast<int>((produceAmount / m_productionRate) * 1000);
                    LOGDF("FruitProducerComponent {} producing: {} of fruit type: {}. Estimated production time: {} ms",
                          getId().toStdString(),
                          produceAmount, getTitle().toStdString(), productionTimeMs);
                    QThread::msleep(productionTimeMs);

                    outputTokens["produced"] = QString::number(produceAmount);
                    outputTokens["fruit_type"] = getTitle();
                    LOGDF("FruitProducerComponent {} produced: {} of fruit type: {}. Remaining buy amount: {}", getId().toStdString(),
                          produceAmount, getTitle().toStdString(), remainingBuy);

                    m_waitingBuyAmount = remainingBuy;
                    m_inProducing.store(false);
                }
            }
        }
    }
    else
    {
        outputTokens["request_buy"] = getId();
    }

    return true;
}

StoreComponent::StoreComponent(const QString & id, const QString title, int washAmount, int sellAmount)
    : Component(id, "store", title)
    , m_washAmount(washAmount)
    , m_sellAmount(sellAmount)
{
}

bool StoreComponent::execute(Tokens & outputTokens, const Tokens & inputTokens, const Tokens & componentSnapshot)
{
    // Check if request from EmployeeComponent
    if (inputTokens.find("washed") != inputTokens.end())
    {
        int washed = inputTokens.value("washed").toInt();
        QString fruitType = inputTokens.value("fruit_type").toString();
        int lastStock = m_products[fruitType];

        if (lastStock <= 0)
        {
            lastStock = 0;
        }

        m_products[fruitType] = lastStock + washed;
        LOGDF("StoreComponent received washed: {} for fruit type: {}. Updated stock: {}", washed, fruitType.toStdString(),
              m_products[fruitType]);
    }
    else if (inputTokens.find("produced") != inputTokens.end())
    {
        int produced = inputTokens.value("produced").toInt();
        QString fruitType = inputTokens.value("fruit_type").toString();
        m_rawMaterials[fruitType] += produced;
    }
    else if (inputTokens.find("request_wash") != inputTokens.end())
    {
        QString employeeId = inputTokens.value("request_wash").toString();
        int total = 0;
        bool isSent = false;

        for (const auto &pair : m_rawMaterials)
        {
            const QString &fruitType = pair.first;
            int rawAmount = pair.second;
            total += rawAmount;

            if (rawAmount >= m_washAmount)
            {
                m_rawMaterials[fruitType] -= m_washAmount;
                outputTokens["wash"] = m_washAmount;
                outputTokens["fruit_type"] = fruitType;
                outputTokens["employee_id"] = employeeId;
                LOGDF("StoreComponent sending washed: {} for fruit type: {} to employee: {}", m_washAmount,
                      fruitType.toStdString(), employeeId.toStdString());
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
                    const QString &fruitType = pair.first;
                    int rawAmount = pair.second;

                    if (rawAmount > 0)
                    {
                        m_rawMaterials[fruitType] -= rawAmount;
                        outputTokens["wash"] = rawAmount;
                        outputTokens["fruit_type"] = fruitType;
                        outputTokens["employee_id"] = employeeId;
                        LOGDF("StoreComponent sending washed: {} for fruit type: {} to employee: {}", rawAmount,
                              fruitType.toStdString(), employeeId.toStdString());
                        break; // Only wash one type of fruit at a time
                    }
                }
            }
        }
    }
    else if (inputTokens.find("request_sell") != inputTokens.end())
    {
        QString employeeId = inputTokens.value("request_sell").toString();
        int total = 0;
        bool isSent = false;

        for (const auto &pair : m_products)
        {
            const QString &fruitType = pair.first;
            int productAmount = pair.second;
            total += productAmount;

            if (productAmount >= m_sellAmount)
            {
                m_products[fruitType] -= m_sellAmount;
                outputTokens["sell"] = m_sellAmount;
                outputTokens["fruit_type"] = fruitType;
                outputTokens["employee_id"] = employeeId;
                LOGDF("StoreComponent sending sold: {} for fruit type: {} to employee: {}", m_sellAmount,
                      fruitType.toStdString(), employeeId.toStdString());
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
                    const QString &fruitType = pair.first;
                    int productAmount = pair.second;

                    if (productAmount > 0)
                    {
                        m_products[fruitType] -= productAmount;
                        outputTokens["sell"] = productAmount;
                        outputTokens["fruit_type"] = fruitType;
                        outputTokens["employee_id"] = employeeId;
                        LOGDF("StoreComponent sending sold: {} for fruit type: {} to employee: {}", productAmount,
                              fruitType.toStdString(), employeeId.toStdString());
                        break; // Only sell one type of fruit at a time
                    }
                }
            }
        }
    }

    return true;
}

EmployeeComponent::EmployeeComponent(const QString & id, const QString title,
                                     std::unordered_map<QString, double> performanceMetrics)
    : Component(id, "employee", title)
    , m_performanceMetrics(performanceMetrics)
{
}

bool EmployeeComponent::execute(Tokens & outputTokens, const Tokens & inputTokens, const Tokens & componentSnapshot)
{
    if (inputTokens.find("wash") != inputTokens.end())
    {
        int washed = inputTokens.value("wash").toInt();
        QString fruitType = inputTokens.value("fruit_type").toString();
        QString employeeId = inputTokens.value("employee_id").toString();

        if (employeeId == getId())
        {
            double performance = m_performanceMetrics[fruitType];
            int processed = static_cast<int>(washed / performance * 1000);
            QThread::msleep(processed);
            outputTokens["washed"] = washed;
            outputTokens["fruit_type"] = fruitType;
            LOGDF("EmployeeComponent {} processed washed: {} for fruit type: {} with performance: {}", getId().toStdString(),
                  washed, fruitType.toStdString(), performance);
        }
    }
    else
    {
        // Request wash or sell from StoreComponent
        outputTokens["request_wash"] = getId();
    }

    return true;
}

SellerComponent::SellerComponent(const QString & id, const QString title,
                                 std::unordered_map<QString, double> performanceMetrics,
                                 std::unordered_map<QString, double> pricingStrategy)
    : Component(id, "seller", title)
    , m_performanceMetrics(performanceMetrics)
    , m_pricingStrategy(pricingStrategy)
{
}

bool SellerComponent::execute(Tokens & outputTokens, const Tokens & inputTokens, const Tokens & componentSnapshot)
{
    if (inputTokens.find("sell") != inputTokens.end())
    {
        int sold = inputTokens.value("sell").toInt();
        QString fruitType = inputTokens.value("fruit_type").toString();
        QString employeeId = inputTokens.value("employee_id").toString();

        if (employeeId == getId())
        {
            double performance = m_performanceMetrics[fruitType];
            double price = m_pricingStrategy[fruitType];
            int processed = static_cast<int>(sold / performance * 1000);
            QThread::msleep(processed);
            outputTokens["revenue"] = sold * price;
            LOGDF("SellerComponent {} processed sold: {} for fruit type: {} with performance: {} and revenue: {}",
                  getId().toStdString(),
                  sold, fruitType.toStdString(), performance, sold * price);
        }
    }
    else
    {
        // Request sell from StoreComponent
        outputTokens["request_sell"] = getId();
    }

    return true;
}

ManagerComponent::ManagerComponent(const QString & id, const QString title, int buyAmount)
    : Component(id, "manager", title)
    , m_buyAmount(buyAmount)
{
}

bool ManagerComponent::execute(Tokens & outputTokens, const Tokens & inputTokens, const Tokens & componentSnapshot)
{
    if (inputTokens.find("revenue") != inputTokens.end())
    {
        double revenue = inputTokens.value("revenue").toDouble();
        m_capital += revenue;
        LOGDF("ManagerComponent {} received revenue: {}. Updated capital: {}", getId().toStdString(), revenue, m_capital);
    }
    else if (inputTokens.find("request_buy") != inputTokens.end())
    {
        QString producerId = inputTokens.value("request_buy").toString();

        if (m_capital >= m_buyAmount)
        {
            m_capital -= m_buyAmount;
            outputTokens["buy"] = m_buyAmount;
            outputTokens["producer_id"] = producerId;
            LOGDF("ManagerComponent {} requested buy: {} from producer: {}. Updated capital: {}", getId().toStdString(),
                  m_buyAmount, producerId.toStdString(), m_capital);
        }
        else
        {
            LOGDF("ManagerComponent {} has insufficient capital: {} to buy from producer: {}", getId().toStdString(), m_capital,
                  producerId.toStdString());
            return false;
        }
    }
    else if (inputTokens.find("init_budget") != inputTokens.end())
    {
        m_capital = inputTokens.value("init_budget").toInt();
        LOGDF("ManagerComponent {} initialized budget: {}", getId().toStdString(), m_capital);
    }

    return true;
}