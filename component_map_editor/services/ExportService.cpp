#include "ExportService.h"

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

    QJsonArray nodesArray;
    for (const NodeModel *node : graph->nodeList()) {
        QJsonObject obj;
        obj[QStringLiteral("id")]    = node->id();
        obj[QStringLiteral("label")] = node->label();
        obj[QStringLiteral("x")]     = node->x();
        obj[QStringLiteral("y")]     = node->y();
        obj[QStringLiteral("color")] = node->color();
        obj[QStringLiteral("type")]  = node->type();
        nodesArray.append(obj);
    }

    QJsonArray edgesArray;
    for (const EdgeModel *edge : graph->edgeList()) {
        QJsonObject obj;
        obj[QStringLiteral("id")]       = edge->id();
        obj[QStringLiteral("sourceId")] = edge->sourceId();
        obj[QStringLiteral("targetId")] = edge->targetId();
        obj[QStringLiteral("label")]    = edge->label();
        edgesArray.append(obj);
    }

    QJsonObject root;
    root[QStringLiteral("nodes")] = nodesArray;
    root[QStringLiteral("edges")] = edgesArray;

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

    const QJsonObject root = doc.object();

    graph->clear();

    const QJsonArray nodes = root[QStringLiteral("nodes")].toArray();
    for (const QJsonValue &v : nodes) {
        const QJsonObject obj = v.toObject();
        auto *node = new NodeModel(
            obj[QStringLiteral("id")].toString(),
            obj[QStringLiteral("label")].toString(),
            obj[QStringLiteral("x")].toDouble(),
            obj[QStringLiteral("y")].toDouble(),
            obj[QStringLiteral("color")].toString(QStringLiteral("#4fc3f7")),
            obj[QStringLiteral("type")].toString(QStringLiteral("default")));
        graph->addNode(node);
    }

    const QJsonArray edges = root[QStringLiteral("edges")].toArray();
    for (const QJsonValue &v : edges) {
        const QJsonObject obj = v.toObject();
        auto *edge = new EdgeModel(
            obj[QStringLiteral("id")].toString(),
            obj[QStringLiteral("sourceId")].toString(),
            obj[QStringLiteral("targetId")].toString(),
            obj[QStringLiteral("label")].toString());
        graph->addEdge(edge);
    }

    return true;
}
