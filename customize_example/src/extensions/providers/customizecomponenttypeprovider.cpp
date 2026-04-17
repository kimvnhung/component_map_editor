#include "customizecomponenttypeprovider.h"

#include "extensions/runtime/templates/ComponentTypeTemplateAdapter.h"
#include "extensions/runtime/templates/TemplateProtoHelpers.h"
#include "provider_templates.pb.h"

namespace {

cme::templates::v1::ComponentTypeTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::ComponentTypeTemplateBundle bundle;
    bundle.set_provider_id("sample.workflow.componentTypes");
    bundle.set_schema_version("1.0.0");

    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeStart),
        QStringLiteral("Start"),
        QStringLiteral("control"),
        92.0,
        92.0,
        QStringLiteral("#66bb6a"),
        false,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeStop),
        QStringLiteral("Stop"),
        QStringLiteral("control"),
        92.0,
        92.0,
        QStringLiteral("#ef5350"),
        true,
        false);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeLoop),
        QStringLiteral("Loop Control"),
        QStringLiteral("control"),
        200.0,
        108.0,
        QStringLiteral("#ffa726"),
        true,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeIfElse),
        QStringLiteral("If/Else Control"),
        QStringLiteral("control"),
        200.0,
        108.0,
        QStringLiteral("#ffb300"),
        true,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeAdd),
        QStringLiteral("Add (a + b)"),
        QStringLiteral("math"),
        180.0,
        108.0,
        QStringLiteral("#7e57c2"),
        true,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeSubtract),
        QStringLiteral("Subtract (a - b)"),
        QStringLiteral("math"),
        180.0,
        108.0,
        QStringLiteral("#5c6bc0"),
        true,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeMultiply),
        QStringLiteral("Multiply (a * b)"),
        QStringLiteral("math"),
        180.0,
        108.0,
        QStringLiteral("#26a69a"),
        true,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeDivide),
        QStringLiteral("Divide (a / b)"),
        QStringLiteral("math"),
        180.0,
        108.0,
        QStringLiteral("#42a5f5"),
        true,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeErrorHandler),
        QStringLiteral("Error Handler"),
        QStringLiteral("system"),
        200.0,
        108.0,
        QStringLiteral("#e53935"),
        true,
        false);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeMod),
        QStringLiteral("Modulus (a % b)"),
        QStringLiteral("math"),
        180.0,
        108.0,
        QStringLiteral("#8d6e63"),
        true,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeLessThan),
        QStringLiteral("Less Than (a < b)"),
        QStringLiteral("math"),
        180.0,
        108.0,
        QStringLiteral("#00897b"),
        true,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeLessOrEqual),
        QStringLiteral("Less or Equal (a <= b)"),
        QStringLiteral("math"),
        180.0,
        108.0,
        QStringLiteral("#00796b"),
        true,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeEqual),
        QStringLiteral("Equal (a == b)"),
        QStringLiteral("math"),
        180.0,
        108.0,
        QStringLiteral("#00695c"),
        true,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeLogicAnd),
        QStringLiteral("Logical AND (a && b)"),
        QStringLiteral("logic"),
        180.0,
        108.0,
        QStringLiteral("#2baf2b"),
        true,
        true);



    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeStart),
        QVariantMap{{QStringLiteral("inputNumber"), 0}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeLoop),
        QVariantMap{{QStringLiteral("iterKey"), QStringLiteral("iter")},
                    {QStringLiteral("maxIterKey"), QStringLiteral("maxIter")},
                    {QStringLiteral("continueKey"), QStringLiteral("continueLoop")},
                    {QStringLiteral("conditionKey"), QStringLiteral("condition")},
                    {QStringLiteral("iter"), 0},
                    {QStringLiteral("maxIter"), 10},
                    {QStringLiteral("condition"), true}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeIfElse),
        QVariantMap{{QStringLiteral("conditionKey"), QStringLiteral("condition")},
                    {QStringLiteral("trueRouteKey"), QStringLiteral("routeTrue")},
                    {QStringLiteral("falseRouteKey"), QStringLiteral("routeFalse")},
                    {QStringLiteral("condition"), false}});

    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeAdd,
        QVariantMap{{QStringLiteral("outputKey"), QStringLiteral("sum")},
                    {QStringLiteral("errorKey"), QStringLiteral("error")}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeSubtract,
        QVariantMap{{QStringLiteral("outputKey"), QStringLiteral("difference")},
                    {QStringLiteral("errorKey"), QStringLiteral("error")}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeMultiply,
        QVariantMap{{QStringLiteral("outputKey"), QStringLiteral("product")},
                    {QStringLiteral("errorKey"), QStringLiteral("error")}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeDivide,
        QVariantMap{{QStringLiteral("outputKey"), QStringLiteral("quotient")},
                    {QStringLiteral("errorKey"), QStringLiteral("error")}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeMod,
        QVariantMap{{QStringLiteral("outputKey"), QStringLiteral("result")},
                    {QStringLiteral("errorKey"), QStringLiteral("error")}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeLessThan,
        QVariantMap{{QStringLiteral("outputKey"), QStringLiteral("result")},
                    {QStringLiteral("errorKey"), QStringLiteral("error")}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeLessOrEqual,
        QVariantMap{{QStringLiteral("outputKey"), QStringLiteral("result")},
                    {QStringLiteral("errorKey"), QStringLiteral("error")}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeEqual,
        QVariantMap{{QStringLiteral("outputKey"), QStringLiteral("result")},
                    {QStringLiteral("errorKey"), QStringLiteral("error")}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeErrorHandler,
        QVariantMap{{QStringLiteral("errorKey"), QStringLiteral("error")},
                    {QStringLiteral("message"), QStringLiteral("Unhandled workflow error.")}});

    return bundle;
}

const cme::templates::v1::ComponentTypeTemplateBundle &templateBundle()
{
    static const cme::templates::v1::ComponentTypeTemplateBundle kBundle = buildTemplateBundle();
    return kBundle;
}

} // namespace

QString CustomizeComponentTypeProvider::providerId() const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::providerId(templateBundle());
}

QStringList CustomizeComponentTypeProvider::componentTypeIds() const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::componentTypeIds(templateBundle());
}

QVariantMap CustomizeComponentTypeProvider::componentTypeDescriptor(const QString &componentTypeId) const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::componentTypeDescriptor(
        templateBundle(), componentTypeId);
}

QVariantMap CustomizeComponentTypeProvider::defaultComponentProperties(const QString &componentTypeId) const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::defaultComponentProperties(
        templateBundle(), componentTypeId);
}