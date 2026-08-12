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

class ExecutionSnapshot
{
    QVariantMap componentSnapshot;  // Input state before execution
    QVariantMap inputTokens;        // Merged tokens
    Timestamp executedAt;
    ExecuteResult result;           // Output result
    QVariantMap outputTokens;       // Routed outputs
    Timestamp resultCommittedAt;
};

class ExecutionStateCapture
{
    struct Snapshot
    {
        QString componentId;
        QVariantMap componentState;    // Before execution
        QVariantMap inputTokens;       // Merged inputs
        ExecuteResult result;          // After execution
        QVariantMap outputTokens;      // Produced outputs
        Timestamp executeAt;
        Duration duration;
    };

    void captureSnapshot(const Snapshot& snap);
    QVector<Snapshot> recentSnapshots(int count = 100);
    QVariantMap currentGraphState();
};

#endif // EXECUTIONSTATECAPTURE_H
