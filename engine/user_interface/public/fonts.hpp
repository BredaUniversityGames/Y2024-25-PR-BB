#pragma once

#include <pch.hpp>
#include <map>

struct Image;

namespace Fonts
{

struct Character
{
    ResourceHandle<Image> image;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    uint16_t Advance;
};

// todo: this needs to be changed to return a created font resource
void LoadFont(std::string_view filepath, int fontsize, const VulkanBrain& brain);
// todo: convert this into font resource and make a single texture atlas
static std::map<char, Character> Characters {};

}
