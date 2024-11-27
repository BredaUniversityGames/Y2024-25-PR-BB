#pragma once

#include "resource_manager.hpp"

#include <map>

class GraphicsContext;
struct GPUImage;

struct Font
{
    struct Character
    {
        glm::ivec2 Size;
        glm::ivec2 Bearing;
        uint16_t Advance;

        glm::vec2 uvp1;
        glm::vec2 uvp2;
    };

    std::map<uint8_t, Character> characters;
    ResourceHandle<GPUImage> _fontAtlas;
    uint16_t characterHeight;
};

NO_DISCARD std::shared_ptr<Font> LoadFromFile(const std::string& path, uint16_t characterHeight, std::shared_ptr<GraphicsContext> context);
