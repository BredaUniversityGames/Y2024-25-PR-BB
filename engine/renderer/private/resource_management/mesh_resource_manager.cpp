#include "resource_management/mesh_resource_manager.hpp"

#include "batch_buffer.hpp"
#include "gpu_resources.hpp"
#include "single_time_commands.hpp"
#include "vulkan_context.hpp"

template <>
std::weak_ptr<ResourceManager<GPUMesh>> ResourceHandle<GPUMesh>::manager = {};
