#include "ExportService.h"

#include "GraphJsonMigration.h"
#include "GraphSchema.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

ExportService::ExportService(QObject *parent)
    : QObject(parent)
{}

QString ExportService::exportToJson(GraphModel *graph)
{
    if (!graph)
        return QStringLiteral("{}");

    QJsonArray componentsArray;
    for (const ComponentModel *component : graph->componentList())
        componentsArray.append(GraphSchema::componentToJson(component));

    QJsonArray connectionsArray;
    for (const ConnectionModel *connection : graph->connectionList())
        connectionsArray.append(GraphSchema::connectionToJson(connection));

    QJsonObject root;
    // Geometry contract v3:
    // - component x/y represent center points in world coordinates.
    // - world uses Y-down, matching QML item coordinates.
    root[GraphSchema::Keys::coordinateSystem()] = GraphSchema::CoordinateSystem::current();
    root[GraphSchema::Keys::components()] = componentsArray;
    root[GraphSchema::Keys::connections()] = connectionsArray;

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

bool ExportService::importFromJson(GraphModel *graph, const QString &json)
{
    if (!graph)
        return false;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;

    const GraphJsonMigration::CanonicalDocument canonical =
        GraphJsonMigration::migrateToCurrentSchema(doc.object());

    graph->clear();
    graph->beginBatchUpdate();

    for (const QJsonValue &v : canonical.components) {
        auto *component = GraphSchema::componentFromCanonicalJson(v.toObject());
        graph->addComponent(component);
    }

    for (const QJsonValue &v : canonical.connections) {
        auto *connection = GraphSchema::connectionFromCanonicalJson(v.toObject());
        graph->addConnection(connection);
    }

    graph->endBatchUpdate();

    return true;
}
