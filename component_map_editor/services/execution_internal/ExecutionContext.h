#ifndef EXECUTIONCONTEXT_H
#define EXECUTIONCONTEXT_H

#include <QString>
#include <QVariantMap>

struct ExecutionContext
{
    QString sourceComponentId;
    QString componentId;
    QVariantMap inputTokens;  // Merged inputs
    QVariantMap componentSnapshot;
};

struct ExecuteResult
{
    bool success;
    QVariantMap outputState;
    QString message;
};

#endif // EXECUTIONCONTEXT_H
