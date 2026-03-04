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

    QJsonArray componentsArray;
    for (const ComponentModel *component : graph->componentList()) {
        QJsonObject obj;
        obj[QStringLiteral("id")]    = component->id();
        obj[QStringLiteral("label")] = component->label();
        obj[QStringLiteral("x")]     = component->x();
        obj[QStringLiteral("y")]     = component->y();
        obj[QStringLiteral("color")] = component->color();
        obj[QStringLiteral("type")]  = component->type();
        componentsArray.append(obj);
    }

    QJsonArray connectionsArray;
    for (const ConnectionModel *connection : graph->connectionList()) {
        QJsonObject obj;
        obj[QStringLiteral("id")]       = connection->id();
        obj[QStringLiteral("sourceId")] = connection->sourceId();
        obj[QStringLiteral("targetId")] = connection->targetId();
        obj[QStringLiteral("label")]    = connection->label();
        connectionsArray.append(obj);
    }

    QJsonObject root;
    root[QStringLiteral("components")] = componentsArray;
    root[QStringLiteral("connections")] = connectionsArray;

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

    const QJsonArray components = root[QStringLiteral("components")].toArray();
    for (const QJsonValue &v : components) {
        const QJsonObject obj = v.toObject();
        auto *component = new ComponentModel(
            obj[QStringLiteral("id")].toString(),
            obj[QStringLiteral("label")].toString(),
            obj[QStringLiteral("x")].toDouble(),
            obj[QStringLiteral("y")].toDouble(),
            obj[QStringLiteral("color")].toString(QStringLiteral("#4fc3f7")),
            obj[QStringLiteral("type")].toString(QStringLiteral("default")));
        graph->addComponent(component);
    }

    const QJsonArray connections = root[QStringLiteral("connections")].toArray();
    for (const QJsonValue &v : connections) {
        const QJsonObject obj = v.toObject();
        auto *connection = new ConnectionModel(
            obj[QStringLiteral("id")].toString(),
            obj[QStringLiteral("sourceId")].toString(),
            obj[QStringLiteral("targetId")].toString(),
            obj[QStringLiteral("label")].toString());
        graph->addConnection(connection);
    }

    return true;
}
