#include "resource_management/mesh_resource_manager.hpp"

#include "gpu_resources.hpp"

template <>
std::weak_ptr<ResourceManager<Mesh>> ResourceHandle<Mesh>::manager = {};

MeshResourceManager::MeshResourceManager() = default;
