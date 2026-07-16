#ifndef COMMON_H
#define COMMON_H

#include "extensions/contracts/IComponentTypeProvider.h"
#include "extensions/contracts/IPropertySchemaProvider.h"
#include "extensions/contracts/IExecutionSemanticsProvider.h"
#include "extensions/runtime/PropertySchemaRegistry.h"
#include "services/GraphExecutionSandbox.h"
#include "extensions/contracts/IExtensionPack.h"


using PackFactory = std::function<std::unique_ptr<IExtensionPack>()>;
using ComponentFactory = std::function<std::unique_ptr<IComponentTypeProvider>()>;
using PropertySchemaFactory = std::function<std::unique_ptr<IPropertySchemaProvider>()>;
using ExecutionSemanticsFactory = std::function<std::unique_ptr<IExecutionSemanticsProvider>()>;
using ConnectionPolicyFactory = std::function<std::unique_ptr<IConnectionPolicyProvider>()>;
using ValidationFactory = std::function<std::unique_ptr<IValidationProvider>()>;
using ActionFactory = std::function<std::unique_ptr<IActionProvider>()>;

using PropertySchemaRegistryFactory = std::function<std::unique_ptr<PropertySchemaRegistry>()>;
using ExecutionSandboxFactory = std::function<std::unique_ptr<GraphExecutionSandbox>()>;

struct PackFactoryEntry
{
    QString extensionId;
    PackFactory factory;
};

namespace utils
{
    template<typename T, typename... Args>
    std::function<std::unique_ptr<T>()> makeFactory(Args&&... args)
    {
        auto params = std::make_tuple(std::forward<Args>(args)...);
        auto paramsPtr = std::make_shared<decltype(params)>(std::move(params));

        return [paramsPtr]() mutable -> std::unique_ptr<T>
        {
            return std::apply(
                [](auto&&... args)
            {
                return std::make_unique<T>(
                    std::move(args)...);
            },
            std::move(*paramsPtr));
        };
    }

} // namespace utils


class ComponentMissingException : public std::runtime_error
{
public:
    explicit ComponentMissingException(const std::string &message)
        : std::runtime_error(message) {}
};

class PackFactoryMissingException : public std::runtime_error
{
public:
    explicit PackFactoryMissingException()
        : std::runtime_error("PackFactory is missing. Please provide a valid PackFactory to build the ComponentMapEditorManager.") {}
};

class ExtensionContractRegistryMissingException : public std::runtime_error
{
public:
    explicit ExtensionContractRegistryMissingException()
        : std::runtime_error("ExtensionContractRegistry is missing. Please provide a valid ExtensionContractRegistry to build the ComponentMapEditorManager.") {}
};

class InvalidRuleFileException : public std::runtime_error
{
public:
    explicit InvalidRuleFileException(const std::string &path)
        : std::runtime_error("Invalid rule file: " + path) {}
};

class InvalidManifestDirectoryException : public std::runtime_error
{
public:
    explicit InvalidManifestDirectoryException(const std::string &path)
        : std::runtime_error("Invalid manifest directory: " + path) {}
};

#endif // COMMON_H
