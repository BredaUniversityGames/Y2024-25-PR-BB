//
// Created by luuk on 23-9-2024.
//

#include "ui/fonts.hpp"

#include <ft2build.h>

#include "engine.hpp"

#include FT_FREETYPE_H
#include "vulkan_brain.hpp"
#include "gpu_resources.hpp"
#include "ui/UserInterfaceSystem.hpp"
#include "vulkan_helper.hpp"

void Fonts::LoadFont(std::string_view filepath, int fontsize, const VulkanBrain& brain)
{
    FT_Library library;
    FT_Init_FreeType(&library);

    FT_Face fontFace;
    FT_New_Face(library, filepath.data(), 0, &fontFace);
    FT_Set_Pixel_Sizes(fontFace, 0, fontsize);

    int padding = 2;
    int row = 0;
    int col = padding;

    // uto sampler = util::CreateSampler(brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToBorder, vk::SamplerMipmapMode::eLinear, 0);

    for (unsigned char c = 32; c < 128; c++)
    {
        // load character glyph
        if (FT_Load_Char(fontFace, c, FT_LOAD_RENDER))
        {
            continue;
        }

        if (fontFace->glyph->bitmap.buffer == nullptr | fontFace->glyph->bitmap.width == 0 || fontFace->glyph->bitmap.rows == 0)
        {
            continue;
        }

        ImageCreation image;
        image.width = fontFace->glyph->bitmap.width;
        image.height = fontFace->glyph->bitmap.rows;
        image.SetData(reinterpret_cast<std::byte*>(fontFace->glyph->bitmap.buffer));
        image.SetFormat(vk::Format::eR8Unorm);
        image.SetFlags(vk::ImageUsageFlagBits::eSampled);
        image.isHDR = false;
        auto handle = brain.GetImageResourceManager().Create(image);

        Character character = {
            handle,
            glm::ivec2(fontFace->glyph->bitmap.width, fontFace->glyph->bitmap.rows),
            glm::ivec2(fontFace->glyph->bitmap_left, fontFace->glyph->bitmap_top),
            uint16_t(fontFace->glyph->advance.x)
        };

        Characters.insert(std::pair<char, Character>(c, character));
    }
    brain.UpdateBindlessSet();
    FT_Done_Face(fontFace);
    FT_Done_FreeType(library);
}
