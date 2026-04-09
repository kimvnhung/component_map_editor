#ifndef CUSTOMIZEEXECUTIONSEMANTICSPROVIDER_H
#define CUSTOMIZEEXECUTIONSEMANTICSPROVIDER_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

#include "executors/customizestartexecutionprovider.h"
#include "executors/customizestopexecutionprovider.h"
#include "executors/customizeaddexecutionprovider.h"
#include "executors/customizedivideexecutionprovider.h"
#include "executors/customizeerrorhandlerexecutionprovider.h"
#include "executors/customizeifelseexecutionprovider.h"
#include "executors/customizeloopexecutionprovider.h"
#include "executors/customizemultiplyexecutionprovider.h"
#include "executors/customizesubtractexecutionprovider.h"

#include "executors/customizelogicandexecutionprovider.h"
#include "executors/customizelessorequalexecutionprovider.h"
#include "executors/customizeequalexecutionprovider.h"
#include "executors/customizemodexecutionprovider.h"
#include "executors/customizelessthanexecutionprovider.h"

class CustomizeExecutionSemanticsProvider : public IExecutionSemanticsProvider
{
public:
    static constexpr const char *TypeStart = CustomizeStartExecutionProvider::TypeId;
    static constexpr const char *TypeStop = CustomizeStopExecutionProvider::TypeId;
    static constexpr const char *TypeLoop = CustomizeLoopExecutionProvider::TypeId;
    static constexpr const char *TypeIfElse = CustomizeIfElseExecutionProvider::TypeId;
    static constexpr const char *TypeAdd = CustomizeAddExecutionProvider::TypeId;
    static constexpr const char *TypeSubtract = CustomizeSubtractExecutionProvider::TypeId;
    static constexpr const char *TypeMultiply = CustomizeMultiplyExecutionProvider::TypeId;
    static constexpr const char *TypeDivide = CustomizeDivideExecutionProvider::TypeId;
    static constexpr const char *TypeErrorHandler = CustomizeErrorHandlerExecutionProvider::TypeId;
    static constexpr const char *TypeMod = CustomizeModExecutionProvider::TypeId;
    static constexpr const char *TypeLessThan = CustomizeLessThanExecutionProvider::TypeId;
    static constexpr const char *TypeLessOrEqual = CustomizeLessOrEqualExecutionProvider::TypeId;
    static constexpr const char *TypeEqual = CustomizeEqualExecutionProvider::TypeId;
    static constexpr const char *TypeLogicAnd = CustomizeLogicAndExecutionProvider::TypeId;


    QString providerId() const override;
    QStringList supportedComponentTypes() const override;

    bool executeComponent(const QString &componentType,
                            const QString &componentId,
                            const QVariantMap &componentSnapshot,
                            const cme::execution::IncomingTokens &incomingTokens,
                            cme::execution::ExecutionPayload *outputPayload,
                            QVariantMap *trace,
                            QString *error) const override;

private:
    const IExecutionSemanticsProvider *providerForType(const QString &componentType) const;

    CustomizeStartExecutionProvider m_startProvider;
    CustomizeStopExecutionProvider m_stopProvider;
    CustomizeAddExecutionProvider m_addProvider;
    CustomizeSubtractExecutionProvider m_subtractProvider;
    CustomizeMultiplyExecutionProvider m_multiplyProvider;
    CustomizeDivideExecutionProvider m_divideProvider;
    CustomizeIfElseExecutionProvider m_ifElseProvider;
    CustomizeLoopExecutionProvider m_loopProvider;
    CustomizeErrorHandlerExecutionProvider m_errorHandlerProvider;

    CustomizeLessThanExecutionProvider m_lessThanProvider;
    CustomizeLessOrEqualExecutionProvider m_lessOrEqualProvider;
    CustomizeEqualExecutionProvider m_equalProvider;
    CustomizeLogicAndExecutionProvider m_logicAndProvider;
    CustomizeModExecutionProvider m_modProvider;
};

#endif // CUSTOMIZEEXECUTIONSEMANTICSPROVIDER_H
