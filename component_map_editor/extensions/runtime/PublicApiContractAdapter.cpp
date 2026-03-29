#include "PublicApiContractAdapter.h"

#include <QSet>

#include "extensions/runtime/templates/TemplateProtoHelpers.h"

namespace {

bool setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
    return false;
}

} // namespace

namespace cme::runtime {

cme::publicapi::v1::ProviderIdentity PublicApiContractAdapter::makeProviderIdentity(const QString &providerId,
                                                                                     const QString &interfaceIid)
{
    cme::publicapi::v1::ProviderIdentity out;
    out.set_provider_id(providerId.toStdString());
    if (!interfaceIid.isEmpty())
        out.set_interface_iid(interfaceIid.toStdString());
    return out;
}

void PublicApiContractAdapter::variantMapToProtoStruct(const QVariantMap &in,
                                                       google::protobuf::Struct *out)
{
    if (!out)
        return;
    out->Clear();
    for (auto it = in.constBegin(); it != in.constEnd(); ++it)
        (*out->mutable_fields())[it.key().toStdString()] = cme::runtime::templates::variantToProtoValue(it.value());
}

QVariantMap PublicApiContractAdapter::protoStructToVariantMap(const google::protobuf::Struct &in)
{
    QVariantMap out;
    for (const auto &kv : in.fields())
        out.insert(QString::fromStdString(kv.first), cme::runtime::templates::protoValueToVariant(kv.second));
    return out;
}

bool PublicApiContractAdapter::toActionDescriptor(const QString &providerId,
                                                  const QString &actionId,
                                                  const QVariantMap &descriptor,
                                                  cme::publicapi::v1::ActionDescriptor *out,
                                                  QString *error)
{
    if (!out)
        return setError(error, QStringLiteral("ActionDescriptor output pointer is null"));

    out->Clear();
    out->set_action_id(actionId.toStdString());

    const QString title = descriptor.value(QStringLiteral("title")).toString();
    if (title.isEmpty())
        return setError(error, QStringLiteral("Action '%1' from provider '%2' is missing title")
                               .arg(actionId, providerId));

    out->set_title(title.toStdString());
    out->set_category(descriptor.value(QStringLiteral("category")).toString().toStdString());
    out->set_icon(descriptor.value(QStringLiteral("icon")).toString().toStdString());
    out->set_enabled(descriptor.value(QStringLiteral("enabled"), true).toBool());
    out->set_hint(descriptor.value(QStringLiteral("hint")).toString().toStdString());

    static const QSet<QString> kKnownKeys = {
        QStringLiteral("id"),
        QStringLiteral("title"),
        QStringLiteral("category"),
        QStringLiteral("icon"),
        QStringLiteral("enabled"),
        QStringLiteral("hint")
    };

    for (auto it = descriptor.constBegin(); it != descriptor.constEnd(); ++it) {
        if (kKnownKeys.contains(it.key()))
            continue;
        (*out->mutable_extra())[it.key().toStdString()] = cme::runtime::templates::variantToProtoValue(it.value());
    }

    return true;
}

bool PublicApiContractAdapter::toPropertySchemaEntry(const QVariantMap &entry,
                                                     cme::publicapi::v1::PropertySchemaEntry *out,
                                                     QString *error)
{
    if (!out)
        return setError(error, QStringLiteral("PropertySchemaEntry output pointer is null"));

    const QString key = entry.value(QStringLiteral("key")).toString();
    const QString type = entry.value(QStringLiteral("type")).toString();
    if (key.isEmpty() || type.isEmpty())
        return setError(error, QStringLiteral("Property schema entry must include non-empty 'key' and 'type'"));

    out->Clear();
    out->set_key(key.toStdString());
    out->set_type(type.toStdString());
    out->set_title(entry.value(QStringLiteral("title")).toString().toStdString());
    out->set_required(entry.value(QStringLiteral("required"), false).toBool());
    out->set_editor_widget(entry.value(QStringLiteral("editor")).toString().toStdString());
    out->set_section(entry.value(QStringLiteral("section")).toString().toStdString());
    out->set_order(entry.value(QStringLiteral("order"), 0).toInt());
    out->set_hint(entry.value(QStringLiteral("hint")).toString().toStdString());

    if (entry.contains(QStringLiteral("defaultValue")))
        *out->mutable_default_value() = cme::runtime::templates::variantToProtoValue(entry.value(QStringLiteral("defaultValue")));

    const QVariantList options = entry.value(QStringLiteral("options")).toList();
    for (const QVariant &option : options)
        *out->add_options() = cme::runtime::templates::variantToProtoValue(option);

    const QVariantMap validation = entry.value(QStringLiteral("validation")).toMap();
    for (auto it = validation.constBegin(); it != validation.constEnd(); ++it)
        (*out->mutable_validation())[it.key().toStdString()] = cme::runtime::templates::variantToProtoValue(it.value());

    const QVariantMap visibleWhen = entry.value(QStringLiteral("visibleWhen")).toMap();
    for (auto it = visibleWhen.constBegin(); it != visibleWhen.constEnd(); ++it)
        (*out->mutable_visible_when())[it.key().toStdString()] = cme::runtime::templates::variantToProtoValue(it.value());

    static const QSet<QString> kKnownKeys = {
        QStringLiteral("key"),
        QStringLiteral("type"),
        QStringLiteral("title"),
        QStringLiteral("required"),
        QStringLiteral("defaultValue"),
        QStringLiteral("editor"),
        QStringLiteral("section"),
        QStringLiteral("order"),
        QStringLiteral("hint"),
        QStringLiteral("options"),
        QStringLiteral("validation"),
        QStringLiteral("visibleWhen")
    };

    for (auto it = entry.constBegin(); it != entry.constEnd(); ++it) {
        if (kKnownKeys.contains(it.key()))
            continue;
        (*out->mutable_extra())[it.key().toStdString()] = cme::runtime::templates::variantToProtoValue(it.value());
    }

    return true;
}

} // namespace cme::runtime
