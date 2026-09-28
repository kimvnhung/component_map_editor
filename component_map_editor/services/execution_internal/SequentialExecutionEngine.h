#ifndef SEQUENTIALEXECUTIONENGINE_H
#define SEQUENTIALEXECUTIONENGINE_H

#include "IExecutionEngine.h"

class SequentialExecutionEngine: public IExecutionEngine
{
public:
    SequentialExecutionEngine();

    // IExecutionEngine interface
public:
    bool prepare(GraphModel *graph,
                const QHash<QString, const IExecutionSemanticsProvider *> &providersByType,
                const QVariantMap &inputSnapshot,
                QString *error) override;
    bool hasReadyWork() const override;
    QString peekNextReadyComponentId() const override;
    QStringList readyComponentIds() const override;
    int executedCount() const override;
    int totalComponentCount() const override;
    const cme::GraphSnapshot &graphSnapshot() const override;
    Telemetry telemetry() const override;
    StepOutcome executeNext(const QVariantMap &legacyGlobalState,
                           ExecutionContext *outCtx,
                           ExecuteResult *outResult,
                           QString *error) override;
    void reset() override;
private:
    void enqueueReadyComponent(const QString &componentId);
    /**
     * @brief dequeNextComponent : Dequeues the next component from the ready queue.
     * @return The ID of the next component to execute, or an empty string if the queue is empty.
     */
    QString dequeNextComponent();
    void prepareIncomingTokens(const QString& componentId, ExecutionContext& ctx, const QVariantMap &legacyGlobalState);
    ExecuteResult invokeProvider(const ExecutionContext& ctx);
    bool validateExecutionResult(const ExecuteResult& result, QString& message);
    void routeOutgoingTokens(const ExecutionContext& ctx, const ExecuteResult& result);
    // Chỉ cập nhật executed-set/in-degree/ready-queue; tick/timeline/componentStates thuộc facade (Plan 1.3).
    void commitSchedulingState(const ExecutionContext& ctx);

private:
    cme::GraphSnapshot m_graphSnapshot;
    QHash<std::string, int> m_pendingInDegree;
    QSet<QString> m_executed;
    QStringList m_readyQueue;
    QSet<QString> m_readyQueueSet;
    QVariantMap m_inputSnapshot;
    QHash<QString, const IExecutionSemanticsProvider *> m_providerByComponentType;

    qint64 m_payloadBytesRead = 0;
    qint64 m_payloadBytesWritten = 0;
    qint64 m_maxPayloadBytes = 0;
    int m_tokenReadCount = 0;
    int m_tokenWriteCount = 0;
};

#endif // SEQUENTIALEXECUTIONENGINE_H
