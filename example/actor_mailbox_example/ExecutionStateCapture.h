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

class ExecuteResult
{
    Q_GADGET
    Q_PROPERTY(bool success MEMBER success)
    Q_PROPERTY(QVariantMap outputState MEMBER outputState)
    Q_PROPERTY(QString message MEMBER message)
public:
    bool success;
    QVariantMap outputState;
    QString message;

    bool operator==(const ExecuteResult& other) const
    {
        return success == other.success &&
               outputState == other.outputState &&
               message == other.message;
    }
};
Q_DECLARE_METATYPE(ExecuteResult)

class  ExecutionSnapshot
{
    Q_GADGET
    Q_PROPERTY(QString componentId MEMBER componentId)
    Q_PROPERTY(QVariantMap componentSnapshot MEMBER componentSnapshot)
    Q_PROPERTY(QVariantMap inputTokens MEMBER inputTokens)
    Q_PROPERTY(ExecuteResult result MEMBER result)
    Q_PROPERTY(QVariantMap outputTokens MEMBER outputTokens)

public:
    QString componentId;
    QVariantMap componentSnapshot;  // Input state before execution
    QVariantMap inputTokens;        // Merged tokens
    Timestamp executedAt;
    ExecuteResult result;           // Output result
    QVariantMap outputTokens;       // Routed outputs
    Timestamp resultCommittedAt;

};

Q_DECLARE_METATYPE(ExecutionSnapshot)


#endif // EXECUTIONSTATECAPTURE_H
