#include "customizeexecutionsanticsprovider.h"

#include <QDebug>
#include <QJsonDocument>

#include <cmath>

QString CustomizeExecutionSemanticsProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution");
}

QStringList CustomizeExecutionSemanticsProvider::supportedComponentTypes() const
{
    return {
        QString::fromLatin1(TypeAdd),
        QString::fromLatin1(TypeSubtract),
        QString::fromLatin1(TypeMultiply),
        QString::fromLatin1(TypeDivide),
        QString::fromLatin1(TypeSqrtNewton),
        QString::fromLatin1(TypeQuadratic),
        QString::fromLatin1(TypeErrorHandler)
    };
}

bool CustomizeExecutionSemanticsProvider::executeComponent(const QString &componentType,
                                                           const QString &componentId,
                                                           const QVariantMap &componentSnapshot,
                                                           const QVariantMap &inputState,
                                                           QVariantMap *outputState,
                                                           QVariantMap *trace,
                                                           QString *error) const
{
    cme::execution::IncomingTokens incomingTokens;
    incomingTokens.insert(QStringLiteral("__legacy_global_state__"), inputState);
    return executeComponentV2(componentType,
                              componentId,
                              componentSnapshot,
                              incomingTokens,
                              outputState,
                              trace,
                              error);
}

bool CustomizeExecutionSemanticsProvider::executeComponentV2(
    const QString &componentType,
    const QString &componentId,
    const QVariantMap &componentSnapshot,
    const cme::execution::IncomingTokens &incomingTokens,
    cme::execution::ExecutionPayload *outputPayload,
    QVariantMap *trace,
    QString *error) const
{
    const QVariantMap context = mergeIncomingTokens(incomingTokens);

    if (componentType == QLatin1String(TypeAdd)
        || componentType == QLatin1String(TypeSubtract)
        || componentType == QLatin1String(TypeMultiply)
        || componentType == QLatin1String(TypeDivide)) {
        return executeBinary(componentType, componentId, componentSnapshot, context, outputPayload, trace, error);
    }

    if (componentType == QLatin1String(TypeSqrtNewton)) {
        QVariantMap out = context;
        const QString sKey = resolveText(componentSnapshot, QStringLiteral("sKey"), QStringLiteral("S"));
        const QString epsilonKey = resolveText(componentSnapshot, QStringLiteral("epsilonKey"), QStringLiteral("epsilon"));
        const QString outputKey = resolveText(componentSnapshot, QStringLiteral("outputKey"), QStringLiteral("sqrt"));
        const QString errorKey = resolveText(componentSnapshot, QStringLiteral("errorKey"), QStringLiteral("error"));
        const QString initialGuessKey = resolveText(componentSnapshot,
                                                    QStringLiteral("initialGuessKey"),
                                                    QStringLiteral("initialGuess"));

        bool okS = false;
        bool okEpsilon = false;
        bool okGuess = false;
        bool okMaxIterations = false;

        const double s = resolveNumber(context, componentSnapshot, sKey, QStringLiteral("S"), 0.0, &okS);
        const double epsilon = resolveNumber(context, componentSnapshot, epsilonKey, QStringLiteral("epsilon"), 1e-6, &okEpsilon);
        const double initialGuess = resolveNumber(context,
                                                  componentSnapshot,
                                                  initialGuessKey,
                                                  QStringLiteral("initialGuess"),
                                                  s > 1.0 ? s / 2.0 : 1.0,
                                                  &okGuess);
        const int maxIterations = componentSnapshot.value(QStringLiteral("maxIterations"), 50).toInt(&okMaxIterations);

        Q_UNUSED(okGuess)

        if (!okS || !okEpsilon || !okMaxIterations || maxIterations <= 0 || epsilon <= 0.0) {
            const QString msg = QStringLiteral("Invalid sqrt_newton inputs (S/epsilon/maxIterations).");
            out.insert(errorKey, msg);
            if (outputPayload)
                *outputPayload = out;
            if (trace)
                *trace = makeTracePayload(componentType, componentId, context, out, msg);
            if (error)
                *error = msg;
            qWarning().noquote() << QStringLiteral("[Trace][%1] %2 error=%3")
                                        .arg(componentType, componentId, msg);
            return false;
        }

        const SqrtResult sqrtResult = computeSqrtNewton(s, epsilon, maxIterations, initialGuess);
        if (!sqrtResult.ok) {
            out.insert(errorKey, sqrtResult.error);
            if (outputPayload)
                *outputPayload = out;
            if (trace)
                *trace = makeTracePayload(componentType, componentId, context, out, sqrtResult.error);
            if (error)
                *error = sqrtResult.error;
            qWarning().noquote() << QStringLiteral("[Trace][%1] %2 error=%3")
                                        .arg(componentType, componentId, sqrtResult.error);
            return false;
        }

        out.insert(outputKey, sqrtResult.value);
        out.insert(QStringLiteral("sqrt.iterations"), sqrtResult.iterations);
        out.insert(QStringLiteral("sqrt.lastDelta"), sqrtResult.lastDelta);
        out.remove(errorKey);

        if (outputPayload)
            *outputPayload = out;
        if (trace)
            *trace = makeTracePayload(componentType, componentId, context, out);

        qInfo().noquote() << QStringLiteral("[Trace][%1] %2 input=%3 output=%4")
                                 .arg(componentType, componentId,
                                      QString::fromUtf8(QJsonDocument::fromVariant(context).toJson(QJsonDocument::Compact)),
                                      QString::fromUtf8(QJsonDocument::fromVariant(out).toJson(QJsonDocument::Compact)));
        return true;
    }

    if (componentType == QLatin1String(TypeQuadratic)) {
        QVariantMap out = context;
        const QString aKey = resolveText(componentSnapshot, QStringLiteral("aKey"), QStringLiteral("a"));
        const QString bKey = resolveText(componentSnapshot, QStringLiteral("bKey"), QStringLiteral("b"));
        const QString cKey = resolveText(componentSnapshot, QStringLiteral("cKey"), QStringLiteral("c"));
        const QString x1Key = resolveText(componentSnapshot, QStringLiteral("x1Key"), QStringLiteral("x1"));
        const QString x2Key = resolveText(componentSnapshot, QStringLiteral("x2Key"), QStringLiteral("x2"));
        const QString deltaKey = resolveText(componentSnapshot, QStringLiteral("deltaKey"), QStringLiteral("delta"));
        const QString statusKey = resolveText(componentSnapshot, QStringLiteral("statusKey"), QStringLiteral("quadratic.status"));
        const QString errorKey = resolveText(componentSnapshot, QStringLiteral("errorKey"), QStringLiteral("error"));
        const QString epsilonKey = resolveText(componentSnapshot, QStringLiteral("epsilonKey"), QStringLiteral("epsilon"));

        bool okA = false;
        bool okB = false;
        bool okC = false;
        const double a = resolveNumber(context, componentSnapshot, aKey, QStringLiteral("a"), 0.0, &okA);
        const double b = resolveNumber(context, componentSnapshot, bKey, QStringLiteral("b"), 0.0, &okB);
        const double c = resolveNumber(context, componentSnapshot, cKey, QStringLiteral("c"), 0.0, &okC);

        if (!okA || !okB || !okC || std::abs(a) < 1e-12) {
            const QString msg = QStringLiteral("Invalid quadratic coefficients: 'a' must be non-zero.");
            out.insert(errorKey, msg);
            out.insert(statusKey, QStringLiteral("error"));
            if (outputPayload)
                *outputPayload = out;
            if (trace)
                *trace = makeTracePayload(componentType, componentId, context, out, msg);
            if (error)
                *error = msg;
            qWarning().noquote() << QStringLiteral("[Trace][%1] %2 error=%3")
                                        .arg(componentType, componentId, msg);
            return false;
        }

        const double delta = b * b - 4.0 * a * c;
        out.insert(deltaKey, delta);

        const double epsilon = resolveNumber(context, componentSnapshot, epsilonKey, QStringLiteral("epsilon"), 1e-6);
        if (delta < 0.0) {
            const QString msg = QStringLiteral("No real roots: delta < 0.");
            out.insert(statusKey, QStringLiteral("no_real_root"));
            out.insert(errorKey, msg);
            if (outputPayload)
                *outputPayload = out;
            if (trace)
                *trace = makeTracePayload(componentType, componentId, context, out, msg);
            if (error)
                *error = msg;
            qWarning().noquote() << QStringLiteral("[Trace][%1] %2 error=%3")
                                        .arg(componentType, componentId, msg);
            return false;
        }

        if (std::abs(delta) <= epsilon) {
            const double x = (-b) / (2.0 * a);
            out.insert(x1Key, x);
            out.insert(x2Key, x);
            out.insert(statusKey, QStringLiteral("one_root"));
            out.remove(errorKey);
        } else {
            const SqrtResult sqrtResult = computeSqrtNewton(delta, epsilon, 64, delta > 1.0 ? delta / 2.0 : 1.0);
            if (!sqrtResult.ok) {
                out.insert(statusKey, QStringLiteral("error"));
                out.insert(errorKey, sqrtResult.error);
                if (outputPayload)
                    *outputPayload = out;
                if (trace)
                    *trace = makeTracePayload(componentType, componentId, context, out, sqrtResult.error);
                if (error)
                    *error = sqrtResult.error;
                qWarning().noquote() << QStringLiteral("[Trace][%1] %2 error=%3")
                                            .arg(componentType, componentId, sqrtResult.error);
                return false;
            }

            const double sqrtDelta = sqrtResult.value;
            const double denom = 2.0 * a;
            out.insert(x1Key, (-b + sqrtDelta) / denom);
            out.insert(x2Key, (-b - sqrtDelta) / denom);
            out.insert(QStringLiteral("sqrtDelta"), sqrtDelta);
            out.insert(statusKey, QStringLiteral("two_roots"));
            out.remove(errorKey);
        }

        if (outputPayload)
            *outputPayload = out;
        if (trace)
            *trace = makeTracePayload(componentType, componentId, context, out);

        qInfo().noquote() << QStringLiteral("[Trace][%1] %2 input=%3 output=%4")
                                 .arg(componentType, componentId,
                                      QString::fromUtf8(QJsonDocument::fromVariant(context).toJson(QJsonDocument::Compact)),
                                      QString::fromUtf8(QJsonDocument::fromVariant(out).toJson(QJsonDocument::Compact)));
        return true;
    }

    if (componentType == QLatin1String(TypeErrorHandler)) {
        QVariantMap out = context;
        const QString errorKey = resolveText(componentSnapshot, QStringLiteral("errorKey"), QStringLiteral("error"));
        if (!out.contains(errorKey))
            out.insert(errorKey, resolveText(componentSnapshot, QStringLiteral("message"), QStringLiteral("Unhandled workflow error.")));

        if (outputPayload)
            *outputPayload = out;
        if (trace)
            *trace = makeTracePayload(componentType, componentId, context, out);

        qWarning().noquote() << QStringLiteral("[Trace][%1] %2 handled=%3")
                                    .arg(componentType, componentId, out.value(errorKey).toString());
        return true;
    }

    QVariantMap passthrough = context;
    if (outputPayload)
        *outputPayload = passthrough;
    if (trace)
        *trace = makeTracePayload(componentType, componentId, context, passthrough);
    return true;
}

QVariantMap CustomizeExecutionSemanticsProvider::mergeIncomingTokens(
    const cme::execution::IncomingTokens &incomingTokens)
{
    QVariantMap merged;
    QStringList tokenKeys = incomingTokens.keys();
    std::sort(tokenKeys.begin(), tokenKeys.end());
    for (const QString &tokenKey : tokenKeys)
        merged.insert(incomingTokens.value(tokenKey));
    return merged;
}

double CustomizeExecutionSemanticsProvider::resolveNumber(const QVariantMap &context,
                                                          const QVariantMap &componentSnapshot,
                                                          const QString &contextKey,
                                                          const QString &fallbackSnapshotKey,
                                                          double fallbackValue,
                                                          bool *ok)
{
    bool parsed = false;
    double value = context.value(contextKey).toDouble(&parsed);
    if (!parsed)
        value = componentSnapshot.value(fallbackSnapshotKey, fallbackValue).toDouble(&parsed);
    if (!parsed)
        value = fallbackValue;
    if (ok)
        *ok = parsed;
    return value;
}

QString CustomizeExecutionSemanticsProvider::resolveText(const QVariantMap &componentSnapshot,
                                                         const QString &key,
                                                         const QString &fallback)
{
    const QString text = componentSnapshot.value(key).toString().trimmed();
    return text.isEmpty() ? fallback : text;
}

QVariantMap CustomizeExecutionSemanticsProvider::makeTracePayload(const QString &componentType,
                                                                  const QString &componentId,
                                                                  const QVariantMap &inputs,
                                                                  const QVariantMap &outputs,
                                                                  const QString &errorText)
{
    QVariantMap trace{
        { QStringLiteral("componentType"), componentType },
        { QStringLiteral("componentId"), componentId },
        { QStringLiteral("inputs"), inputs },
        { QStringLiteral("outputs"), outputs }
    };

    if (!errorText.isEmpty())
        trace.insert(QStringLiteral("error"), errorText);
    return trace;
}

bool CustomizeExecutionSemanticsProvider::executeBinary(
    const QString &opName,
    const QString &componentId,
    const QVariantMap &componentSnapshot,
    const QVariantMap &context,
    cme::execution::ExecutionPayload *outputPayload,
    QVariantMap *trace,
    QString *error)
{
    QVariantMap out = context;

    const QString inputAKey = resolveText(componentSnapshot, QStringLiteral("inputAKey"), QStringLiteral("a"));
    const QString inputBKey = resolveText(componentSnapshot, QStringLiteral("inputBKey"), QStringLiteral("b"));
    const QString outputKey = resolveText(componentSnapshot, QStringLiteral("outputKey"), QStringLiteral("result"));
    const QString errorKey = resolveText(componentSnapshot, QStringLiteral("errorKey"), QStringLiteral("error"));

    bool okA = false;
    bool okB = false;
    const double a = resolveNumber(context, componentSnapshot, inputAKey, QStringLiteral("a"), 0.0, &okA);
    const double b = resolveNumber(context, componentSnapshot, inputBKey, QStringLiteral("b"), 0.0, &okB);

    if (!okA || !okB) {
        const QString msg = QStringLiteral("Invalid numeric input for operation '%1'.").arg(opName);
        out.insert(errorKey, msg);
        if (outputPayload)
            *outputPayload = out;
        if (trace)
            *trace = makeTracePayload(opName, componentId, context, out, msg);
        if (error)
            *error = msg;
        qWarning().noquote() << QStringLiteral("[Trace][%1] %2 error=%3")
                                    .arg(opName, componentId, msg);
        return false;
    }

    double value = 0.0;
    if (opName == QLatin1String(TypeAdd))
        value = a + b;
    else if (opName == QLatin1String(TypeSubtract))
        value = a - b;
    else if (opName == QLatin1String(TypeMultiply))
        value = a * b;
    else if (opName == QLatin1String(TypeDivide)) {
        if (std::abs(b) < 1e-12) {
            const QString msg = QStringLiteral("Division by zero.");
            out.insert(errorKey, msg);
            if (outputPayload)
                *outputPayload = out;
            if (trace)
                *trace = makeTracePayload(opName, componentId, context, out, msg);
            if (error)
                *error = msg;
            qWarning().noquote() << QStringLiteral("[Trace][%1] %2 error=%3")
                                        .arg(opName, componentId, msg);
            return false;
        }
        value = a / b;
    }

    out.insert(outputKey, value);
    out.remove(errorKey);

    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = makeTracePayload(opName, componentId, context, out);

    qInfo().noquote() << QStringLiteral("[Trace][%1] %2 a=%3 b=%4 result=%5")
                             .arg(opName, componentId)
                             .arg(a)
                             .arg(b)
                             .arg(value);

    return true;
}

CustomizeExecutionSemanticsProvider::SqrtResult
CustomizeExecutionSemanticsProvider::computeSqrtNewton(double s,
                                                       double epsilon,
                                                       int maxIterations,
                                                       double initialGuess)
{
    SqrtResult result;
    if (s < 0.0) {
        result.error = QStringLiteral("Square root of negative number is not supported.");
        return result;
    }

    if (s == 0.0) {
        result.ok = true;
        result.value = 0.0;
        return result;
    }

    double xOld = initialGuess;
    if (xOld <= 0.0)
        xOld = s > 1.0 ? s / 2.0 : 1.0;

    for (int i = 0; i < maxIterations; ++i) {
        if (std::abs(xOld) < 1e-12) {
            result.error = QStringLiteral("Newton iteration encountered zero denominator.");
            return result;
        }

        const double xNew = (xOld + (s / xOld)) / 2.0;
        const double delta = std::abs(xNew - xOld);
        result.iterations = i + 1;
        result.lastDelta = delta;

        if (delta < epsilon) {
            result.ok = true;
            result.value = xNew;
            return result;
        }

        xOld = xNew;
    }

    result.ok = true;
    result.value = xOld;
    return result;
}
