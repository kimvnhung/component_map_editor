#ifndef IEXECUTIONENGINE_H
#define IEXECUTIONENGINE_H

#include "ExecutionContext.h"
#include "extensions/contracts/IExecutionSemanticsProvider.h"
#include "models/GraphModel.h"

#include <graph.pb.h>
class IExecutionEngine
{
public:
    virtual ~IExecutionEngine() = default;

    // Build snapshot + in-degree + ready-queue ban đầu từ GraphModel.
    virtual bool prepare(GraphModel *graph,
                         const QHash<QString, const IExecutionSemanticsProvider *> &providersByType,
                         QString *error) = 0;

    virtual bool hasReadyWork() const = 0;
    virtual int executedCount() const = 0;
    virtual int totalComponentCount() const = 0;
    virtual const cme::GraphSnapshot &graphSnapshot() const = 0;

    // Thực thi đúng 1 component ready tiếp theo (đồng bộ, blocking tới khi có kết quả).
    // onCommitted được gọi đúng 1 lần khi state nội bộ engine (in-degree/ready-queue) đã cập nhật xong.
    virtual bool executeNext(const QVariantMap &legacyGlobalState,
                             bool bypassBreakpoint,
                             const std::function<void(const ExecutionContext &, const ExecuteResult &)> &onCommitted,
                             QString *error) = 0;

    virtual void reset() = 0;
};

#endif // IEXECUTIONENGINE_H
