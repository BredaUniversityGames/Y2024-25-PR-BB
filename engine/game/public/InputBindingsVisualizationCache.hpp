#pragma once

#include "input/action_manager.hpp"
#include "resource_management/image_resource_manager.hpp"

struct GPUImage;

struct CachedBindingOriginVisual
{
    std::string bindingInputName {};
    // Texture for the input visual. May be empty if a glyph is not available for the binding.
    ResourceHandle<GPUImage> glyphImage {};
};

class InputBindingsVisualizationCache
{
public:
    InputBindingsVisualizationCache(const ActionManager& actionManager, ImageResourceManager& imageResourceManager);

    NO_DISCARD std::vector<CachedBindingOriginVisual> GetDigital(std::string_view actionName);
    NO_DISCARD std::vector<CachedBindingOriginVisual> GetAnalog(std::string_view actionName);

private:
    NO_DISCARD ResourceHandle<GPUImage> GetGlyph(const std::string& path);

    const ActionManager& _actionManager;
    ImageResourceManager& _imageResourceManager;
    std::unordered_map<std::string, ResourceHandle<GPUImage>> _glyphCache {};
};
