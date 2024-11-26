#pragma once

#include "mesh.hpp"

#include <model.hpp>

CPUMesh<Vertex>::Primitive GenerateUVSphere(uint32_t slices, uint32_t stacks, float radius = 1.0f);