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

using PropertySchemaRegistryFactory = std::function<std::unique_ptr<PropertySchemaRegistry>()>;
using ExecutionSandboxFactory = std::function<std::unique_ptr<GraphExecutionSandbox>()>;

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

#endif // COMMON_H
