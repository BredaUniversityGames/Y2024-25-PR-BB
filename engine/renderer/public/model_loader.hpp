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

    enum class LoadMode : uint8_t
    {
        eFlat,
        eHierarchical
    };

    CPUModelData ExtractModelFromGltfFile(std::string_view path, LoadMode loadMode);

    void ReadGeometrySize(std::string_view path, uint32_t& vertexBufferSize, uint32_t& indexBufferSize);

private:
    fastgltf::Parser _parser;
};
