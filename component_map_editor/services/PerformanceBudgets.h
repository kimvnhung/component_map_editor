#ifndef PERFORMANCEBUDGETS_H
#define PERFORMANCEBUDGETS_H

#include <QObject>
#include <QQmlEngine>
#include <QVariantMap>

// Central hard-budget policy for Phase 9.
// These values are used by tests and UI/benchmark tooling to evaluate whether
// performance targets are met.
class PerformanceBudgets : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(double frameP95BudgetMs READ frameP95BudgetMs CONSTANT FINAL)
    Q_PROPERTY(double commandLatencyP95BudgetMs READ commandLatencyP95BudgetMs CONSTANT FINAL)
    Q_PROPERTY(qint64 memoryGrowthBudgetBytes READ memoryGrowthBudgetBytes CONSTANT FINAL)

public:
    explicit PerformanceBudgets(QObject *parent = nullptr);

    static constexpr double kFrameP95BudgetMs = 16.0;
    static constexpr double kCommandLatencyP95BudgetMs = 50.0;
    static constexpr qint64 kMemoryGrowthBudgetBytes = 64ll * 1024ll * 1024ll;

    double frameP95BudgetMs() const;
    double commandLatencyP95BudgetMs() const;
    qint64 memoryGrowthBudgetBytes() const;

    Q_INVOKABLE bool passesFrameBudget(double observedFrameP95Ms) const;
    Q_INVOKABLE bool passesCommandLatencyBudget(double observedCommandP95Ms) const;
    Q_INVOKABLE bool passesMemoryGrowthBudget(qint64 observedMemoryGrowthBytes) const;

    Q_INVOKABLE QVariantMap evaluateSession(double observedFrameP95Ms,
                                            double observedCommandP95Ms,
                                            qint64 observedMemoryGrowthBytes) const;
};

#endif // PERFORMANCEBUDGETS_H
