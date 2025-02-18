#pragma once

#include "gpu_resources.hpp"
#include "resource_manager.hpp"

struct StaticMeshComponent
{
    ResourceHandle<GPUMesh> mesh;
};
