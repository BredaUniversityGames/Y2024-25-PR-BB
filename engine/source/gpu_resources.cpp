#include "gpu_resources.hpp"

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

ImageCreation& ImageCreation::SetType(ImageType type)
{
    this->type = type;
    return *this;
}
