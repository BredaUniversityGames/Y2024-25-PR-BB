#pragma once

#include "gpu_resources.hpp"
#include "resource_manager.hpp"

#include <entt/entity/entity.hpp>

struct SkinnedMeshComponent
{
    ResourceHandle<GPUMesh> mesh {};
    entt::entity skeletonEntity { entt::null };
};
