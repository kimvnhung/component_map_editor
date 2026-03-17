#ifndef GRAPHSCHEMA_H
#define GRAPHSCHEMA_H

#include <QJsonObject>
#include <QString>
#include <QStringList>

#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"

namespace GraphSchema {

namespace Keys {

const QString &coordinateSystem();
const QString &components();
const QString &connections();

const QString &componentId();
const QString &componentTitle();
const QString &componentContent();
const QString &componentIcon();
const QString &componentX();
const QString &componentY();
const QString &componentWidth();
const QString &componentHeight();
const QString &componentShape();
const QString &componentColor();
const QString &componentType();

const QString &connectionId();
const QString &connectionSourceId();
const QString &connectionTargetId();
const QString &connectionLabel();
const QString &connectionSourceSide();
const QString &connectionTargetSide();

} // namespace Keys

namespace CoordinateSystem {

const QString &current();
const QString &legacyTopLeftYDownV2();

} // namespace CoordinateSystem

QStringList componentFieldOrder();
QStringList connectionFieldOrder();

ConnectionModel::Side normalizeConnectionSide(int sideValue);

QJsonObject componentToJson(const ComponentModel *component);
QJsonObject connectionToJson(const ConnectionModel *connection);

ComponentModel *componentFromCanonicalJson(const QJsonObject &obj);
ConnectionModel *connectionFromCanonicalJson(const QJsonObject &obj);

} // namespace GraphSchema

#endif // GRAPHSCHEMA_H
