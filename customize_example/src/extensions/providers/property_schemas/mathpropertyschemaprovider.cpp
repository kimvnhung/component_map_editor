#include "mathpropertyschemaprovider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include <utils/PropertySchemaHelper.h>

namespace
{

using cme::runtime::SchemaFieldSection;
using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;
using cme::runtime::SchemaOptionsSource;

const QString kTokenRefHint = QStringLiteral(
    "Choose an exact incoming token field reference. If no ref is selected, the fallback value below is used.");
const QString kOutputKeyHint = QStringLiteral("Result field written by this component.");
const QString kErrorKeyHint = QStringLiteral("Error field written when execution fails.");

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema.math");
    bundle.set_schema_version("1.0.0");

    cme::helper::property_schemas::addTarget(&bundle, "component/math/add",
                                             {
                                                 cme::helper::property_schemas::Field("title")
                                                     .type(SchemaFieldType::String)
                                                     .title("Title")
                                                     .defaultValue("Add Componnent")
                                                     .textField()
                                                     .section(SchemaFieldSection::General)
                                                     .order(1)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("inputARef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Input A Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(2)
                                                     .hint(kTokenRefHint)
                                                     .build(),

                                                 cme::helper::property_schemas::Field("inputBRef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Input B Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(4)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("outputKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("Output Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("sum"))
                                                     .textField()
                                                     .section(SchemaFieldSection::Context)
                                                     .order(5)
                                                     .hint(kOutputKeyHint)
                                                     .build(),

                                                 cme::helper::property_schemas::Field("errorKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("Error Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("error"))
                                                     .textField()
                                                     .section(SchemaFieldSection::Context)
                                                     .order(6)
                                                     .hint(kErrorKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("a")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback A")
                                                     .defaultValue(0)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(20)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("b")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback B")
                                                     .defaultValue(0)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(21)
                                                     .build(),
                                             });

    cme::helper::property_schemas::addTarget(&bundle, "component/math/subtract",
                                             {cme::helper::property_schemas::Field("title")
                                                  .type(SchemaFieldType::String)
                                                  .title("Title")
                                                  .defaultValue("Subtract Componnent")
                                                  .textField()
                                                  .section(SchemaFieldSection::General)
                                                  .order(1)
                                                  .build(),
                                              cme::helper::property_schemas::Field("inputARef")
                                                  .type(SchemaFieldType::String)
                                                  .title("Input A Ref")
                                                  .defaultValue(QString())
                                                  .dropdown(Options::tokenRef())
                                                  .section(SchemaFieldSection::Context)
                                                  .order(2)
                                                  .hint(kTokenRefHint)
                                                  .build(),
                                              cme::helper::property_schemas::Field("inputBRef")
                                                  .type(SchemaFieldType::String)
                                                  .title("Input B Ref")
                                                  .defaultValue(QString())
                                                  .dropdown(Options::tokenRef())
                                                  .section(SchemaFieldSection::Context)
                                                  .order(4)
                                                  .hint(kTokenRefHint)
                                                  .build(),
                                              cme::helper::property_schemas::Field("outputKey")
                                                  .type(SchemaFieldType::String)
                                                  .title("Output Key")
                                                  .required()
                                                  .defaultValue(QStringLiteral("difference"))
                                                  .textField()
                                                  .section(SchemaFieldSection::Context)
                                                  .order(5)
                                                  .hint(kOutputKeyHint)
                                                  .build(),
                                              cme::helper::property_schemas::Field("errorKey")
                                                  .type(SchemaFieldType::String)
                                                  .title("Error Key")
                                                  .required()
                                                  .defaultValue(QStringLiteral("error"))
                                                  .textField()
                                                  .section(SchemaFieldSection::Context)
                                                  .order(6)
                                                  .hint(kErrorKeyHint)
                                                  .build(),
                                              cme::helper::property_schemas::Field("a")
                                                  .type(SchemaFieldType::Number)
                                                  .title("Fallback A")
                                                  .defaultValue(0)
                                                  .spinBox()
                                                  .section(SchemaFieldSection::Fallback)
                                                  .order(20)
                                                  .build(),
                                              cme::helper::property_schemas::Field("b")
                                                  .type(SchemaFieldType::Number)
                                                  .title("Fallback B")
                                                  .defaultValue(0)
                                                  .spinBox()
                                                  .section(SchemaFieldSection::Fallback)
                                                  .order(21)
                                                  .build()});

    cme::helper::property_schemas::addTarget(&bundle, "component/math/multiply",
                                             {
                                                 cme::helper::property_schemas::Field("inputARef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Input A Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(2)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("inputBRef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Input B Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(4)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("outputKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("Output Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("product"))
                                                     .textField()
                                                     .section(SchemaFieldSection::Context)
                                                     .order(5)
                                                     .hint(kOutputKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("errorKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("Error Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("error"))
                                                     .textField()
                                                     .section(SchemaFieldSection::Context)
                                                     .order(6)
                                                     .hint(kErrorKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("a")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback A")
                                                     .defaultValue(1)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(20)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("b")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback B")
                                                     .defaultValue(1)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(21)
                                                     .build(),
                                             });

    cme::helper::property_schemas::addTarget(&bundle, "component/math/divide",
                                             {
                                                 cme::helper::property_schemas::Field("inputARef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Input A Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(2)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("inputBRef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Input B Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(4)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("outputKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("Output Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("quotient"))
                                                     .textField()
                                                     .section(SchemaFieldSection::Context)
                                                     .order(5)
                                                     .hint(kOutputKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("errorKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("Error Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("error"))
                                                     .textField()
                                                     .section(SchemaFieldSection::Context)
                                                     .order(6)
                                                     .hint(kErrorKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("a")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback A")
                                                     .defaultValue(1)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(20)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("b")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback B")
                                                     .defaultValue(1)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(21)
                                                     .build(),
                                             });

    cme::helper::property_schemas::addTarget(&bundle, "component/math/sqrt",
                                             {cme::helper::property_schemas::Field("inputRef")
                                                  .type(SchemaFieldType::String)
                                                  .title("Input Ref")
                                                  .defaultValue(QString())
                                                  .dropdown(Options::tokenRef())
                                                  .section(SchemaFieldSection::Context)
                                                  .order(2)
                                                  .hint(kTokenRefHint)
                                                  .build(),
                                              cme::helper::property_schemas::Field("outputKey")
                                                  .type(SchemaFieldType::String)
                                                  .title("Output Key")
                                                  .required()
                                                  .defaultValue(QStringLiteral("sqrtS"))
                                                  .textField()
                                                  .section(SchemaFieldSection::Context)
                                                  .order(5)
                                                  .hint(kOutputKeyHint)
                                                  .build(),
                                              cme::helper::property_schemas::Field("errorKey")
                                                  .type(SchemaFieldType::String)
                                                  .title("Error Key")
                                                  .required()
                                                  .defaultValue(QStringLiteral("error"))
                                                  .textField()
                                                  .section(SchemaFieldSection::Context)
                                                  .order(6)
                                                  .hint(kErrorKeyHint)
                                                  .build(),
                                              cme::helper::property_schemas::Field("S")
                                                  .type(SchemaFieldType::Number)
                                                  .title("Fallback S")
                                                  .defaultValue(0)
                                                  .spinBox()
                                                  .section(SchemaFieldSection::Fallback)
                                                  .order(20)
                                                  .build()});

    cme::helper::property_schemas::addTarget(&bundle, "component/math/mod",
                                             {
                                                 cme::helper::property_schemas::Field("inputARef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Input A Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(2)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("inputBRef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Input B Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(4)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("outputKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("Output Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("result"))
                                                     .textField()
                                                     .section(SchemaFieldSection::Context)
                                                     .order(5)
                                                     .hint(kOutputKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("errorKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("Error Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("error"))
                                                     .textField()
                                                     .section(SchemaFieldSection::Context)
                                                     .order(6)
                                                     .hint(kErrorKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("a")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback A")
                                                     .defaultValue(1)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(20)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("b")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback B")
                                                     .defaultValue(1)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(21)
                                                     .build(),
                                             });

    cme::helper::property_schemas::addTarget(&bundle, "component/math/less_than",
                                             {
                                                 cme::helper::property_schemas::Field("inputARef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Input A Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(2)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("inputBRef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Input B Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(4)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("outputKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("Output Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("result"))
                                                     .textField()
                                                     .section(SchemaFieldSection::Context)
                                                     .order(5)
                                                     .hint(kOutputKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("errorKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("Error Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("error"))
                                                     .textField()
                                                     .section(SchemaFieldSection::Context)
                                                     .order(6)
                                                     .hint(kErrorKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("a")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback A")
                                                     .defaultValue(0)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(20)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("b")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback B")
                                                     .defaultValue(0)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(21)
                                                     .build(),
                                             });

    cme::helper::property_schemas::addTarget(&bundle, "component/math/less_or_equal",
                                             {
                                                 cme::helper::property_schemas::Field("inputARef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Input A Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(2)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("inputBRef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Input B Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(4)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("outputKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("Output Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("result"))
                                                     .textField()
                                                     .section(SchemaFieldSection::Context)
                                                     .order(5)
                                                     .hint(kOutputKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("errorKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("Error Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("error"))
                                                     .textField()
                                                     .section(SchemaFieldSection::Context)
                                                     .order(6)
                                                     .hint(kErrorKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("a")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback A")
                                                     .defaultValue(0)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(20)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("b")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback B")
                                                     .defaultValue(0)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(21)
                                                     .build(),
                                             });

    cme::helper::property_schemas::addTarget(&bundle, "component/math/equal",
                                             {
                                                 cme::helper::property_schemas::Field("inputARef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Input A Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(2)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("inputBRef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Input B Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(4)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("outputKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("Output Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("result"))
                                                     .textField()
                                                     .section(SchemaFieldSection::Context)
                                                     .order(5)
                                                     .hint(kOutputKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("errorKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("Error Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("error"))
                                                     .textField()
                                                     .section(SchemaFieldSection::Context)
                                                     .order(6)
                                                     .hint(kErrorKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("a")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback A")
                                                     .defaultValue(0)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(20)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("b")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback B")
                                                     .defaultValue(0)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(21)
                                                     .build(),
                                             });

    return bundle;
}

const cme::templates::v1::PropertySchemaTemplateBundle& schemaBundle()
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

QVariantList MathPropertySchemaProvider::propertySchema(const QString& targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(schemaBundle(), targetId);
}
