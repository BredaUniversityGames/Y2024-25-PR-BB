#include "systems/lifetime_system.hpp"

#include "ecs_module.hpp"
#include "systems/lifetime_component.hpp"

void LifetimeSystem::Update(ECSModule& ecs, float dt)
{
    const auto& lifetimeView = ecs.GetRegistry().view<LifetimeComponent>();

    for (const auto entity : lifetimeView)
    {
        LifetimeComponent& lifetime = ecs.GetRegistry().get<LifetimeComponent>(entity);

        lifetime.lifetime -= dt * !lifetime.paused;

        if (lifetime.lifetime <= 0.0f)
        {
            ecs.GetRegistry().destroy(entity);
        }
    }
}

void LifetimeSystem::Inspect()
{
}
