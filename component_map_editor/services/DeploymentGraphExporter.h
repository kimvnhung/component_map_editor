#ifndef DEPLOYMENTGRAPHEXPORTER_H
#define DEPLOYMENTGRAPHEXPORTER_H

#include <QString>

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
        QString defaultSourcePort = QStringLiteral("out");
        QString defaultTargetPort = QStringLiteral("in");
        QString defaultMessageType = QStringLiteral("event");
        quint32 defaultStateSchemaVersion = 1;
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
