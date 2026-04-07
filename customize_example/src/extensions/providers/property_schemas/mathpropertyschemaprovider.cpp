#include "mathpropertyschemaprovider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "propertyschematemplateutils.h"

namespace {

using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;
using cme::runtime::SchemaOptionsSource;

const QString kTokenKeyHint =
    QStringLiteral("Token options are sourced from connection token keys, execution-state keys, and schema key defaults.");

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema.math");
    bundle.set_schema_version("1.0.0");

    customize::property_schemas::addTarget(&bundle,
              "component/math/add",
              {
                  customize::property_schemas::makeField("inputAKey", SchemaFieldType::String, "Input A Key", true, QStringLiteral("a"), SchemaFieldWidget::Dropdown, "Context", 1,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("inputARef", SchemaFieldType::String, "Input A Ref", false, QString(), SchemaFieldWidget::Dropdown, "Context", 2,
                                                         QStringLiteral("Optional disambiguated incoming field reference."),
                                                         {}, {}, {},
                                                         SchemaOptionsSource::TokenKeyOptions),
                  customize::property_schemas::makeField("inputBKey", SchemaFieldType::String, "Input B Key", true, QStringLiteral("b"), SchemaFieldWidget::Dropdown, "Context", 3,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("outputKey", SchemaFieldType::String, "Output Key", true, QStringLiteral("sum"), SchemaFieldWidget::Dropdown, "Context", 4,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("a", SchemaFieldType::Number, "Fallback A", false, 0, SchemaFieldWidget::SpinBox, "Fallback", 20),
                  customize::property_schemas::makeField("b", SchemaFieldType::Number, "Fallback B", false, 0, SchemaFieldWidget::SpinBox, "Fallback", 21)
              });

    customize::property_schemas::addTarget(&bundle,
              "component/math/subtract",
              {
                  customize::property_schemas::makeField("inputAKey", SchemaFieldType::String, "Input A Key", true, QStringLiteral("a"), SchemaFieldWidget::Dropdown, "Context", 1,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("inputARef", SchemaFieldType::String, "Input A Ref", false, QString(), SchemaFieldWidget::Dropdown, "Context", 2,
                                                         QStringLiteral("Optional disambiguated incoming field reference."),
                                                         {}, {}, {},
                                                         SchemaOptionsSource::TokenKeyOptions),
                  customize::property_schemas::makeField("inputBKey", SchemaFieldType::String, "Input B Key", true, QStringLiteral("b"), SchemaFieldWidget::Dropdown, "Context", 3,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("outputKey", SchemaFieldType::String, "Output Key", true, QStringLiteral("difference"), SchemaFieldWidget::Dropdown, "Context", 4,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("errorKey", SchemaFieldType::String, "Error Key", true, QStringLiteral("error"), SchemaFieldWidget::Dropdown, "Context", 5,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("a", SchemaFieldType::Number, "Fallback A", false, 0, SchemaFieldWidget::SpinBox, "Fallback", 20),
                  customize::property_schemas::makeField("b", SchemaFieldType::Number, "Fallback B", false, 0, SchemaFieldWidget::SpinBox, "Fallback", 21)
              });

    customize::property_schemas::addTarget(&bundle,
              "component/math/multiply",
              {
                  customize::property_schemas::makeField("inputAKey", SchemaFieldType::String, "Input A Key", true, QStringLiteral("a"), SchemaFieldWidget::Dropdown, "Context", 1,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("inputARef", SchemaFieldType::String, "Input A Ref", false, QString(), SchemaFieldWidget::Dropdown, "Context", 2,
                                                         QStringLiteral("Optional disambiguated incoming field reference."),
                                                         {}, {}, {},
                                                         SchemaOptionsSource::TokenKeyOptions),
                  customize::property_schemas::makeField("inputBKey", SchemaFieldType::String, "Input B Key", true, QStringLiteral("b"), SchemaFieldWidget::Dropdown, "Context", 3,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("outputKey", SchemaFieldType::String, "Output Key", true, QStringLiteral("product"), SchemaFieldWidget::Dropdown, "Context", 4,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("errorKey", SchemaFieldType::String, "Error Key", true, QStringLiteral("error"), SchemaFieldWidget::Dropdown, "Context", 5,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("a", SchemaFieldType::Number, "Fallback A", false, 1, SchemaFieldWidget::SpinBox, "Fallback", 20),
                  customize::property_schemas::makeField("b", SchemaFieldType::Number, "Fallback B", false, 1, SchemaFieldWidget::SpinBox, "Fallback", 21)
              });

    customize::property_schemas::addTarget(&bundle,
              "component/math/divide",
              {
                  customize::property_schemas::makeField("inputAKey", SchemaFieldType::String, "Input A Key", true, QStringLiteral("a"), SchemaFieldWidget::Dropdown, "Context", 1,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("inputARef", SchemaFieldType::String, "Input A Ref", false, QString(), SchemaFieldWidget::Dropdown, "Context", 2,
                                                         QStringLiteral("Optional disambiguated incoming field reference."),
                                                         {}, {}, {},
                                                         SchemaOptionsSource::TokenKeyOptions),
                  customize::property_schemas::makeField("inputBKey", SchemaFieldType::String, "Input B Key", true, QStringLiteral("b"), SchemaFieldWidget::Dropdown, "Context", 3,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("outputKey", SchemaFieldType::String, "Output Key", true, QStringLiteral("quotient"), SchemaFieldWidget::Dropdown, "Context", 4,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("errorKey", SchemaFieldType::String, "Error Key", true, QStringLiteral("error"), SchemaFieldWidget::Dropdown, "Context", 5,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("a", SchemaFieldType::Number, "Fallback A", false, 1, SchemaFieldWidget::SpinBox, "Fallback", 20),
                  customize::property_schemas::makeField("b", SchemaFieldType::Number, "Fallback B", false, 1, SchemaFieldWidget::SpinBox, "Fallback", 21)
              });

    customize::property_schemas::addTarget(&bundle,
              "component/math/mod",
              {
                  customize::property_schemas::makeField("inputAKey", SchemaFieldType::String, "Input A Key", true, QStringLiteral("a"), SchemaFieldWidget::Dropdown, "Context", 1,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("inputARef", SchemaFieldType::String, "Input A Ref", false, QString(), SchemaFieldWidget::Dropdown, "Context", 2,
                                                         QStringLiteral("Optional disambiguated incoming field reference."),
                                                         {}, {}, {},
                                                         SchemaOptionsSource::TokenKeyOptions),
                  customize::property_schemas::makeField("inputBKey", SchemaFieldType::String, "Input B Key", true, QStringLiteral("b"), SchemaFieldWidget::Dropdown, "Context", 3,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("outputKey", SchemaFieldType::String, "Output Key", true, QStringLiteral("result"), SchemaFieldWidget::Dropdown, "Context", 4,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("errorKey", SchemaFieldType::String, "Error Key", true, QStringLiteral("error"), SchemaFieldWidget::Dropdown, "Context", 5,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("a", SchemaFieldType::Number, "Fallback A", false, 1, SchemaFieldWidget::SpinBox, "Fallback", 20),
                  customize::property_schemas::makeField("b", SchemaFieldType::Number, "Fallback B", false, 1, SchemaFieldWidget::SpinBox, "Fallback", 21)
              });

    return bundle;
}

const cme::templates::v1::PropertySchemaTemplateBundle &schemaBundle()
{
    static const cme::templates::v1::PropertySchemaTemplateBundle kBundle = buildTemplateBundle();
    return kBundle;
}

} // namespace

QString MathPropertySchemaProvider::providerId() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::providerId(schemaBundle());
}

QStringList MathPropertySchemaProvider::schemaTargets() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaTargets(schemaBundle());
}

QVariantList MathPropertySchemaProvider::propertySchema(const QString &targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(
        schemaBundle(), targetId);
}
