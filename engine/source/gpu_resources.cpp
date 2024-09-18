#include "gpu_resources.hpp"

#include <stb_image.h>


ImageCreation& ImageCreation::LoadFromFile(std::string_view file_path)
{
    int width;
    int height;
    int nrChannels; //dummy

    //yes we have to construct a string from a string_view here anyway but keeping the argument as a string_view keeps things consistent.
    initialData = reinterpret_cast<std::byte*>(stbi_load(std::string(file_path).c_str(), &width, &height, &nrChannels,
                                                         4));

    if (initialData == nullptr)
    {
        std::runtime_error("Failed to load image!");
    }
    if (width > std::numeric_limits<uint16_t>::max() || height > std::numeric_limits<uint16_t>::max())
    {
        std::runtime_error("Image size is too large!");
    }

    SetSize(width, height);

    return *this;
}


ImageCreation& ImageCreation::SetData(std::byte* data)
{
    initialData = data;
    return *this;
}

ImageCreation& ImageCreation::SetSize(uint16_t width, uint16_t height, uint16_t depth)
{
    this->width = width;
    this->height = height;
    this->depth = depth;
    return *this;
}

ImageCreation& ImageCreation::SetMips(uint8_t mips)
{
    this->mips = mips;
    return *this;
}

ImageCreation& ImageCreation::SetFlags(vk::ImageUsageFlags flags)
{
    this->flags = flags;
    return *this;
}

ImageCreation& ImageCreation::SetFormat(vk::Format format)
{
    this->format = format;
    return *this;
}

ImageCreation& ImageCreation::SetName(std::string_view name)
{
    this->name = name;
    return *this;
}