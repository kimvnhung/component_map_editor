#ifndef DEPLOYMENTGRAPHEXPORTER_H
#define DEPLOYMENTGRAPHEXPORTER_H

#include <QString>
#include <QStringList>

#include "models/GraphModel.h"
#include "deployment.pb.h"

namespace cme::editor
{

class DeploymentGraphExporter
{
public:
    struct Options
    {
        QString graphVersion;
        // Component dynamic properties use QVariantList entries with keys:
        // { id, messageType }. The exporter emits these as runtime ports.
        QString inputPortsProperty = QStringLiteral("inputPorts");
        QString outputPortsProperty = QStringLiteral("outputPorts");
        QString stateSchemaVersionProperty = QStringLiteral("stateSchemaVersion");
        QString compositeGraphIdProperty = QStringLiteral("compositeGraphId");
        QString compositeGraphVersionProperty = QStringLiteral("compositeGraphVersion");
        bool requireExplicitRouteMetadata = true;
        QString sourcePortProperty = QStringLiteral("sourcePort");
        QString targetPortProperty = QStringLiteral("targetPort");
        QString messageTypeProperty = QStringLiteral("messageType");
        // Only these dynamic properties are copied into the runtime properties map.
        // UI/editor metadata remains in the editor document and never crosses the boundary.
        QStringList runtimePropertyNames;
        quint32 defaultStateSchemaVersion = 1;
        QStringList validationTypeNames;
        bool validateWithKnownTypes = false;
    };

    // Converts the editor model into the runtime-owned deployment contract.
    // The output is replaced only after the complete graph has been converted.
    static bool exportGraph(const GraphModel *graph,
                            const Options &options,
                            cme::runtime::v1::DeploymentGraph *output,
                            QString *error = nullptr);

private:
    DeploymentGraphExporter() = delete;
};

} // namespace cme::editor

#endif // DEPLOYMENTGRAPHEXPORTER_H
