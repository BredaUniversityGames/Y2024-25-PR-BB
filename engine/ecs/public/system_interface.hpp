#pragma once
#include "common.hpp"
#include <type_traits>

class ECSModule;

class SystemInterface
{
public:
    SystemInterface() = default;
    virtual ~SystemInterface() = default;

    NON_MOVABLE(SystemInterface);
    NON_COPYABLE(SystemInterface);

    virtual void Update(MAYBE_UNUSED ECSModule& ecs, MAYBE_UNUSED float dt) {};
    virtual void Render(MAYBE_UNUSED const ECSModule& ecs) const {};
    virtual void Inspect() {};
};

template <typename T>
concept IsSystem = std::is_base_of<SystemInterface, T>::value;