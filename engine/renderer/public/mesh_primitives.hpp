#pragma once

#include "vertex.hpp"
#include "cpu_resources.hpp"

CPUMesh<Vertex> GenerateUVSphere(uint32_t slices, uint32_t stacks, float radius = 1.0f);