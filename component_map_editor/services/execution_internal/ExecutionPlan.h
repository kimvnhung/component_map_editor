#ifndef EXECUTIONPLAN_H
#define EXECUTIONPLAN_H

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

struct ExecutionPlan
{
    QString resolvedStartComponentId;
    QSet<QString> reachableComponentIds;
    QSet<QString> ignoredComponentIds;
    QSet<QString> activeConnectionIds;
    QHash<QString, int> pendingInDegree;
    QStringList diagnostics;
};

#endif // EXECUTIONPLAN_H
