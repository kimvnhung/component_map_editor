#include "component_map_runtime/BuiltinFactoryExtension.h"

namespace cme::runtime
{
namespace
{
class FactoryHandler final : public RuntimeHandler
{
public:
    explicit FactoryHandler(QString type) : m_type(std::move(type)) {}
    QString componentType() const override { return m_type; }
    bool process(const RuntimeEvent &event, QByteArray &state,
                 RuntimeEvent *output, RuntimeFailure *) override
    {
        state.append(event.payload);
        if (output)
        {
            *output = event;
            output->sourceComponentId = event.targetComponentId;
        }
        return true;
    }

private:
    QString m_type;
};

RuntimeComponentDefinition definition(const QString &type, const QString &handler)
{
    RuntimeComponentDefinition result;
    result.componentType = type;
    result.handlerId = handler;
    result.inputMessageTypes = {QStringLiteral("test.event")};
    result.outputMessageTypes = {QStringLiteral("test.event")};
    return result;
}
}

RuntimeExtensionPack makeBuiltinFactoryExtension()
{
    RuntimeExtensionPack extension;
    extension.extensionId = QStringLiteral("component-map.factory");
    extension.version = QStringLiteral("1.0.0");
    for (const auto &entry : {
             std::pair{QStringLiteral("factory/manager"), QStringLiteral("factory.manager")},
             std::pair{QStringLiteral("factory/fruit_producer"), QStringLiteral("factory.fruit_producer")},
             std::pair{QStringLiteral("factory/store"), QStringLiteral("factory.store")},
             std::pair{QStringLiteral("factory/wash_machine"), QStringLiteral("factory.wash_machine")},
             std::pair{QStringLiteral("factory/seller"), QStringLiteral("factory.seller")},
             // Compatibility fixture types used by the process-boundary vertical slice.
             std::pair{QStringLiteral("producer"), QStringLiteral("compat.producer")},
             std::pair{QStringLiteral("processor"), QStringLiteral("compat.processor")},
             std::pair{QStringLiteral("sink"), QStringLiteral("compat.sink")}})
    {
        extension.components.append(definition(entry.first, entry.second));
        extension.factories.insert(entry.first, [type = entry.first] {
            return std::make_unique<FactoryHandler>(type);
        });
    }
    return extension;
}

} // namespace cme::runtime
