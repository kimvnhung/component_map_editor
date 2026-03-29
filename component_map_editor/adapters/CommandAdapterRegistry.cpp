// component_map_editor/adapters/CommandAdapterRegistry.cpp

#include "CommandAdapterRegistry.h"
#include "CommandAdapter.h"

#include <QDebug>

CommandAdapterRegistry::CommandAdapterRegistry()
{
}

CommandAdapterRegistry::~CommandAdapterRegistry()
{
    clear();
}

bool CommandAdapterRegistry::registerAdapter(const QString &commandType,
                                             std::unique_ptr<cme::adapter::CommandAdapter> adapter)
{
    if (commandType.isEmpty() || !adapter) {
        qWarning() << "CommandAdapterRegistry: cannot register with empty command type or null adapter";
        return false;
    }

    if (m_adapters.find(commandType) != m_adapters.end()) {
        qWarning() << "CommandAdapterRegistry: adapter for command type" << commandType << "is already registered";
        return false;
    }

    m_adapters.emplace(commandType, std::move(adapter));
    return true;
}

cme::adapter::CommandAdapter *CommandAdapterRegistry::adapterForCommand(const QString &commandType) const
{
    auto it = m_adapters.find(commandType);
    if (it != m_adapters.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool CommandAdapterRegistry::hasAdapter(const QString &commandType) const
{
    return m_adapters.find(commandType) != m_adapters.end();
}

QStringList CommandAdapterRegistry::registeredCommands() const
{
    QStringList result;
    for (const auto &pair : m_adapters) {
        result.append(pair.first);
    }
    return result;
}

void CommandAdapterRegistry::clear()
{
    m_adapters.clear();
}

void CommandAdapterRegistry::registerBuiltInAdapters(GraphModel *graph, UndoStack *undoStack)
{
    // TODO Phase 3++: Register adapters for:
    //   - addComponent
    //   - removeComponent
    //   - moveComponent
    //   - addConnection
    //   - removeConnection
    //   - setComponentProperty
    //   - setConnectionProperty
    //
    // For now: stub implementation
    (void)graph;
    (void)undoStack;
}
