#include "InputBindingsVisualizationCache.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "graphics_resources.hpp"

InputBindingsVisualizationCache::InputBindingsVisualizationCache(const ActionManager& actionManager, GraphicsContext& graphicsContext)
    : _actionManager(actionManager)
    , _graphicsContext(graphicsContext)
{
}

std::vector<CachedBindingOriginVisual> InputBindingsVisualizationCache::GetDigital(std::string_view actionName)
{
    auto visualizations = _actionManager.GetDigitalActionBindingOriginVisual(actionName);
    std::vector<CachedBindingOriginVisual> out {};

    for (const auto& visualization : visualizations)
    {
        CachedBindingOriginVisual& cached = out.emplace_back();
        cached.bindingInputName = visualization.bindingInputName;
        cached.glyphImage = GetGlyph(visualization.glyphImagePath);
    }

    return out;
}

std::vector<CachedBindingOriginVisual> InputBindingsVisualizationCache::GetAnalog(std::string_view actionName)
{
    auto visualizations = _actionManager.GetAnalogActionBindingOriginVisual(actionName);
    std::vector<CachedBindingOriginVisual> out {};

    for (const auto& visualization : visualizations)
    {
        CachedBindingOriginVisual& cached = out.emplace_back();
        cached.bindingInputName = visualization.bindingInputName;
        cached.glyphImage = GetGlyph(visualization.glyphImagePath);
    }

    return out;
}

ResourceHandle<GPUImage> InputBindingsVisualizationCache::GetGlyph(const std::string& path)
{
    if (path.empty())
    {
        return ResourceHandle<GPUImage>::Null();
    }

    auto it = _glyphCache.find(path);
    if (it != _glyphCache.end())
    {
        return it->second;
    }

    CPUImage commonImageData;
    commonImageData.format = vk::Format::eR8G8B8A8Unorm;
    commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
    commonImageData.isHDR = false;

    auto& imageResourceManager = _graphicsContext.Resources()->ImageResourceManager();
    auto image = imageResourceManager.Create(commonImageData.FromPNG(path));
    _graphicsContext.UpdateBindlessSet();

    _glyphCache.emplace(path, image);
    return image;
}
