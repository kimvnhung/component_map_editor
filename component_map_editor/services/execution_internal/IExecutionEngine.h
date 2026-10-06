#ifndef IEXECUTIONENGINE_H
#define IEXECUTIONENGINE_H

#include "ExecutionContext.h"
#include "extensions/contracts/IExecutionSemanticsProvider.h"
#include "models/GraphModel.h"

#include <graph.pb.h>

class IExecutionEngine
{
public:
    enum class StepOutcome
    {
        NoWork,          // ready-queue rỗng, không còn gì để chạy
        Committed,       // thực thi + validate OK, scheduling state (executed/in-degree/ready-queue) đã cập nhật
        Rejected,        // provider trả Rejected: không commit, component coi như bị loại khỏi ready-queue
        ProviderError,   // provider trả Error: facade không được đụng vào executionState
        ValidationError  // output thiếu declared key: facade vẫn phải áp outResult->output trước khi báo lỗi
    };

    // Các bộ đếm chỉ phát sinh trong lúc đọc/ghi token (prepareIncomingTokens/routeOutgoingTokens).
    // Không gồm redactedFieldCount vì việc redact phụ thuộc m_sensitiveDebugKeys - thuộc facade.
    struct Telemetry
    {
        qint64 payloadBytesRead = 0;
        qint64 payloadBytesWritten = 0;
        qint64 maxPayloadBytes = 0;
        int tokenReadCount = 0;
        int tokenWriteCount = 0;
    };

    virtual ~IExecutionEngine() = default;

    // Build snapshot + in-degree + ready-queue ban đầu từ GraphModel.
    virtual bool prepare(GraphModel *graph,
                         const QHash<QString, const IExecutionSemanticsProvider *> &providersByType,
                         const QVariantMap &inputSnapshot,
                         QString *error) = 0;

    virtual bool hasReadyWork() const = 0;
    // Peek không xoá khỏi ready-queue; dùng cho snapshotSummary()/debug.
    virtual QString peekNextReadyComponentId() const = 0;
    // Toàn bộ id đang ready, theo đúng thứ tự nội bộ - dùng cho snapshotSummary()/debug.
    virtual QStringList readyComponentIds() const = 0;

    virtual int executedCount() const = 0;
    virtual int totalComponentCount() const = 0;
    virtual const cme::GraphSnapshot &graphSnapshot() const = 0;
    virtual Telemetry telemetry() const = 0;

    // Thực thi đúng 1 component ready tiếp theo (đồng bộ, blocking tới khi có kết quả).
    virtual StepOutcome executeNext(const QVariantMap &legacyGlobalState,
                                    ExecutionContext *outCtx,
                                    ExecuteResult *outResult,
                                    QString *error) = 0;

    virtual void reset() = 0;
};

#endif // IEXECUTIONENGINE_H
