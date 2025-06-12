#pragma once

#include "input/action_manager.hpp"
#include "graphics_context.hpp"

struct GPUImage;

struct CachedBindingOriginVisual
{
    std::string bindingInputName {};
    // Texture for the input visual. May be empty if a glyph is not available for the binding.
    ResourceHandle<GPUImage> glyphImage {};
};

class input_bindings_visualization_cache
{
public:
    input_bindings_visualization_cache(const ActionManager& actionManager, GraphicsContext& graphicsContext);

    NO_DISCARD std::vector<CachedBindingOriginVisual> GetDigital(std::string_view actionName);
    NO_DISCARD std::vector<CachedBindingOriginVisual> GetAnalog(std::string_view actionName);

private:
    NO_DISCARD ResourceHandle<GPUImage> GetGlyph(const std::string& path);

    const ActionManager& _actionManager;
    GraphicsContext& _graphicsContext;
    std::unordered_map<std::string, ResourceHandle<GPUImage>> _glyphCache {};
};
