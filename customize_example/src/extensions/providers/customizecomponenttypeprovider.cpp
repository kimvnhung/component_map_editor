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
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeCondition),
        QStringLiteral("Condition"),
        QStringLiteral("control"),
        164.0,
        100.0,
        QStringLiteral("#ffca28"),
        true,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeProcess),
        QStringLiteral("Process"),
        QStringLiteral("work"),
        164.0,
        100.0,
        QStringLiteral("#4fc3f7"),
        true,
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
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeSqrtNewton),
        QStringLiteral("Square Root (Newton)"),
        QStringLiteral("workflow"),
        220.0,
        120.0,
        QStringLiteral("#ef6c00"),
        true,
        true);
    *bundle.add_component_types() = cme::runtime::templates::makeComponentTypeTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeQuadratic),
        QStringLiteral("Quadratic Solver"),
        QStringLiteral("workflow"),
        220.0,
        120.0,
        QStringLiteral("#8d6e63"),
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

    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        QString::fromLatin1(CustomizeComponentTypeProvider::TypeStart),
        QVariantMap{{QStringLiteral("inputNumber"), 0}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeProcess,
        QVariantMap{{QStringLiteral("addValue"), 9}, {QStringLiteral("description"), QString()}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeCondition,
        QVariantMap{{QStringLiteral("condition"), QString()}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeAdd,
        QVariantMap{{QStringLiteral("inputAKey"), QStringLiteral("a")},
                    {QStringLiteral("inputBKey"), QStringLiteral("b")},
                    {QStringLiteral("outputKey"), QStringLiteral("sum")},
                    {QStringLiteral("errorKey"), QStringLiteral("error")}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeSubtract,
        QVariantMap{{QStringLiteral("inputAKey"), QStringLiteral("a")},
                    {QStringLiteral("inputBKey"), QStringLiteral("b")},
                    {QStringLiteral("outputKey"), QStringLiteral("difference")},
                    {QStringLiteral("errorKey"), QStringLiteral("error")}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeMultiply,
        QVariantMap{{QStringLiteral("inputAKey"), QStringLiteral("a")},
                    {QStringLiteral("inputBKey"), QStringLiteral("b")},
                    {QStringLiteral("outputKey"), QStringLiteral("product")},
                    {QStringLiteral("errorKey"), QStringLiteral("error")}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeDivide,
        QVariantMap{{QStringLiteral("inputAKey"), QStringLiteral("a")},
                    {QStringLiteral("inputBKey"), QStringLiteral("b")},
                    {QStringLiteral("outputKey"), QStringLiteral("quotient")},
                    {QStringLiteral("errorKey"), QStringLiteral("error")}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeSqrtNewton,
        QVariantMap{{QStringLiteral("sKey"), QStringLiteral("S")},
                    {QStringLiteral("epsilonKey"), QStringLiteral("epsilon")},
                    {QStringLiteral("outputKey"), QStringLiteral("sqrt")},
                    {QStringLiteral("initialGuessKey"), QStringLiteral("initialGuess")},
                    {QStringLiteral("maxIterations"), 50},
                    {QStringLiteral("errorKey"), QStringLiteral("error")}});
    *bundle.add_defaults() = cme::runtime::templates::makeComponentTypeDefaultsTemplate(
        CustomizeComponentTypeProvider::TypeQuadratic,
        QVariantMap{{QStringLiteral("aKey"), QStringLiteral("a")},
                    {QStringLiteral("bKey"), QStringLiteral("b")},
                    {QStringLiteral("cKey"), QStringLiteral("c")},
                    {QStringLiteral("deltaKey"), QStringLiteral("delta")},
                    {QStringLiteral("x1Key"), QStringLiteral("x1")},
                    {QStringLiteral("x2Key"), QStringLiteral("x2")},
                    {QStringLiteral("statusKey"), QStringLiteral("quadratic.status")},
                    {QStringLiteral("errorKey"), QStringLiteral("error")},
                    {QStringLiteral("epsilonKey"), QStringLiteral("epsilon")}});
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