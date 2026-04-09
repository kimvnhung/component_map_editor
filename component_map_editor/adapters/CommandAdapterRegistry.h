// component_map_editor/adapters/CommandAdapterRegistry.h
//
// Registry for command adapters enabling extensible command handling.
// 
// Purpose:
//   - Maintains a map of command types to their handler adapters
//   - Enables third-party extensions to register custom command handlers
//   - Decouples CommandGateway from specific command implementations
//   - Supports both TypedCommandHandler and LegacyCommandAdapter patterns
//
// Design:
//   - Single-threaded; use from main thread only
//   - Adapters are registered at startup; registration is not thread-safe
//   - Lookup is O(1) via hash map
//   - Ownership: CommandAdapterRegistry owns all registered adapters

#pragma once

#include <QString>
#include <QVariantMap>
#include <map>
#include <memory>

#include "command.pb.h"
#include "CommandAdapter.h"

class GraphModel;
class UndoStack;

// ── Extension point for custom command handlers ──────────────────────────────

class CommandAdapterRegistry
{
public:
    explicit CommandAdapterRegistry();
    ~CommandAdapterRegistry();

    /// Register a command adapter for a specific command type.
    /// Ownership transfers to the registry.
    /// Returns false if a handler was already registered for this command type.
    bool registerAdapter(const QString &commandType,
                         std::unique_ptr<cme::adapter::CommandAdapter> adapter);

    /// Look up a registered adapter by command type.
    /// Returns nullptr if no adapter is registered.
    cme::adapter::CommandAdapter *adapterForCommand(const QString &commandType) const;

    /// Check if a command type has a registered adapter.
    bool hasAdapter(const QString &commandType) const;

    /// Get the list of registered command types.
    QStringList registeredCommands() const;

    /// Clear all registered adapters.
    void clear();

    /// For Phase 3+: batch-register built-in command adapters.
    /// Registers adapters for all core command types.
    void registerBuiltInAdapters(GraphModel *graph, UndoStack *undoStack);

private:
    std::map<QString, std::unique_ptr<cme::adapter::CommandAdapter>> m_adapters;
};
