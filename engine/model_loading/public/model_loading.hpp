#pragma once

#include "components/animation_channel_component.hpp"
#include "cpu_resources.hpp"
#include "include_fastgltf.hpp"

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

namespace ModelLoading
{
// Loads a GLTF model from the given path to the file.
NO_DISCARD CPUModel LoadGLTF(std::string_view path);
// Reads the vertex and index sizes of the entire model from the given file path.
void ReadGeometrySizeGLTF(std::string_view path, uint32_t& vertexBufferSize, uint32_t& indexBufferSize);
}
