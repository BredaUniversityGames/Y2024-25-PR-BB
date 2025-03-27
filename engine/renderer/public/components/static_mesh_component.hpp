#pragma once

#include "resource_manager.hpp"
#include "resources/mesh.hpp"

struct StaticMeshComponent
{
    ResourceHandle<GPUMesh> mesh;
};
