#include "fonts.hpp"

#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <stb_rect_pack.h>

std::shared_ptr<UIFont> LoadFromFile(const std::string& path, uint16_t characterHeight, GraphicsContext& context)
{
    FT_Library library;
    FT_Init_FreeType(&library);

    FT_Face fontFace;
    FT_New_Face(library, path.c_str(), 0, &fontFace);

    FT_Set_Pixel_Sizes(fontFace, 0, characterHeight);

    std::shared_ptr<UIFont> font = std::make_shared<UIFont>();
    font->characterHeight = characterHeight;

    const uint8_t maxGlyphs = 128;
    std::array<stbrp_rect, maxGlyphs> rects;
    for (uint8_t c = 0; c < maxGlyphs; ++c)
    {
        if (FT_Load_Char(fontFace, c, FT_LOAD_RENDER) != 0)
        {
            rects[c].id = c;
            rects[c].w = rects[c].h = 0;
            continue;
        }

        rects[c].id = c;
        rects[c].w = fontFace->glyph->bitmap.width + 1;
        rects[c].h = fontFace->glyph->bitmap.rows + 1;
    }

    const uint16_t atlasWidth = 512;
    const uint16_t atlasHeight = 512;

    stbrp_context stbrpContext;
    std::vector<stbrp_node> nodes(atlasWidth);

    stbrp_init_target(&stbrpContext, atlasWidth, atlasHeight, nodes.data(), atlasWidth);
    if (stbrp_pack_rects(&stbrpContext, rects.data(), maxGlyphs) == 0)
    {
        throw std::runtime_error("Failed to pack font glyphs into the atlas.");
    }

    std::vector<std::byte> atlasData(atlasWidth * atlasHeight, std::byte { 0 });

    for (uint8_t c = 0; c < maxGlyphs; ++c)
    {
        if (rects[c].was_packed != 0)
        {
            if (FT_Load_Char(fontFace, c, FT_LOAD_RENDER) != 0)
            {
                continue;
            }

            const FT_Bitmap& bitmap = fontFace->glyph->bitmap;
            for (uint16_t y = 0; y < bitmap.rows; ++y)
            {
                for (uint16_t x = 0; x < bitmap.width; ++x)
                {
                    atlasData[((rects[c].y + y) * atlasWidth) + (rects[c].x + x)] = std::byte(bitmap.buffer[y * bitmap.width + x]);
                }
            }

            // Store Character and GlyphRegion
            font->characters[c] = {
                .size = glm::ivec2(bitmap.width, bitmap.rows),
                .bearing = glm::ivec2(fontFace->glyph->bitmap_left, fontFace->glyph->bitmap_top),
                .advance = static_cast<uint16_t>(fontFace->glyph->advance.x),
                .uvMin = glm::vec2(static_cast<float>(rects[c].x) / atlasWidth, static_cast<float>(rects[c].y) / atlasHeight),
                .uvMax = glm::vec2(static_cast<float>(rects[c].x + bitmap.width) / atlasWidth,
                    static_cast<float>(rects[c].y + bitmap.rows) / atlasHeight)
            };
        }
    }

    CPUImage image;
    image.name = path + " fontatlas";
    image.width = atlasWidth;
    image.height = atlasHeight;
    image.SetData(std::move(atlasData));
    image.SetFormat(vk::Format::eR8Unorm);
    image.SetFlags(vk::ImageUsageFlagBits::eSampled);
    image.isHDR = false;

    font->fontAtlas = context.Resources()->ImageResourceManager().Create(image);
    context.UpdateBindlessSet();

    FT_Done_Face(fontFace);
    FT_Done_FreeType(library);

    return font;
}