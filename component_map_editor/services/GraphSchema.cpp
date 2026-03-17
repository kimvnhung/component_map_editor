#include "GraphSchema.h"

namespace GraphSchema {

namespace Keys {

const QString &coordinateSystem() { static const QString key = QStringLiteral("coordinateSystem"); return key; }
const QString &components() { static const QString key = QStringLiteral("components"); return key; }
const QString &connections() { static const QString key = QStringLiteral("connections"); return key; }

const QString &componentId() { static const QString key = QStringLiteral("id"); return key; }
const QString &componentTitle() { static const QString key = QStringLiteral("title"); return key; }
const QString &componentContent() { static const QString key = QStringLiteral("content"); return key; }
const QString &componentIcon() { static const QString key = QStringLiteral("icon"); return key; }
const QString &componentX() { static const QString key = QStringLiteral("x"); return key; }
const QString &componentY() { static const QString key = QStringLiteral("y"); return key; }
const QString &componentWidth() { static const QString key = QStringLiteral("width"); return key; }
const QString &componentHeight() { static const QString key = QStringLiteral("height"); return key; }
const QString &componentShape() { static const QString key = QStringLiteral("shape"); return key; }
const QString &componentColor() { static const QString key = QStringLiteral("color"); return key; }
const QString &componentType() { static const QString key = QStringLiteral("type"); return key; }

const QString &connectionId() { static const QString key = QStringLiteral("id"); return key; }
const QString &connectionSourceId() { static const QString key = QStringLiteral("sourceId"); return key; }
const QString &connectionTargetId() { static const QString key = QStringLiteral("targetId"); return key; }
const QString &connectionLabel() { static const QString key = QStringLiteral("label"); return key; }
const QString &connectionSourceSide() { static const QString key = QStringLiteral("sourceSide"); return key; }
const QString &connectionTargetSide() { static const QString key = QStringLiteral("targetSide"); return key; }

} // namespace Keys

namespace CoordinateSystem {

const QString &current()
{
    static const QString value = QStringLiteral("world-center-y-down-v3");
    return value;
}

const QString &legacyTopLeftYDownV2()
{
    static const QString value = QStringLiteral("world-top-left-y-down-v2");
    return value;
}

} // namespace CoordinateSystem

QStringList componentFieldOrder()
{
    return {
        Keys::componentId(),
        Keys::componentTitle(),
        Keys::componentContent(),
        Keys::componentIcon(),
        Keys::componentX(),
        Keys::componentY(),
        Keys::componentWidth(),
        Keys::componentHeight(),
        Keys::componentShape(),
        Keys::componentColor(),
        Keys::componentType()
    };
}

QStringList connectionFieldOrder()
{
    return {
        Keys::connectionId(),
        Keys::connectionSourceId(),
        Keys::connectionTargetId(),
        Keys::connectionLabel(),
        Keys::connectionSourceSide(),
        Keys::connectionTargetSide()
    };
}

ConnectionModel::Side normalizeConnectionSide(int sideValue)
{
    switch (sideValue) {
    case ConnectionModel::SideTop:
    case ConnectionModel::SideRight:
    case ConnectionModel::SideBottom:
    case ConnectionModel::SideLeft:
    case ConnectionModel::SideAuto:
        return static_cast<ConnectionModel::Side>(sideValue);
    default:
        return ConnectionModel::SideAuto;
    }
}

QJsonObject componentToJson(const ComponentModel *component)
{
    QJsonObject obj;
    if (!component)
        return obj;

    obj[Keys::componentId()] = component->id();
    obj[Keys::componentTitle()] = component->title();
    obj[Keys::componentContent()] = component->content();
    obj[Keys::componentIcon()] = component->icon();
    obj[Keys::componentX()] = component->x();
    obj[Keys::componentY()] = component->y();
    obj[Keys::componentWidth()] = component->width();
    obj[Keys::componentHeight()] = component->height();
    obj[Keys::componentShape()] = component->shape();
    obj[Keys::componentColor()] = component->color();
    obj[Keys::componentType()] = component->type();
    return obj;
}

QJsonObject connectionToJson(const ConnectionModel *connection)
{
    QJsonObject obj;
    if (!connection)
        return obj;

    obj[Keys::connectionId()] = connection->id();
    obj[Keys::connectionSourceId()] = connection->sourceId();
    obj[Keys::connectionTargetId()] = connection->targetId();
    obj[Keys::connectionLabel()] = connection->label();
    obj[Keys::connectionSourceSide()] = static_cast<int>(connection->sourceSide());
    obj[Keys::connectionTargetSide()] = static_cast<int>(connection->targetSide());
    return obj;
}

ComponentModel *componentFromCanonicalJson(const QJsonObject &obj)
{
    const qreal width = obj[Keys::componentWidth()].toDouble(96.0);
    const qreal height = obj[Keys::componentHeight()].toDouble(96.0);
    const QString title = obj[Keys::componentTitle()].toString();

    auto *component = new ComponentModel(
        obj[Keys::componentId()].toString(),
        title,
        obj[Keys::componentX()].toDouble(),
        obj[Keys::componentY()].toDouble(),
        obj[Keys::componentColor()].toString(QStringLiteral("#4fc3f7")),
        obj[Keys::componentType()].toString(QStringLiteral("default")));

    component->setWidth(width);
    component->setHeight(height);
    component->setShape(obj[Keys::componentShape()].toString(QStringLiteral("rounded")));
    component->setTitle(title);
    component->setContent(obj[Keys::componentContent()].toString());
    component->setIcon(obj[Keys::componentIcon()].toString());
    return component;
}

ConnectionModel *connectionFromCanonicalJson(const QJsonObject &obj)
{
    auto *connection = new ConnectionModel(
        obj[Keys::connectionId()].toString(),
        obj[Keys::connectionSourceId()].toString(),
        obj[Keys::connectionTargetId()].toString(),
        obj[Keys::connectionLabel()].toString());

    connection->setSourceSide(normalizeConnectionSide(
        obj[Keys::connectionSourceSide()].toInt(static_cast<int>(ConnectionModel::SideAuto))));
    connection->setTargetSide(normalizeConnectionSide(
        obj[Keys::connectionTargetSide()].toInt(static_cast<int>(ConnectionModel::SideAuto))));
    return connection;
}

} // namespace GraphSchema
