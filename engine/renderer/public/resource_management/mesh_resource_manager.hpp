#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "common.hpp"
#include "resource_manager.hpp"

class VulkanContext;
struct Mesh;

class MeshResourceManager final : public ResourceManager<Mesh>
{
public:
    MeshResourceManager();
    ~MeshResourceManager();
    NON_COPYABLE(MeshResourceManager);
    NON_MOVABLE(MeshResourceManager);

private:
};
