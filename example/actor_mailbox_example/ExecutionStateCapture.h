#ifndef EXECUTIONSTATECAPTURE_H
#define EXECUTIONSTATECAPTURE_H

#include <QVariantMap>
#include <chrono>

using Timestamp = std::chrono::steady_clock::time_point;
using Duration = std::chrono::steady_clock::duration;

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

struct ExecutionSnapshot
{
    QVariantMap componentSnapshot;  // Input state before execution
    QVariantMap inputTokens;        // Merged tokens
    Timestamp executedAt;
    ExecuteResult result;           // Output result
    QVariantMap outputTokens;       // Routed outputs
    Timestamp resultCommittedAt;
};



#endif // EXECUTIONSTATECAPTURE_H
