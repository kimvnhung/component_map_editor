#ifndef COMPONENT_MAP_RUNTIME_RUNTIME_HANDLER_REGISTRY_H
#define COMPONENT_MAP_RUNTIME_RUNTIME_HANDLER_REGISTRY_H

#include "component_map_runtime/RuntimeTypes.h"

#include <functional>
#include <memory>
#include <utility>
#include <QHash>
#include <QStringList>

namespace cme::runtime
{

class RuntimeContext
{
public:
    RuntimeContext(QString componentId, QString componentType, QByteArray state,
                   bool *cancelled = nullptr)
        : m_componentId(std::move(componentId))
        , m_componentType(std::move(componentType))
        , m_state(std::move(state))
        , m_cancelled(cancelled)
    {
    }

    const QString &componentId() const { return m_componentId; }
    const QString &componentType() const { return m_componentType; }
    const QByteArray &state() const { return m_state; }
    void setState(const QByteArray &state) { m_state = state; }
    bool isCancelled() const { return m_cancelled && *m_cancelled; }
    void setFailure(const RuntimeFailure &failure) { m_failure = failure; }
    bool hasFailure() const { return m_failure.isValid(); }
    const RuntimeFailure &failure() const { return m_failure; }

private:
    QString m_componentId;
    QString m_componentType;
    QByteArray m_state;
    RuntimeFailure m_failure;
    bool *m_cancelled = nullptr;
};

class RuntimeEmitter
{
public:
    void emitEvent(const RuntimeEvent &event) { m_outputs.append(event); }
    void emitDelayed(const RuntimeEvent &event, qint64) { m_outputs.append(event); }
    const QList<RuntimeEvent> &outputs() const { return m_outputs; }

private:
    QList<RuntimeEvent> m_outputs;
};

class RuntimeHandler
{
public:
    virtual ~RuntimeHandler() = default;

    virtual QString componentType() const = 0;
    virtual bool process(const RuntimeEvent &event,
                         QByteArray &state,
                         RuntimeEvent *output,
                         RuntimeFailure *failure) = 0;

    virtual void onStart(RuntimeContext &) {}
    virtual void onEvent(const RuntimeEvent &event, RuntimeContext &context,
                         RuntimeEmitter &emitter)
    {
        QByteArray state = context.state();
        RuntimeEvent output;
        RuntimeFailure failure;
        if (process(event, state, &output, &failure))
        {
            context.setState(state);
            if (!output.eventId.isEmpty()) emitter.emitEvent(output);
        }
        else
        {
            context.setFailure(failure);
        }
    }
    virtual void onStop(RuntimeContext &) {}
};

struct RuntimeHandlerResult
{
    bool succeeded = true;
    RuntimeFailure failure;
    QList<RuntimeEvent> outputs;
};

inline RuntimeHandlerResult invokeHandler(RuntimeHandler &handler, const RuntimeEvent &event,
                                          RuntimeContext &context)
{
    RuntimeEmitter emitter;
    handler.onEvent(event, context, emitter);
    return {!context.hasFailure(), context.failure(), emitter.outputs()};
}

struct RuntimeComponentDefinition
{
    QString componentType;
    QString handlerId;
    QString version = QStringLiteral("1.0.0");
    int stateSchemaVersion = 1;
    QStringList inputMessageTypes;
    QStringList outputMessageTypes;
};

struct RuntimeExtensionPack
{
    QString extensionId;
    QString version = QStringLiteral("1.0.0");
    QList<RuntimeComponentDefinition> components;
    QHash<QString, std::function<std::unique_ptr<RuntimeHandler>()>> factories;
};

class RuntimeHandlerRegistry
{
public:
    using Factory = std::function<std::unique_ptr<RuntimeHandler>()>;

    bool registerFactory(const QString &componentType, Factory factory);
    bool registerExtension(const RuntimeExtensionPack &extension, QString *error = nullptr);
    bool contains(const QString &componentType) const;
    std::unique_ptr<RuntimeHandler> create(const QString &componentType) const;
    QStringList componentTypes() const;
    RuntimeComponentDefinition definition(const QString &componentType) const;
    QString extensionFor(const QString &componentType) const;

private:
    QHash<QString, Factory> m_factories;
    QHash<QString, RuntimeComponentDefinition> m_definitions;
    QHash<QString, QString> m_extensions;
};

RuntimeExtensionPack makeBuiltinFactoryExtension();

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_RUNTIME_HANDLER_REGISTRY_H
