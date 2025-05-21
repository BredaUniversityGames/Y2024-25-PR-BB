#include "aim_assist.hpp"

#include "game_module.hpp"
#include "components/transform_helpers.hpp"
#include "components/transform_component.hpp"

glm::vec3 AimAssist::GetAimAssistDirection(ECSModule& ecs, const glm::vec3& forward)
{
    const float minAngle = 0.96f;

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

        if (parallel >= closestParallel)
        {
            result = playerToEnemy;
            closestParallel = parallel;
        }
    }

    return result;
}
