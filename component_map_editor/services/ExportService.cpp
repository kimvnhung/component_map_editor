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
        obj[QStringLiteral("title")] = component->title();
        obj[QStringLiteral("content")] = component->content();
        obj[QStringLiteral("icon")] = component->icon();
        obj[QStringLiteral("x")]     = component->x();
        obj[QStringLiteral("y")]     = component->y();
        obj[QStringLiteral("width")] = component->width();
        obj[QStringLiteral("height")] = component->height();
        obj[QStringLiteral("shape")] = component->shape();
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
    // Geometry contract v3:
    // - component x/y represent center points in world coordinates.
    // - world uses Y-down, matching QML item coordinates.
    root[QStringLiteral("coordinateSystem")] = QStringLiteral("world-center-y-down-v3");
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
    const QString coordinateSystem = root[QStringLiteral("coordinateSystem")].toString();
    const bool isV3CenterYDown = coordinateSystem == QStringLiteral("world-center-y-down-v3");
    const bool isV2TopLeftYDown = coordinateSystem == QStringLiteral("world-top-left-y-down-v2");

    graph->clear();

    const QJsonArray components = root[QStringLiteral("components")].toArray();
    for (const QJsonValue &v : components) {
        const QJsonObject obj = v.toObject();

        qreal width = obj[QStringLiteral("width")].toDouble(96.0);
        qreal height = obj[QStringLiteral("height")].toDouble(96.0);
        qreal x = obj[QStringLiteral("x")].toDouble();
        qreal y = obj[QStringLiteral("y")].toDouble();

        // Coordinate system compatibility:
        // - v3 stores center coordinates with Y-down: use as-is.
        // - v2 stores top-left coordinates with Y-down: convert to center.
        // - legacy files store center coordinates with Y-up: flip Y only.
        if (isV2TopLeftYDown) {
            x = x + width / 2.0;
            y = y + height / 2.0;
        } else if (!isV3CenterYDown) {
            y = -y;
        }

        const QString resolvedTitle = obj[QStringLiteral("title")].toString();
        auto *component = new ComponentModel(
            obj[QStringLiteral("id")].toString(),
            resolvedTitle,
            x,
            y,
            obj[QStringLiteral("color")].toString(QStringLiteral("#4fc3f7")),
            obj[QStringLiteral("type")].toString(QStringLiteral("default")));

        component->setWidth(width);
        component->setHeight(height);
        component->setShape(obj[QStringLiteral("shape")].toString(QStringLiteral("rounded")));
        component->setTitle(resolvedTitle);
        component->setContent(obj[QStringLiteral("content")].toString());
        component->setIcon(obj[QStringLiteral("icon")].toString());

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
