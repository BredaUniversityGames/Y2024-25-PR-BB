#pragma once

#include "include_fastgltf.hpp"
#include "components/animation_channel_component.hpp"
#include "cpu_resources.hpp"

#include <vector>

struct StagingAnimationChannels
{
    std::vector<Animation> animations;

    struct IndexChannel
    {
        std::vector<TransformAnimationSpline> animationChannels;
        std::vector<uint32_t> nodeIndices;
    };
    std::vector<IndexChannel> indexChannels;
};

class ModelLoader
{
public:
    ModelLoader() = default;
    ~ModelLoader() = default;

    NON_COPYABLE(ModelLoader);
    NON_MOVABLE(ModelLoader);

    NO_DISCARD CPUModel LoadGLTF(std::string_view path);
    void ReadGeometrySizeGLTF(std::string_view path, uint32_t& vertexBufferSize, uint32_t& indexBufferSize);

private:
    constexpr static auto DEFAULT_LOAD_FLAGS = fastgltf::Options::DecomposeNodeMatrices | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages;
    fastgltf::Parser _parser;
};
