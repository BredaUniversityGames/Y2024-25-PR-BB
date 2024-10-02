#pragma once


#include <pch.hpp>
#include <map>

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


