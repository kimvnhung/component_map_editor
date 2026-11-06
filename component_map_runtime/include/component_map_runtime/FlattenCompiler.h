#pragma once
#include <deployment.pb.h>
#include <QString>
#include <QList>
#include <QMap>
namespace cme::runtime {
struct FlattenLimits { int maxDepth=16; int maxLeaves=10000; int maxRoutes=20000; int maxSerializedBytes=16*1024*1024; };
struct FlattenLeaf { QString qualifiedPath, componentType, graphScope, definitionId, definitionVersion; quint32 stateSchemaVersion=0; QMap<QString,QString> resolvedProperties; };
struct FlattenRoute { QString routeId, sourceLeafPath, sourcePort, targetLeafPath, targetPort, messageType, sourceDefinitionPath, targetDefinitionPath, boundaryPath; };
struct ExecutionPlan { QList<FlattenLeaf> leaves; QList<FlattenRoute> routes; QByteArray hash; };
struct FlattenIssue { QString code, fieldPath, message; };
struct FlattenResult { bool valid=false; ExecutionPlan plan; QList<FlattenIssue> issues; };
class FlattenCompiler { public: explicit FlattenCompiler(FlattenLimits limits={}); FlattenResult compile(const cme::runtime::v1::DeploymentGraph&) const; private: FlattenLimits m_limits; };
}
