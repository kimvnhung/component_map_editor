#include "ExportService.h"

#include "GraphSchema.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

ExportService::ExportService(QObject* parent) : QObject(parent) {}

QString ExportService::exportToJson(GraphModel* graph)
{
    if (!graph)
    {
        return QStringLiteral("{}");
    }

    QJsonArray componentsArray;

    for (const ComponentModel* component : graph->componentList())
    {
        componentsArray.append(GraphSchema::componentToJson(component));
    }

    QJsonArray connectionsArray;

    for (const ConnectionModel* connection : graph->connectionList())
    {
        connectionsArray.append(GraphSchema::connectionToJson(connection));
    }

    QJsonObject root;
    // Geometry contract v3:
    // - component x/y represent center points in world coordinates.
    // - world uses Y-down, matching QML item coordinates.
    root[GraphSchema::Keys::coordinateSystem()] = GraphSchema::CoordinateSystem::current();
    root[GraphSchema::Keys::components()] = componentsArray;
    root[GraphSchema::Keys::connections()] = connectionsArray;
    root[GraphSchema::Keys::graphId()] = graph->id();
    root[GraphSchema::Keys::graphTitle()] = graph->title();
    root[GraphSchema::Keys::graphStartComponentId()] = graph->startComponentId();

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

bool ExportService::importFromJson(GraphModel* graph, const QString& json)
{
    if (!graph)
    {
        return false;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);

    if (err.error != QJsonParseError::NoError || !doc.isObject())
    {
        return false;
    }

    const QJsonObject root = doc.object();
    const QJsonValue coordinateSystem = root.value(GraphSchema::Keys::coordinateSystem());
    if (!coordinateSystem.isString()
        || coordinateSystem.toString() != GraphSchema::CoordinateSystem::current()) {
        return false;
    }

    graph->clear();
    graph->beginBatchUpdate();

    graph->setId(root.value(GraphSchema::Keys::graphId()).toString());
    graph->setTitle(root.value(GraphSchema::Keys::graphTitle()).toString());
    graph->setStartComponentId(root.value(GraphSchema::Keys::graphStartComponentId()).toString());

    for (const QJsonValue& v : root.value(GraphSchema::Keys::components()).toArray())
    {
        auto* component = GraphSchema::componentFromCanonicalJson(v.toObject());
        graph->addComponent(component);
    }

    for (const QJsonValue& v : root.value(GraphSchema::Keys::connections()).toArray())
    {
        auto* connection = GraphSchema::connectionFromCanonicalJson(v.toObject());
        graph->addConnection(connection);
    }

    graph->endBatchUpdate();

    return true;
}

bool ExportService::exportToJsonFile(GraphModel* graph, const QString& filePath)
{
    if (!graph)
    {
        return false;
    }

    // Convert format file:/// to absolute path
    QFile file(QUrl(filePath).toLocalFile());

    // Auto create new or replace existing file
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        return false;
    }

    const QString json = exportToJson(graph);
    file.write(json.toUtf8());
    file.close();

    return true;
}

bool ExportService::importFromJsonFile(GraphModel* graph, const QString& filePath)
{
    if (!graph)
    {
        return false;
    }

    // Convert format file:/// to absolute path
    QFile file(QUrl(filePath).toLocalFile());

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    const QString json = QString::fromUtf8(file.readAll());
    file.close();

    return importFromJson(graph, json);
}
