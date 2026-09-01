#ifndef EXECUTIONCONTEXT_H
#define EXECUTIONCONTEXT_H

#include <QString>
#include <QVariantMap>

#include <graph.pb.h>

#include "extensions/contracts/IExecutionSemanticsProvider.h"

struct ExecutionContext
{
    QString componentId;
    QString componentType;
    bool tokenRoutingEnabled;
    cme::execution::IncomingTokens incomingTokens;
    QVariantMap stepState;
    QVariantMap trace;
};

struct ExecuteResult
{
    enum Status { Ok, Rejected, Error } status;
    QVariantMap output; // default token payload
    QString providerId;
    cme::ComponentData componentData;
    QStringList requiredOutputKeys; // list of keys that must be present in the output
    QVariantMap trace; // { providerId, durationMs, status, details }
    QString errorMessage;
};

#endif // EXECUTIONCONTEXT_H
