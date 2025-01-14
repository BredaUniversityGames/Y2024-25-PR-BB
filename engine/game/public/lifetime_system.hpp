#pragma once

#include "system_interface.hpp"

class LifetimeSystem final : public SystemInterface
{
public:
    LifetimeSystem() = default;
    ~LifetimeSystem() override = default;
    NON_COPYABLE(LifetimeSystem);
    NON_MOVABLE(LifetimeSystem);

    void Update(ECSModule& ecs, MAYBE_UNUSED float dt) override;
    void Render(MAYBE_UNUSED const ECSModule& ecs) const override { }
    void Inspect() override;

    std::string_view GetName() override { return "LifetimeSystem"; }
};