#pragma once

#include "resource_manager.hpp"

#include <map>

struct Image;

class VulkanBrain;

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
    ResourceHandle<Image> _fontAtlas;
};

NO_DISCARD Font LoadFromFile(const std::string& path, uint16_t characterHeight, const VulkanBrain& brain);
