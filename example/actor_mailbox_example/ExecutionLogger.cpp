#include "ExecutionLogger.h"

void ExecutionLogger::logExecutionStarted(const QString &componentId, const Timestamp &ts)
{
    // Log execution start event
}

void ExecutionLogger::logExecutionCompleted(const QString &componentId,
        const ExecuteResult &result, Duration elapsed)
{
    // Log execution completion event
}

void ExecutionLogger::logTokensProduced(const QString &componentId, const QString &port, int count)
{
    // Log tokens produced event
}

void ExecutionLogger::logError(const QString &componentId, const QString &message)
{
    // Log error event
}

void ExecutionLogger::flushToTimelineModel(TimelineModel* timeline)
{
    // Flush logs to the TimelineModel in a non-blocking manner
}