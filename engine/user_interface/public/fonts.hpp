#pragma once

#include "resource_manager.hpp"
#include <map>
#include <glm/glm.hpp>

class GraphicsContext;
struct GPUImage;

struct Font
{
    struct Character
    {
        glm::ivec2 size;
        glm::ivec2 bearing;
        uint16_t advance;

        glm::vec2 uvMin;
        glm::vec2 uvMax;
    };

    std::map<uint8_t, Character> characters;
    ResourceHandle<GPUImage> fontAtlas;
    uint16_t characterHeight;
};

NO_DISCARD std::shared_ptr<Font> LoadFromFile(const std::string& path, uint16_t characterHeight, std::shared_ptr<GraphicsContext>& context);
