#pragma once

#include "resource_manager.hpp"
#include "resources/mesh.hpp"

#include <entt/entity/entity.hpp>

struct SkinnedMeshComponent
{
    ResourceHandle<GPUMesh> mesh {};
    entt::entity skeletonEntity { entt::null };
    entt::entity rootEntity { entt::null };
};
