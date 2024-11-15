#pragma once

#include <imgui.h>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "resource_handle.hpp"

class GraphicsContext;
class SwapChain;
class ApplicationModule;
class GBuffers;
struct Image;

class ImGuiBackend
{
public:
    ImGuiBackend(const std::shared_ptr<GraphicsContext>& context, const ApplicationModule& applicationModule, const SwapChain& swapChain, const GBuffers& gbuffers);
    ~ImGuiBackend();

    NON_COPYABLE(ImGuiBackend);
    NON_MOVABLE(ImGuiBackend);

    void NewFrame();

    ImTextureID GetTexture(ResourceHandle<Image> image);

private:
    std::shared_ptr<GraphicsContext> _context;
    vk::UniqueSampler _basicSampler;

    // TODO: Textures are currently only cleaned up on shutdown.
    std::vector<ImTextureID> _imageIDs;
};