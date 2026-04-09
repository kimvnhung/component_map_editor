#include "GraphJsonMigration.h"

#include "GraphSchema.h"

namespace GraphJsonMigration {

namespace {

QJsonObject normalizeComponentObject(const QJsonObject &obj, const QString &coordinateSystem)
{
    const qreal width = obj[GraphSchema::Keys::componentWidth()].toDouble(96.0);
    const qreal height = obj[GraphSchema::Keys::componentHeight()].toDouble(96.0);
    qreal x = obj[GraphSchema::Keys::componentX()].toDouble();
    qreal y = obj[GraphSchema::Keys::componentY()].toDouble();

    if (coordinateSystem == GraphSchema::CoordinateSystem::legacyTopLeftYDownV2()) {
        x += width / 2.0;
        y += height / 2.0;
    } else if (coordinateSystem != GraphSchema::CoordinateSystem::current()) {
        // Legacy compatibility: old files used center coordinates with Y-up.
        y = -y;
    }

    QJsonObject normalized;
    normalized[GraphSchema::Keys::componentId()] = obj[GraphSchema::Keys::componentId()].toString();
    normalized[GraphSchema::Keys::componentTitle()] = obj[GraphSchema::Keys::componentTitle()].toString();
    normalized[GraphSchema::Keys::componentContent()] = obj[GraphSchema::Keys::componentContent()].toString();
    normalized[GraphSchema::Keys::componentIcon()] = obj[GraphSchema::Keys::componentIcon()].toString();
    normalized[GraphSchema::Keys::componentX()] = x;
    normalized[GraphSchema::Keys::componentY()] = y;
    normalized[GraphSchema::Keys::componentWidth()] = width;
    normalized[GraphSchema::Keys::componentHeight()] = height;
    normalized[GraphSchema::Keys::componentShape()] =
        obj[GraphSchema::Keys::componentShape()].toString(QStringLiteral("rounded"));
    normalized[GraphSchema::Keys::componentColor()] =
        obj[GraphSchema::Keys::componentColor()].toString(QStringLiteral("#4fc3f7"));
    normalized[GraphSchema::Keys::componentType()] =
        obj[GraphSchema::Keys::componentType()].toString(QStringLiteral("default"));
    return normalized;
}

QJsonObject normalizeConnectionObject(const QJsonObject &obj)
{
    QJsonObject normalized;
    normalized[GraphSchema::Keys::connectionId()] = obj[GraphSchema::Keys::connectionId()].toString();
    normalized[GraphSchema::Keys::connectionSourceId()] =
        obj[GraphSchema::Keys::connectionSourceId()].toString();
    normalized[GraphSchema::Keys::connectionTargetId()] =
        obj[GraphSchema::Keys::connectionTargetId()].toString();
    normalized[GraphSchema::Keys::connectionLabel()] = obj[GraphSchema::Keys::connectionLabel()].toString();
    normalized[GraphSchema::Keys::connectionSourceSide()] = static_cast<int>(
        GraphSchema::normalizeConnectionSide(obj[GraphSchema::Keys::connectionSourceSide()]
                                                 .toInt(static_cast<int>(ConnectionModel::SideAuto))));
    normalized[GraphSchema::Keys::connectionTargetSide()] = static_cast<int>(
        GraphSchema::normalizeConnectionSide(obj[GraphSchema::Keys::connectionTargetSide()]
                                                 .toInt(static_cast<int>(ConnectionModel::SideAuto))));
    return normalized;
}

} // namespace

CanonicalDocument migrateToCurrentSchema(const QJsonObject &root)
{
    CanonicalDocument out;

    const QString coordinateSystem = root[GraphSchema::Keys::coordinateSystem()].toString();

    const QJsonArray sourceComponents = root[GraphSchema::Keys::components()].toArray();
    for (const QJsonValue &value : sourceComponents)
        out.components.append(normalizeComponentObject(value.toObject(), coordinateSystem));

    const QJsonArray sourceConnections = root[GraphSchema::Keys::connections()].toArray();
    for (const QJsonValue &value : sourceConnections)
        out.connections.append(normalizeConnectionObject(value.toObject()));

    return out;
}

} // namespace GraphJsonMigration
