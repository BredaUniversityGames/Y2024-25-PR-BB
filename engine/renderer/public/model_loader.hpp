#pragma once

#include "components/animation_channel_component.hpp"
#include "cpu_resources.hpp"
#include "vertex.hpp"

#include <include_fastgltf.hpp>
#include <memory>
#include <vector>

class SingleTimeCommands;
class BatchBuffer;
class GraphicsContext;

struct StagingAnimationChannels
{
    Animation animation;
    std::vector<AnimationChannelComponent> animationChannels;
    std::vector<uint32_t> nodeIndices;
};

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
