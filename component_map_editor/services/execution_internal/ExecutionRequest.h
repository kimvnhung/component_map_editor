#ifndef EXECUTIONREQUEST_H
#define EXECUTIONREQUEST_H

#include <QVariantMap>

struct ExecutionRequest
{
    QVariantMap inputSnapshot;
    QString requestedStartComponentId;
    bool allowUnreachableComponents = false;
    bool requireRootStart = true;
};

#endif // EXECUTIONREQUEST_H
