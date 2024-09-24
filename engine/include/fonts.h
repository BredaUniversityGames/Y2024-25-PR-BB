//
// Created by luuk on 23-9-2024.
//

#pragma once
#include <map>
#include <string_view>
#include <vector>

#include <pch.hpp>
struct Character;
struct Image;




namespace utils
{
void LoadFont(std::string_view filepath, int fontsize,const VulkanBrain& brain);

	



}

class Font
{
public:
	//todo: convert this into font resource.
	static std::map<char, Character> Characters;
};
struct Character {
	vk::DescriptorSet DescriptorSet;
	ResourceHandle<Image> image;
	glm::ivec2   Size;       // Size of glyph
	glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
	unsigned int Advance;    // Offset to advance to next glyph
};


