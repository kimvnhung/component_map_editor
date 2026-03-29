#ifndef CUSTOMIZEEXECUTIONSEMANTICSPROVIDER_H
#define CUSTOMIZEEXECUTIONSEMANTICSPROVIDER_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

class CustomizeExecutionSemanticsProvider : public IExecutionSemanticsProvider
{
public:
    static constexpr const char *TypeAdd = "math/add";
    static constexpr const char *TypeSubtract = "math/subtract";
    static constexpr const char *TypeMultiply = "math/multiply";
    static constexpr const char *TypeDivide = "math/divide";
    static constexpr const char *TypeSqrtNewton = "workflow/sqrt_newton";
    static constexpr const char *TypeQuadratic = "workflow/quadratic";
    static constexpr const char *TypeErrorHandler = "system/error_handler";

    QString providerId() const override;
    QStringList supportedComponentTypes() const override;

    bool executeComponent(const QString &componentType,
                          const QString &componentId,
                          const QVariantMap &componentSnapshot,
                          const QVariantMap &inputState,
                          QVariantMap *outputState,
                          QVariantMap *trace,
                          QString *error) const override;

    bool executeComponentV2(const QString &componentType,
                            const QString &componentId,
                            const QVariantMap &componentSnapshot,
                            const cme::execution::IncomingTokens &incomingTokens,
                            cme::execution::ExecutionPayload *outputPayload,
                            QVariantMap *trace,
                            QString *error) const override;

private:
    struct SqrtResult {
        bool ok = false;
        double value = 0.0;
        int iterations = 0;
        double lastDelta = 0.0;
        QString error;
    };

    static QVariantMap mergeIncomingTokens(const cme::execution::IncomingTokens &incomingTokens);
    static double resolveNumber(const QVariantMap &context,
                                const QVariantMap &componentSnapshot,
                                const QString &contextKey,
                                const QString &fallbackSnapshotKey,
                                double fallbackValue,
                                bool *ok = nullptr);
    static QString resolveText(const QVariantMap &componentSnapshot,
                               const QString &key,
                               const QString &fallback);
    static QVariantMap makeTracePayload(const QString &componentType,
                                        const QString &componentId,
                                        const QVariantMap &inputs,
                                        const QVariantMap &outputs,
                                        const QString &errorText = QString());

    static bool executeBinary(const QString &opName,
                              const QString &componentId,
                              const QVariantMap &componentSnapshot,
                              const QVariantMap &context,
                              cme::execution::ExecutionPayload *outputPayload,
                              QVariantMap *trace,
                              QString *error);
    static SqrtResult computeSqrtNewton(double s, double epsilon, int maxIterations, double initialGuess);
};

#endif // CUSTOMIZEEXECUTIONSEMANTICSPROVIDER_H
