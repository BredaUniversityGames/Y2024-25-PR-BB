#pragma once

#include "mesh.hpp"
#include "model.hpp"

#include <lib/include_fastgltf.hpp>
#include <memory>
#include <vector>

class SingleTimeCommands;
class BatchBuffer;
class ECS;
class GraphicsContext;

class ModelLoader
{
public:
    ModelLoader() = default;
    ~ModelLoader() = default;

    NON_COPYABLE(ModelLoader);
    NON_MOVABLE(ModelLoader);

    NO_DISCARD CPUModel ExtractModelFromGltfFile(std::string_view path);

    void ReadGeometrySize(std::string_view path, uint32_t& vertexBufferSize, uint32_t& indexBufferSize);

private:
    fastgltf::Parser _parser;
};
