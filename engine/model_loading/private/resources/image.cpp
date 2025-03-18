#include "resources/image.hpp"

#include <stb_image.h>

CPUImage& CPUImage::FromPNG(std::string_view path)
{
    int width;
    int height;
    int nrChannels;

    std::byte* data = reinterpret_cast<std::byte*>(stbi_load(std::string(path).c_str(),
        &width, &height, &nrChannels,
        4));

    if (data == nullptr)
    {
        throw std::runtime_error("Failed to load image!");
    }

    if (width > UINT16_MAX || height > UINT16_MAX)
    {
        throw std::runtime_error("Image size is too large!");
    }

    SetFormat(vk::Format::eR8G8B8A8Unorm);
    SetSize(static_cast<uint16_t>(width), static_cast<uint16_t>(height));
    SetName(path);
    initialData.assign(data, data + static_cast<ptrdiff_t>(width * height * 4));
    stbi_image_free(data);

    return *this;
}
CPUImage& CPUImage::SetData(std::vector<std::byte> data)
{
    initialData = std::move(data);
    return *this;
}

CPUImage& CPUImage::SetSize(uint16_t width, uint16_t height, uint16_t depth)
{
    this->width = width;
    this->height = height;
    this->depth = depth;
    return *this;
}

CPUImage& CPUImage::SetMips(uint8_t mips)
{
    this->mips = mips;
    return *this;
}

CPUImage& CPUImage::SetFlags(vk::ImageUsageFlags flags)
{
    this->flags = flags;
    return *this;
}

CPUImage& CPUImage::SetFormat(vk::Format format)
{
    this->format = format;
    return *this;
}

CPUImage& CPUImage::SetName(std::string_view name)
{
    this->name = name;
    return *this;
}

CPUImage& CPUImage::SetType(ImageType type)
{
    this->type = type;
    return *this;
}
