#include "InputBindingsVisualizationCache.hpp"

InputBindingsVisualizationCache::InputBindingsVisualizationCache(const ActionManager& actionManager, ImageResourceManager& imageResourceManager)
    : _actionManager(actionManager)
    , _imageResourceManager(imageResourceManager)
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

    auto image = _imageResourceManager.Create(commonImageData.FromPNG(path));
    _glyphCache.emplace(path, image);
    return image;
}
