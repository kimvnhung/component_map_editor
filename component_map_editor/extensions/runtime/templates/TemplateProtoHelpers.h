#pragma once

#include <QString>
#include <QVariant>
#include <QVariantMap>

#include <google/protobuf/struct.pb.h>

#include "policy.pb.h"
#include "provider_templates.pb.h"

namespace cme::runtime::templates {

google::protobuf::Value variantToProtoValue(const QVariant &value);
QVariant protoValueToVariant(const google::protobuf::Value &value);

cme::templates::v1::ComponentTypeTemplate makeComponentTypeTemplate(
    const QString &id,
    const QString &title,
    const QString &category,
    double defaultWidth,
    double defaultHeight,
    const QString &defaultColor,
    bool allowIncoming,
    bool allowOutgoing);

cme::templates::v1::ComponentTypeDefaultPropertiesTemplate makeComponentTypeDefaultsTemplate(
    const QString &componentTypeId,
    const QVariantMap &properties);

cme::templates::v1::ConnectionPolicyRuleTemplate makeConnectionPolicyRuleTemplate(
    const QString &sourceTypeId,
    const QString &targetTypeId,
    bool allowed,
    const QString &reason);

QString formatUnknownConnectionReason(const cme::ConnectionPolicyContext &context,
                                     const QString &pattern);

} // namespace cme::runtime::templates
