#include "systems/test_system.hpp"

#include "ECS.hpp"
#include "components/test_component.hpp"

void TestSystem::Update(ECS& ecs, MAYBE_UNUSED float dt)
{
    const auto view = ecs._registry.view<TestComponent>();

    for (const auto entity : view)
    {
        TestComponent& component = view.get<TestComponent>(entity);

        if (++component._y >= 100)
        {
            ecs.DestroyEntity(entity);
        }
    }
}
