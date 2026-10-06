#ifndef EXECUTIONLOGGER_H
#define EXECUTIONLOGGER_H

#include <QString>
#include "ExecutionStateCapture.h"
#include "TimelineModel.h"


class ExecutionLogger
{
    void logExecutionStarted(const QString &componentId, const Timestamp &ts);
    void logExecutionCompleted(const QString &componentId,
                               const ExecuteResult &result, Duration elapsed);
    void logTokensProduced(const QString &componentId, const QString &port, int count);
    void logError(const QString &componentId, const QString &message);

    // Non-blocking append to TimelineModel
    void flushToTimelineModel(TimelineModel* timeline);
};

#endif // EXECUTIONLOGGER_H
