#pragma once

#include "resource_manager.hpp"
#include <glm/glm.hpp>
#include <unordered_map>

class GraphicsContext;
struct GPUImage;

struct UIFont
{
    struct Character
    {
        glm::ivec2 size;
        glm::ivec2 bearing;
        uint16_t advance;

        glm::vec2 uvMin;
        glm::vec2 uvMax;
    };

    std::unordered_map<uint8_t, Character> characters;
    ResourceHandle<GPUImage> fontAtlas;
    uint16_t characterHeight;
};

NO_DISCARD std::shared_ptr<UIFont> LoadFromFile(const std::string& path, uint16_t characterHeight, std::shared_ptr<GraphicsContext>& context);
