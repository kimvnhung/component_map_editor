#ifndef COMPONENT_H
#define COMPONENT_H

#include <QString>
#include <QVariantMap>

#include <atomic>

using Tokens = QVariantMap;

enum GraphItemType
{
    Component_Type,
    Connection_Type
};

QString new_id(GraphItemType type);
QString token2string(const Tokens &tokens);

class Component
{
public:
    Component(const QString&id, const QString&title, const QString&type);
    virtual bool execute(Tokens &outputTokens, const Tokens&inputTokens = {}, const Tokens &componentSnapshot = {})
        = 0;
    QString getId() const;
    QString getType() const;
    QString getTitle() const;

    void addProperty(const QString &key, const QString &value);
    void removeProperty(const QString &key);
    QString getProperty(const QString &key) const;
    Tokens snapshot() const { return m_properties; }
private:
    QString m_id{};
    QString m_type{};
    QString m_title{};
    QVariantMap m_properties{};
};

class Connection
{
public:
    Connection(const QString &id, const QString &sourceComponentId, const QString &targetComponentId);
    QString getId() const;
    QString getSourceComponentId() const;
    QString getTargetComponentId() const;

    void setSourceComponentId(const QString &sourceComponentId);
    void setTargetComponentId(const QString &targetComponentId);
    void addProperty(const QString &key, const QString &value);
    void removeProperty(const QString &key);
    QString getProperty(const QString &key) const;
private:
    QString m_id{};
    QString m_sourceComponentId{};
    QString m_targetComponentId{};
    QVariantMap m_properties{};
};

class ClockComponent : public Component
{
public:
    ClockComponent(const QString &id);
    bool execute(Tokens &outputTokens, const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}) override;
};

class PrinterComponent : public Component
{
public:
    PrinterComponent(const QString &id);
    bool execute(Tokens &outputTokens, const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}) override;
};

class FruitProducerComponent: public Component
{
public:
    FruitProducerComponent(const QString &id, const QString title, int price,
                           double productionRate/*number per second*/, int buySizePerTime = 2);
    bool execute(Tokens &outputTokens, const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}) override;
private:
    int m_price;
    double m_productionRate;
    int m_buySizePerTime;
    int m_waitingBuyAmount;
    std::atomic_bool m_inProducing{false};
};

class StoreComponent: public Component
{
public:
    StoreComponent(const QString &id, const QString title, int washAmount = 5, int sellAmount = 10);
    bool execute(Tokens &outputTokens, const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}) override;
private:
    int m_washAmount;
    int m_sellAmount;
    QMap<QString, int> m_rawMaterials;
    QMap<QString, int> m_products;
};

class EmployeeComponent: public Component
{
public:
    EmployeeComponent(const QString &id, const QString title,
                      QMap<QString, double> performanceMetrics);
    bool execute(Tokens &outputTokens, const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}) override;
private:
    QMap<QString, double> m_performanceMetrics;
};

class SellerComponent: public Component
{
public:
    SellerComponent(const QString &id, const QString title,
                    QMap<QString, double> performanceMetrics, QMap<QString, double> pricingStrategy);
    bool execute(Tokens &outputTokens, const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}) override;
private:
    QMap<QString, double> m_performanceMetrics;
    QMap<QString, double> m_pricingStrategy;
};

class ManagerComponent: public Component
{
public:
    ManagerComponent(const QString &id, const QString title, int buyAmount = 10);
    bool execute(Tokens &outputTokens, const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}) override;
private:
    int m_capital;
    int m_buyAmount;
};

#endif // COMPONENT_H
