#include "mathpropertyschemaprovider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "propertyschematemplateutils.h"

namespace
{

    using cme::runtime::SchemaFieldType;
    using cme::runtime::SchemaFieldWidget;
    using cme::runtime::SchemaFieldSection;
    using cme::runtime::SchemaOptionsSource;

    const QString kTokenRefHint =
        QStringLiteral("Choose an exact incoming token field reference. If no ref is selected, the fallback value below is used.");
    const QString kOutputKeyHint =
        QStringLiteral("Result field written by this component.");
    const QString kErrorKeyHint =
        QStringLiteral("Error field written when execution fails.");

    cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
    {
        cme::templates::v1::PropertySchemaTemplateBundle bundle;
        bundle.set_provider_id("customize.workflow.propertySchema.math");
        bundle.set_schema_version("1.0.0");

        customize::property_schemas::addTarget(&bundle,
                                               "component/math/add",
        {
            customize::property_schemas::makeField("title", SchemaFieldType::String, "Title", false, "Add Componnent", SchemaFieldWidget::TextField, SchemaFieldSection::General, 1),
            customize::property_schemas::makeField("inputARef", SchemaFieldType::String, "Input A Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 2,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("inputBRef", SchemaFieldType::String, "Input B Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 4,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("outputKey", SchemaFieldType::String, "Output Key", true, QStringLiteral("sum"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 5,
            kOutputKeyHint),
            customize::property_schemas::makeField("errorKey", SchemaFieldType::String, "Error Key", true, QStringLiteral("error"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 6,
            kErrorKeyHint),
            customize::property_schemas::makeField("a", SchemaFieldType::Number, "Fallback A", false, 0, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 20),
            customize::property_schemas::makeField("b", SchemaFieldType::Number, "Fallback B", false, 0, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 21)
        });

        customize::property_schemas::addTarget(&bundle,
                                               "component/math/subtract",
        {
            customize::property_schemas::makeField("title", SchemaFieldType::String, "Title", false, "Subtract Componnent", SchemaFieldWidget::TextField, SchemaFieldSection::General, 1),
            customize::property_schemas::makeField("inputARef", SchemaFieldType::String, "Input A Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 2,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("inputBRef", SchemaFieldType::String, "Input B Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 4,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("outputKey", SchemaFieldType::String, "Output Key", true, QStringLiteral("difference"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 5,
            kOutputKeyHint),
            customize::property_schemas::makeField("errorKey", SchemaFieldType::String, "Error Key", true, QStringLiteral("error"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 6,
            kErrorKeyHint),
            customize::property_schemas::makeField("a", SchemaFieldType::Number, "Fallback A", false, 0, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 20),
            customize::property_schemas::makeField("b", SchemaFieldType::Number, "Fallback B", false, 0, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 21)
        });

        customize::property_schemas::addTarget(&bundle,
                                               "component/math/multiply",
        {
            customize::property_schemas::makeField("inputARef", SchemaFieldType::String, "Input A Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 2,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("inputBRef", SchemaFieldType::String, "Input B Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 4,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("outputKey", SchemaFieldType::String, "Output Key", true, QStringLiteral("product"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 5,
            kOutputKeyHint),
            customize::property_schemas::makeField("errorKey", SchemaFieldType::String, "Error Key", true, QStringLiteral("error"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 6,
            kErrorKeyHint),
            customize::property_schemas::makeField("a", SchemaFieldType::Number, "Fallback A", false, 1, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 20),
            customize::property_schemas::makeField("b", SchemaFieldType::Number, "Fallback B", false, 1, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 21)
        });

        customize::property_schemas::addTarget(&bundle,
                                               "component/math/divide",
        {
            customize::property_schemas::makeField("inputARef", SchemaFieldType::String, "Input A Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 2,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("inputBRef", SchemaFieldType::String, "Input B Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 4,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("outputKey", SchemaFieldType::String, "Output Key", true, QStringLiteral("quotient"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 5,
            kOutputKeyHint),
            customize::property_schemas::makeField("errorKey", SchemaFieldType::String, "Error Key", true, QStringLiteral("error"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 6,
            kErrorKeyHint),
            customize::property_schemas::makeField("a", SchemaFieldType::Number, "Fallback A", false, 1, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 20),
            customize::property_schemas::makeField("b", SchemaFieldType::Number, "Fallback B", false, 1, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 21)
        });

        customize::property_schemas::addTarget(&bundle,
                                               "component/math/mod",
        {
            customize::property_schemas::makeField("inputARef", SchemaFieldType::String, "Input A Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 2,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("inputBRef", SchemaFieldType::String, "Input B Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 4,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("outputKey", SchemaFieldType::String, "Output Key", true, QStringLiteral("result"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 5,
            kOutputKeyHint),
            customize::property_schemas::makeField("errorKey", SchemaFieldType::String, "Error Key", true, QStringLiteral("error"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 6,
            kErrorKeyHint),
            customize::property_schemas::makeField("a", SchemaFieldType::Number, "Fallback A", false, 1, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 20),
            customize::property_schemas::makeField("b", SchemaFieldType::Number, "Fallback B", false, 1, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 21)
        });

        customize::property_schemas::addTarget(&bundle,
                                               "component/math/less_than",
        {
            customize::property_schemas::makeField("inputARef", SchemaFieldType::String, "Input A Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 2,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("inputBRef", SchemaFieldType::String, "Input B Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 4,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("outputKey", SchemaFieldType::String, "Output Key", true, QStringLiteral("result"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 5,
            kOutputKeyHint),
            customize::property_schemas::makeField("errorKey", SchemaFieldType::String, "Error Key", true, QStringLiteral("error"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 6,
            kErrorKeyHint),
            customize::property_schemas::makeField("a", SchemaFieldType::Number, "Fallback A", false, 0, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 20),
            customize::property_schemas::makeField("b", SchemaFieldType::Number, "Fallback B", false, 0, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 21)
        });

        customize::property_schemas::addTarget(&bundle,
                                               "component/math/less_or_equal",
        {
            customize::property_schemas::makeField("inputARef", SchemaFieldType::String, "Input A Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 2,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("inputBRef", SchemaFieldType::String, "Input B Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 4,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("outputKey", SchemaFieldType::String, "Output Key", true, QStringLiteral("result"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 5,
            kOutputKeyHint),
            customize::property_schemas::makeField("errorKey", SchemaFieldType::String, "Error Key", true, QStringLiteral("error"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 6,
            kErrorKeyHint),
            customize::property_schemas::makeField("a", SchemaFieldType::Number, "Fallback A", false, 0, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 20),
            customize::property_schemas::makeField("b", SchemaFieldType::Number, "Fallback B", false, 0, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 21)
        });

        customize::property_schemas::addTarget(&bundle,
                                               "component/math/equal",
        {
            customize::property_schemas::makeField("inputARef", SchemaFieldType::String, "Input A Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 2,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("inputBRef", SchemaFieldType::String, "Input B Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 4,
                                                   kTokenRefHint,
            {}, {}, {},
            SchemaOptionsSource::TokenKeyOptions),
            customize::property_schemas::makeField("outputKey", SchemaFieldType::String, "Output Key", true, QStringLiteral("result"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 5,
            kOutputKeyHint),
            customize::property_schemas::makeField("errorKey", SchemaFieldType::String, "Error Key", true, QStringLiteral("error"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 6,
            kErrorKeyHint),
            customize::property_schemas::makeField("a", SchemaFieldType::Number, "Fallback A", false, 0, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 20),
            customize::property_schemas::makeField("b", SchemaFieldType::Number, "Fallback B", false, 0, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 21)
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
