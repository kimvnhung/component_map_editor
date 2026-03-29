#pragma once

#include <QVariantMap>

#include <google/protobuf/struct.pb.h>

#include "public_api.pb.h"

namespace cme::runtime {

// Initial scaffold for bridging runtime QVariant-based contracts to explicit
// protobuf public API messages.
class PublicApiContractAdapter
{
public:
    static cme::publicapi::v1::ProviderIdentity makeProviderIdentity(const QString &providerId,
                                                                      const QString &interfaceIid = QString());

    static void variantMapToProtoStruct(const QVariantMap &in,
                                        google::protobuf::Struct *out);

    static QVariantMap protoStructToVariantMap(const google::protobuf::Struct &in);

    static bool toActionDescriptor(const QString &providerId,
                                   const QString &actionId,
                                   const QVariantMap &descriptor,
                                   cme::publicapi::v1::ActionDescriptor *out,
                                   QString *error);

    static bool toPropertySchemaEntry(const QVariantMap &entry,
                                      cme::publicapi::v1::PropertySchemaEntry *out,
                                      QString *error);
};

} // namespace cme::runtime
