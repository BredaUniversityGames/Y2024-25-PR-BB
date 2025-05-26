#include "aim_assist.hpp"

#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "game_module.hpp"
#include "physics_module.hpp"

bool IsVisible(ECSModule& ecs, PhysicsModule& physics, const glm::vec3& origin, const glm::vec3& direction, float range, entt::entity target)
{
    auto hits = physics.ShootRay(origin, direction, range);

    for (auto& hit : hits)
    {
        bool isPlayer = ecs.GetRegistry().all_of<PlayerTag>(hit.entity);

        if (isPlayer)
        {
            continue;
        }

        return hit.entity == target;
    }

    return false;
}

glm::vec3 AimAssist::GetAimAssistDirection(ECSModule& ecs, PhysicsModule& physics, const glm::vec3& forward, float range, float minAngle)
{
    glm::vec3 result = forward;
    float closestParallel = minAngle;

    auto& reg = ecs.GetRegistry();

    auto& player = *reg.view<PlayerTag>().begin();
    glm::vec3 playerPosition = TransformHelpers::GetWorldPosition(reg, player);

    for (auto enemy : reg.view<TransformComponent, EnemyTag>())
    {
        glm::vec3 enemyPosition = TransformHelpers::GetWorldPosition(reg, enemy);
        glm::vec3 playerToEnemy = glm::normalize(enemyPosition - playerPosition);
        float parallel = glm::dot(forward, playerToEnemy);

        if (parallel < closestParallel)
        {
            continue;
        }

        // Make sure the enemy is not behind something
        if (!IsVisible(ecs, physics, playerPosition, playerToEnemy, range, enemy))
        {
            continue;
        }

        result = playerToEnemy;
        closestParallel = parallel;
    }

    return result;
}
