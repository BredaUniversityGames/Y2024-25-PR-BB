#include "fonts.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "vulkan_brain.hpp"
#include "gpu_resources.hpp"
#include "vulkan_helper.hpp"

// chatgpt
void appendBitmapToAtlas(
    const unsigned char* bitmapBuffer,
    int bitmapWidth,
    int bitmapHeight,
    int targetWidth,
    int targetHeight,
    std::vector<std::byte>& atlasData)
{
    for (int row = 0; row < targetHeight; ++row)
    {
        if (row < bitmapHeight)
        {
            // Calculate the start of the current row in the bitmap buffer
            const std::byte* rowStart = reinterpret_cast<const std::byte*>(bitmapBuffer + row * bitmapWidth);

            // Copy the current row to atlasData
            atlasData.insert(atlasData.end(), rowStart, rowStart + bitmapWidth);

            // Add horizontal padding if the row width is less than the target width
            if (bitmapWidth < targetWidth)
            {
                size_t padding = targetWidth - bitmapWidth;
                atlasData.insert(atlasData.end(), padding, std::byte { 0 }); // Fill with zeros
            }
        }
        else
        {
            // Add a full row of horizontal padding (blank row) for vertical padding
            atlasData.insert(atlasData.end(), targetWidth, std::byte { 0 });
        }
    }
}
Font LoadFromFile(const std::string& path, uint16_t characterHeight, const VulkanBrain& brain)
{
    FT_Library library;
    FT_Init_FreeType(&library);

    FT_Face fontFace;

    if (FT_New_Face(library, path.c_str(), 0, &fontFace))
    {
        throw std::runtime_error("Failed to load font");
    }
    FT_Set_Pixel_Sizes(fontFace, 0, characterHeight);

    Font font;
    std::vector<std::byte> atlasData;

#define CHARACTERMIN 0
#define CHARACTERMAX 130

    characterHeight = characterHeight * 1.5;

    constexpr uint8_t characterAmount = CHARACTERMAX - CHARACTERMIN;

    for (unsigned char c = CHARACTERMIN; c < CHARACTERMAX; c++)
    {

        // load character glyph
        if (FT_Load_Char(fontFace, c, FT_LOAD_RENDER))
        {
            continue;
        }

        if (c > 120)
        {
            bblog::info("sda");
        }
        if (fontFace->glyph->bitmap.buffer == nullptr || fontFace->glyph->bitmap.width == 0 || fontFace->glyph->bitmap.rows == 0)
        {
            atlasData.insert(atlasData.end(), characterHeight * characterHeight, std::byte { 0 });
            continue;
        }

        uint8_t index = c - CHARACTERMIN;

        Font::Character character {
            glm::ivec2(fontFace->glyph->bitmap.width, fontFace->glyph->bitmap.rows),
            glm::ivec2(fontFace->glyph->bitmap_left, fontFace->glyph->bitmap_top),
            uint16_t(fontFace->glyph->advance.x),
            glm::vec2(0, (static_cast<float>(index)) / characterAmount),
            glm::vec2(float(fontFace->glyph->bitmap.width) / (characterHeight),
                (static_cast<float>(index + 1)) / characterAmount)
        };

        appendBitmapToAtlas(fontFace->glyph->bitmap.buffer, fontFace->glyph->bitmap.width, fontFace->glyph->bitmap.rows, characterHeight, characterHeight, atlasData);

        font.characters.insert(std::pair(c, character));
    }

    ImageCreation image;
    image.name = path + "font atlas";
    image.width = characterHeight;
    image.height = characterAmount * characterHeight;
    image.SetData(atlasData.data());
    image.SetFormat(vk::Format::eR8Unorm);
    image.SetFlags(vk::ImageUsageFlagBits::eSampled);
    image.isHDR = false;

    size_t expectedDataSize
        = static_cast<size_t>(image.width) * static_cast<size_t>(image.height);
    if (atlasData.size() != expectedDataSize)
    {
        throw std::runtime_error("Mismatch in data size: atlasData.size()");
    }

    font._fontAtlas = brain.GetImageResourceManager().Create(image);
    brain.UpdateBindlessSet();

    FT_Done_Face(fontFace);
    FT_Done_FreeType(library);

    return font;
}