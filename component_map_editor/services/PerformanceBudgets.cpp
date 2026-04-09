#include "PerformanceBudgets.h"

PerformanceBudgets::PerformanceBudgets(QObject *parent)
    : QObject(parent)
{}

double PerformanceBudgets::frameP95BudgetMs() const
{
    return kFrameP95BudgetMs;
}

double PerformanceBudgets::commandLatencyP95BudgetMs() const
{
    return kCommandLatencyP95BudgetMs;
}

qint64 PerformanceBudgets::memoryGrowthBudgetBytes() const
{
    return kMemoryGrowthBudgetBytes;
}

bool PerformanceBudgets::passesFrameBudget(double observedFrameP95Ms) const
{
    return observedFrameP95Ms > 0.0 && observedFrameP95Ms <= kFrameP95BudgetMs;
}

bool PerformanceBudgets::passesCommandLatencyBudget(double observedCommandP95Ms) const
{
    return observedCommandP95Ms > 0.0 && observedCommandP95Ms <= kCommandLatencyP95BudgetMs;
}

bool PerformanceBudgets::passesMemoryGrowthBudget(qint64 observedMemoryGrowthBytes) const
{
    return observedMemoryGrowthBytes >= 0 && observedMemoryGrowthBytes <= kMemoryGrowthBudgetBytes;
}

QVariantMap PerformanceBudgets::evaluateSession(double observedFrameP95Ms,
                                                double observedCommandP95Ms,
                                                qint64 observedMemoryGrowthBytes) const
{
    const bool framePass = passesFrameBudget(observedFrameP95Ms);
    const bool commandPass = passesCommandLatencyBudget(observedCommandP95Ms);
    const bool memoryPass = passesMemoryGrowthBudget(observedMemoryGrowthBytes);

    QVariantMap result;
    result.insert(QStringLiteral("frameP95BudgetMs"), kFrameP95BudgetMs);
    result.insert(QStringLiteral("commandLatencyP95BudgetMs"), kCommandLatencyP95BudgetMs);
    result.insert(QStringLiteral("memoryGrowthBudgetBytes"), kMemoryGrowthBudgetBytes);

    result.insert(QStringLiteral("observedFrameP95Ms"), observedFrameP95Ms);
    result.insert(QStringLiteral("observedCommandLatencyP95Ms"), observedCommandP95Ms);
    result.insert(QStringLiteral("observedMemoryGrowthBytes"), observedMemoryGrowthBytes);

    result.insert(QStringLiteral("framePass"), framePass);
    result.insert(QStringLiteral("commandPass"), commandPass);
    result.insert(QStringLiteral("memoryPass"), memoryPass);
    result.insert(QStringLiteral("allPass"), framePass && commandPass && memoryPass);

    return result;
}
