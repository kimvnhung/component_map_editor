#ifndef RULERUNTIMEENGINE_H
#define RULERUNTIMEENGINE_H

#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "RuleDslTypes.h"

class RuleRuntimeEngine
{
public:
    RuleRuntimeEngine() = default;

    void setDescriptor(const CompiledRuleDescriptor *descriptor);

    bool canConnect(const QString &sourceTypeId,
                    const QString &targetTypeId,
                    QString *reason) const;

    QVariantMap normalizePropertiesForTarget(const QString &targetId,
                                             const QVariantMap &rawProperties) const;

    QVariantList validateGraph(const QVariantMap &graphSnapshot) const;

private:
    const CompiledRuleDescriptor *m_descriptor = nullptr;
};

#endif // RULERUNTIMEENGINE_H
