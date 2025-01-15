#include "physics_bindings.hpp"

#include "physics_module.hpp"
#include <optional>

namespace bindings
{
std::vector<RayHitInfo> ShootRay(PhysicsModule& self, const glm::vec3& origin, const glm::vec3& direction, const float distance)
{
    return self.ShootRay(origin, direction, distance);
}

entt::entity GetHitEntity(RayHitInfo& self)
{
    return self.entity;
}

glm::vec3 GetRayHitPosition(RayHitInfo& self)
{
    return self.position;
}

glm::vec3 GetRayHitNormal(RayHitInfo& self)
{
    return self.normal;
}
}
void BindPhysicsAPI(wren::ForeignModule& module)
{
    auto& wren_class = module.klass<PhysicsModule>("Physics");
    wren_class.funcExt<bindings::ShootRay>("ShootRay");

    auto& rayHitInfo = module.klass<RayHitInfo>("RayHitInfo");
    rayHitInfo.propReadonlyExt<bindings::GetHitEntity>("hitEntity");
    rayHitInfo.propReadonlyExt<bindings::GetRayHitPosition>("position");
    rayHitInfo.propReadonlyExt<bindings::GetRayHitNormal>("normal");
}
