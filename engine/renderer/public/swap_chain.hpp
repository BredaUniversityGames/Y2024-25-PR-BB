#pragma once

#include "common.hpp"

#include <glm/vec2.hpp>
#include <memory>
#include <vector>
#include <vulkan_include.hpp>

struct QueueFamilyIndices;
class GraphicsContext;

class SwapChain
{
public:
    struct SupportDetails
    {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    SwapChain(const std::shared_ptr<GraphicsContext>& context, const glm::uvec2& screenSize);
    ~SwapChain();
    NON_MOVABLE(SwapChain);
    NON_COPYABLE(SwapChain);

    void Resize(const glm::uvec2& screenSize);
    size_t GetImageCount() const { return _images.size(); };
    vk::SwapchainKHR GetSwapChain() const { return _swapChain; }
    vk::ImageView GetImageView(uint32_t index) const { return _imageViews[index]; }
    vk::Extent2D GetExtent() const { return _extent; }
    vk::Format GetFormat() const { return _format; }
    vk::Image GetImage(uint32_t index) const { return _images[index]; }
    glm::uvec2 GetImageSize() const { return _imageSize; }

    static SupportDetails QuerySupport(vk::PhysicalDevice device, vk::SurfaceKHR surface);

private:
    std::shared_ptr<GraphicsContext> _context;
    glm::uvec2 _imageSize;

    vk::SwapchainKHR _swapChain;
    vk::Extent2D _extent;

    std::vector<vk::Image> _images;
    std::vector<vk::ImageView> _imageViews;
    vk::Format _format;

    void CreateSwapChain(const glm::uvec2& screenSize);
    void CleanUpSwapChain();
    vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, const glm::uvec2& screenSize);
    void CreateSwapChainImageViews();
};