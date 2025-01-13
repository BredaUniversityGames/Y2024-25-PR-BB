#include "physics_bindings.hpp"

#include "physics_module.hpp"
#include <optional>

namespace bindings
{
RayHitInfo ShootRay(PhysicsModule& self, const glm::vec3& origin, const glm::vec3& direction, const float distance)
{
    return self.ShootRay(origin, direction, distance);
}

bool GetRayHitBool(RayHitInfo& self)
{
    return self.hasHit;
}

std::optional<entt::entity> GetHitEntity(RayHitInfo& self)
{
    return self.hasHit ? std::optional(self.entity) : std::nullopt;
}

glm::vec3 GetRayHitPosition(RayHitInfo& self)
{
    return self.position;
}

}

void BindPhysicsAPI(wren::ForeignModule& module)
{
    auto& wren_class = module.klass<PhysicsModule>("Physics");
    wren_class.funcExt<bindings::ShootRay>("ShootRay");

    auto& rayHitInfo = module.klass<RayHitInfo>("RayHitInfo");
    rayHitInfo.propReadonlyExt<bindings::GetHitEntity>("hitEntity");
    rayHitInfo.propReadonlyExt<bindings::GetRayHitBool>("hasHit");
    rayHitInfo.propReadonlyExt<bindings::GetRayHitPosition>("position");
}
