#include "imgui_backend.hpp"

#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include "application_module.hpp"
#include "constants.hpp"
#include "gbuffers.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"
#include "swap_chain.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

ImGuiBackend::ImGuiBackend(const std::shared_ptr<GraphicsContext>& context, const ApplicationModule& applicationModule, const SwapChain& swapChain, const GBuffers& gbuffers)
    : _context(context)
{
    vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfoKhr {};
    pipelineRenderingCreateInfoKhr.colorAttachmentCount = 1;

    vk::Format swapChainFormat = swapChain.GetFormat();
    pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = &swapChainFormat;
    pipelineRenderingCreateInfoKhr.depthAttachmentFormat = gbuffers.DepthFormat();

    auto vkContext { _context->VulkanContext() };

    ImGui_ImplVulkan_InitInfo initInfoVulkan {};
    initInfoVulkan.UseDynamicRendering = true;
    initInfoVulkan.PipelineRenderingCreateInfo = static_cast<VkPipelineRenderingCreateInfo>(pipelineRenderingCreateInfoKhr);
    initInfoVulkan.PhysicalDevice = vkContext->PhysicalDevice();
    initInfoVulkan.Device = vkContext->Device();
    initInfoVulkan.ImageCount = MAX_FRAMES_IN_FLIGHT;
    initInfoVulkan.Instance = vkContext->Instance();
    initInfoVulkan.MSAASamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
    initInfoVulkan.Queue = vkContext->GraphicsQueue();
    initInfoVulkan.QueueFamily = vkContext->QueueFamilies().graphicsFamily.value();
    initInfoVulkan.DescriptorPool = vkContext->DescriptorPool();
    initInfoVulkan.MinImageCount = 2;
    initInfoVulkan.ImageCount = swapChain.GetImageCount();
    ImGui_ImplVulkan_Init(&initInfoVulkan);

    ImGui_ImplVulkan_CreateFontsTexture();

    ImGui_ImplSDL3_InitForVulkan(applicationModule.GetWindowHandle());

    _basicSampler = _context->Resources()->SamplerResourceManager().Create(SamplerCreation { .name = "ImGui sampler" });
}

ImGuiBackend::~ImGuiBackend()
{
    for (const auto imageID : _imageIDs)
    {
        ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(imageID));
    }

    ImGui_ImplVulkan_DestroyFontsTexture();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
}

void ImGuiBackend::NewFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
}

ImTextureID ImGuiBackend::GetTexture(ResourceHandle<GPUImage> image)
{
    const auto* resource = _context->Resources()->ImageResourceManager().Access(image);
    vk::ImageLayout layout {};
    switch (resource->type)
    {
    case ImageType::e2D:
    case ImageType::eDepth:
    case ImageType::eCubeMap:
        layout = vk::ImageLayout::eShaderReadOnlyOptimal;
        break;
    case ImageType::eShadowMap:
        layout = vk::ImageLayout::eShaderReadOnlyOptimal; // vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal; // TODO: I dont know why this works :l
        break;
    }

    _imageIDs.emplace_back(ImGui_ImplVulkan_AddTexture(_context->Resources()->SamplerResourceManager().Access(_basicSampler)->sampler, resource->view, static_cast<VkImageLayout>(layout)));

    return _imageIDs.back();
}
