//
// Created by luuk on 23-9-2024.
//

#include "include/fonts.h"

#include <ft2build.h>
#include FT_FREETYPE_H  
#include "vulkan_brain.hpp"
#include "gpu_resources.hpp"

void utils::LoadFont(std::string_view filepath, int fontsize, const VulkanBrain& brain)
{
	FT_Library library;
	FT_Init_FreeType(&library);
	
	FT_Face fontFace;
	FT_New_Face(library, filepath.data(), 0, &fontFace);
	FT_Set_Pixel_Sizes(fontFace, 0, fontsize);

	int padding = 2;
	int row = 0;
	int col = padding;

	auto texturewidth = (127-32)*fontsize;
	uint8_t texture[texturewidth*fontsize];
	for (unsigned char c = 0; c < 128; c++)
	{
		// load character glyph 
		if (FT_Load_Char(fontFace, c, FT_LOAD_RENDER))
		{
			continue;
		}


		ImageCreation image;
		image.width = 	fontFace->glyph->bitmap.width;
		image.height = 	fontFace->glyph->bitmap.rows;
		image.SetData(reinterpret_cast<std::byte*>(fontFace->glyph->bitmap.buffer));
		image.SetFormat(vk::Format::eR8Uint);

		// now store character for later use
		Character character = {
			brain.ImageResourceManager().Create(image),
			glm::ivec2(fontFace->glyph->bitmap.width, fontFace->glyph->bitmap.rows),
			glm::ivec2(fontFace->glyph->bitmap_left, fontFace->glyph->bitmap_top),
			uint8_t(fontFace->glyph->advance.x)
		};
		Characters.insert(std::pair<char, Character>(c, character));
	}
	FT_Done_Face(fontFace);
	FT_Done_FreeType(library);
}
