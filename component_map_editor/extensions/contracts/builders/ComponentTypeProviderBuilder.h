#ifndef COMPONENTTYPEPROVIDERBUILDER_H
#define COMPONENTTYPEPROVIDERBUILDER_H

#include <memory>
#include "utils/common.h"

class ComponentTypeProviderBuilder
{
public:
    ComponentTypeProviderBuilder();
    ~ComponentTypeProviderBuilder();

    ComponentTypeProviderBuilder &withComponentTypeProviderFactory(ComponentFactory f);

    ComponentFactory build() const;
private:
    class BuiltComponentTypeProvider;

    std::unique_ptr<BuiltComponentTypeProvider> m_builtProvider{nullptr};
};

#endif // COMPONENTTYPEPROVIDERBUILDER_H
