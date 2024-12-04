#pragma once
#include "common.hpp"

class ECSModule;

class SystemInterface
{
public:
    virtual ~SystemInterface() = default;

    virtual void Update(MAYBE_UNUSED ECSModule& ecs, MAYBE_UNUSED float dt) {};
    virtual void Render(MAYBE_UNUSED const ECSModule& ecs) const {};
    virtual void Inspect() {};
};

template <typename T>
concept IsSystem = std::is_base_of<SystemInterface, T>::value;