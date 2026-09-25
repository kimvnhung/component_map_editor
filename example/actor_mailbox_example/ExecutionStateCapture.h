#ifndef EXECUTIONSTATECAPTURE_H
#define EXECUTIONSTATECAPTURE_H

#include <QVariantMap>
#include <QDateTime>
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
    Q_PROPERTY(QString executedAtStr READ executedAtStr)
    Q_PROPERTY(QString resultCommittedAtStr READ resultCommittedAtStr)

public:
    QString componentId;
    QVariantMap componentSnapshot;  // Input state before execution
    QVariantMap inputTokens;        // Merged tokens
    Timestamp executedAt;
    ExecuteResult result;           // Output result
    QVariantMap outputTokens;       // Routed outputs
    Timestamp resultCommittedAt;

    QString executedAtStr() const
    {
        // Convert to yyyy-MM-dd HH:mm:ss.mmm format
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(executedAt.time_since_epoch()).count();
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(ms);
        return dt.toString("yyyy-MM-dd HH:mm:ss.zzz");
    }

    QString resultCommittedAtStr() const
    {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(resultCommittedAt.time_since_epoch()).count();
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(ms);
        return dt.toString("yyyy-MM-dd HH:mm:ss.zzz");
    }

};

Q_DECLARE_METATYPE(ExecutionSnapshot)


#endif // EXECUTIONSTATECAPTURE_H
