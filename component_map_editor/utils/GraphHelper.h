#ifndef GRAPHHELPER_H
#define GRAPHHELPER_H

#include <QStringList>
#include <graph.pb.h>
#include "extensions/contracts/IExecutionSemanticsProvider.h"

namespace cme::helper
{
    QVariantMap mapToVariantMap(const google::protobuf::Map<std::string, std::string> &map);
    google::protobuf::Map<std::string, std::string> variantMapToMap(const QVariantMap &variantMap);
    QStringList getComponentIds(const cme::GraphSnapshot &graph);
    ComponentData getComponentById(const cme::GraphSnapshot &graph, const QString &componentId);
    int getComponentCount(const cme::GraphSnapshot &graph);
    QList<ConnectionData> getConnectionsBySourceId(const cme::GraphSnapshot &graph, const QString &sourceId);
    QHash<QString, QList<ConnectionData>> getOutgoingConnectionsBySourceId(const cme::GraphSnapshot &graph);
    QList<ConnectionData> getConnectionsByTargetId(const cme::GraphSnapshot &graph, const QString &targetId);
    QHash<QString, QList<ConnectionData>> getIncomingConnectionsByTargetId(const cme::GraphSnapshot &graph);
    execution::ExecutionPayload getConnectionPayloadById(const cme::GraphSnapshot &graph, const QString &connectionId);
    bool setPayload(cme::GraphSnapshot &graph, const QString &connectionId, const execution::ExecutionPayload &payload);
}
#endif // GRAPHHELPER_H
