#include "fonts.hpp"

#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <file_io.hpp>
#include <memory>
#include <resource_management/sampler_resource_manager.hpp>
#include <stb_rect_pack.h>

struct FTStreamContext
{
    explicit FTStreamContext(PhysFS::ifstream* stream)
        : stream(stream)
    {
    }
    PhysFS::ifstream* stream;
};

unsigned long FTStreamRead(FT_Stream ftStream, unsigned long offset, unsigned char* buffer, unsigned long count)
{
    auto* ctx = static_cast<FTStreamContext*>(ftStream->descriptor.pointer);
    auto& stream = *(ctx->stream);

    stream.clear(); // clear any previous eof/fail bits
    stream.seekg(offset, std::ios::beg);
    if (!stream.good())
        return 0;

    if (buffer)
    {
        stream.read(reinterpret_cast<char*>(buffer), count);
        return static_cast<unsigned long>(stream.gcount());
    }
    else
    {
        // FreeType may call this with buffer == nullptr to test size/seekability
        return 0;
    }
}

void FTStreamClose(MAYBE_UNUSED FT_Stream ftStream)
{
}

std::shared_ptr<UIFont> LoadFromFile(const std::string& path, uint16_t characterHeight, GraphicsContext& context)
{
    FT_Library library;
    FT_Init_FreeType(&library);

    FT_Face fontFace = nullptr;

    auto stream = fileIO::OpenReadStream(path.c_str());
    if (!stream)
        return nullptr;

    stream->seekg(0, std::ios::end);
    std::streamsize size = stream->tellg();
    stream->seekg(0, std::ios::beg);

    auto ftStream = std::make_unique<FT_StreamRec>();
    auto ctx = std::make_unique<FTStreamContext>(&stream.value());

    ftStream->size = static_cast<unsigned long>(size);
    ftStream->pos = 0;
    ftStream->descriptor.pointer = ctx.get();
    ftStream->pathname.pointer = nullptr;
    ftStream->read = FTStreamRead;
    ftStream->close = FTStreamClose;
    ftStream->memory = nullptr;
    ftStream->cursor = nullptr;
    ftStream->limit = nullptr;

    FT_Open_Args args {};
    args.flags = FT_OPEN_STREAM;
    args.stream = ftStream.get();

    auto err = FT_Open_Face(library, &args, 0, &fontFace);
    if (err != 0)
    {
        return nullptr;
    }

    std::shared_ptr<UIFont> font = std::make_shared<UIFont>();

    FT_Set_Pixel_Sizes(fontFace, 0, characterHeight);
    font->metrics.resolutionY = characterHeight;

    FT_Pos ascent = fontFace->size->metrics.ascender;
    FT_Pos descent = fontFace->size->metrics.descender;
    FT_Pos line_gap = fontFace->size->metrics.height - (fontFace->size->metrics.ascender - fontFace->size->metrics.descender);

    font->metrics.ascent = (ascent >> 6);
    font->metrics.descent = (descent >> 6);
    font->metrics.lineGap = (line_gap >> 6);

    const uint8_t maxGlyphs = 128;

    std::array<stbrp_rect, maxGlyphs> rects;
    for (uint8_t c = 0; c < maxGlyphs; ++c)
    {
        if (FT_Load_Char(fontFace, c, FT_LOAD_RENDER | FT_LOAD_MONOCHROME) != 0)
        {
            rects[c].id = c;
            rects[c].w = rects[c].h = 0;
            continue;
        }

        rects[c].id = c;
        rects[c].w = fontFace->glyph->bitmap.width + 2; // Add margin
        rects[c].h = fontFace->glyph->bitmap.rows + 2;
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

            glm::uvec2 atlasStart = { rects[c].x + 1, rects[c].y + 1 };
            for (uint16_t y = 0; y < bitmap.rows; ++y)
            {
                for (uint16_t x = 0; x < bitmap.width; ++x)
                {
                    glm::uvec2 atlasIndex = { atlasStart.x + x, atlasStart.y + y };
                    atlasData[atlasIndex.y * atlasWidth + atlasIndex.x] = std::byte(bitmap.buffer[y * bitmap.width + x]);
                }
            }

            UIFont::Character cInfo {};
            cInfo.size = { bitmap.width, bitmap.rows };
            cInfo.bearing = { fontFace->glyph->bitmap_left, fontFace->glyph->bitmap_top };

            cInfo.uvMin = {
                static_cast<float>(rects[c].x + 1) / atlasWidth,
                static_cast<float>(rects[c].y + 1) / atlasHeight
            };

            cInfo.uvMax = {
                static_cast<float>(rects[c].x + 1 + bitmap.width) / atlasWidth,
                static_cast<float>(rects[c].y + 1 + bitmap.rows) / atlasHeight
            };

            cInfo.advance = fontFace->glyph->advance.x >> 6;

            // Store Character and GlyphRegion
            font->characters[c] = cInfo;
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

    SamplerCreation samplerCreation;
    samplerCreation.minFilter = vk::Filter::eNearest;
    samplerCreation.magFilter = vk::Filter::eNearest;
    static ResourceHandle<Sampler> sampler = context.Resources()->SamplerResourceManager().Create(samplerCreation);

    font->fontAtlas = context.Resources()->ImageResourceManager().Create(image, sampler);
    context.UpdateBindlessSet();

    FT_Done_Face(fontFace);
    FT_Done_FreeType(library);

    return font;
}