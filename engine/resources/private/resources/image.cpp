#include "resources/image.hpp"
#include "profile_macros.hpp"
#include "vulkan_helper.hpp"

#include <file_io.hpp>
#include <stb_image.h>

CPUImage& CPUImage::FromPNG(std::string_view path)
{
    int width;
    int height;
    int nrChannels;

    auto stream1 = fileIO::OpenReadStream(std::string(path));
    auto stream2 = std::ifstream { std::string { path } };

    std::byte* data = nullptr;
    if (path.find("Steam") > path.size())
    {
        data = reinterpret_cast<std::byte*>(fileIO::LoadImageFromIfstream(stream1.value(),
            &width, &height, &nrChannels,
            4));
    }
    else
    {
        data = reinterpret_cast<std::byte*>(stbi_load(path.data(),
            &width, &height, &nrChannels,
            4));
    }

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

vk::ImageType ImageTypeConversion(ImageType type)
{
    switch (type)
    {
    case ImageType::e2D:
    case ImageType::eDepth:
    case ImageType::eShadowMap:
    case ImageType::eCubeMap:
        return vk::ImageType::e2D;
    default:
        throw std::runtime_error("Unsupported ImageType!");
    }
}

vk::ImageViewType ImageViewTypeConversion(ImageType type)
{
    switch (type)
    {
    case ImageType::eShadowMap:
    case ImageType::e2D:
        return vk::ImageViewType::e2D;
    case ImageType::eDepth:
        return vk::ImageViewType::e2D;
    case ImageType::eCubeMap:
        return vk::ImageViewType::eCube;
    default:
        throw std::runtime_error("Unsupported ImageType!");
    }
}

GPUImage::GPUImage(const CPUImage& creation, ResourceHandle<Sampler> textureSampler, const std::shared_ptr<VulkanContext>& context, SingleTimeCommands* const commands)
    : _context(context)
{
    width = creation.width;
    height = creation.height;
    depth = creation.depth;
    layers = creation.layers;
    flags = creation.flags;
    type = creation.type;
    format = creation.format;
    mips = std::min(creation.mips, static_cast<uint8_t>(floor(log2(std::max(width, height))) + 1));
    name = creation.name;
    isHDR = creation.isHDR;
    sampler = textureSampler;

    vk::ImageCreateInfo imageCreateInfo {};
    imageCreateInfo.imageType = ImageTypeConversion(creation.type);
    imageCreateInfo.extent.width = creation.width;
    imageCreateInfo.extent.height = creation.height;
    imageCreateInfo.extent.depth = creation.depth;
    imageCreateInfo.mipLevels = creation.mips;
    imageCreateInfo.arrayLayers = creation.type == ImageType::eCubeMap ? 6 : creation.layers;
    imageCreateInfo.format = creation.format;
    imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imageCreateInfo.samples = vk::SampleCountFlagBits::e1;

    imageCreateInfo.usage = creation.flags;

    if (creation.initialData.data())
    {
        imageCreateInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;
        imageCreateInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc;
    }
    if (creation.type == ImageType::eCubeMap)
    {
        imageCreateInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;
    }

    {
        ZoneScopedN("VMA Image allocation");
        VmaAllocationCreateInfo allocCreateInfo {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        util::vmaCreateImage(_context->MemoryAllocator(), reinterpret_cast<VkImageCreateInfo*>(&imageCreateInfo), &allocCreateInfo, reinterpret_cast<VkImage*>(&image), &allocation, nullptr);
        std::string allocName = creation.name + " texture allocation";
        vmaSetAllocationName(_context->MemoryAllocator(), allocation, allocName.c_str());
    }

    vk::ImageViewCreateInfo viewCreateInfo {};
    viewCreateInfo.image = image;
    viewCreateInfo.viewType = vk::ImageViewType::e2D;
    viewCreateInfo.format = creation.format;
    viewCreateInfo.subresourceRange.aspectMask = util::GetImageAspectFlags(format);
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = creation.mips;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;

    for (size_t i = 0; i < imageCreateInfo.arrayLayers; ++i)
    {
        viewCreateInfo.subresourceRange.levelCount = creation.mips;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.baseArrayLayer = i;
        Layer& layer = layerViews.emplace_back();
        layer.view = _context->Device().createImageView(viewCreateInfo);

        for (size_t j = 0; j < imageCreateInfo.mipLevels; ++j)
        {
            viewCreateInfo.subresourceRange.levelCount = 1;
            viewCreateInfo.subresourceRange.baseMipLevel = j;
            layer.mipViews.emplace_back(_context->Device().createImageView(viewCreateInfo));
        }
    }
    view = layerViews.begin()->view;

    if (creation.type == ImageType::eCubeMap)
    {
        vk::ImageViewCreateInfo cubeViewCreateInfo {};
        cubeViewCreateInfo.image = image;
        cubeViewCreateInfo.viewType = ImageViewTypeConversion(creation.type);
        cubeViewCreateInfo.format = creation.format;
        cubeViewCreateInfo.subresourceRange.aspectMask = util::GetImageAspectFlags(format);
        cubeViewCreateInfo.subresourceRange.baseMipLevel = 0;
        cubeViewCreateInfo.subresourceRange.levelCount = creation.mips;
        cubeViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        cubeViewCreateInfo.subresourceRange.layerCount = 6;

        util::VK_ASSERT(_context->Device().createImageView(&cubeViewCreateInfo, nullptr, &view), "Failed creating image view!");
    }

    if (creation.initialData.data())
    {
        ZoneScopedN("Image data Upload");
        vk::DeviceSize imageSize = width * height * depth * 4;

        if (format == vk::Format::eR8Unorm)
        {
            imageSize = width * height * depth;
        }
        if (isHDR)
        {
            imageSize *= sizeof(float);
        }

        vk::Buffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;

        {
            ZoneScopedN("Image buffer allocation");
            util::CreateBuffer(_context, imageSize, vk::BufferUsageFlagBits::eTransferSrc, stagingBuffer, true, stagingBufferAllocation, VMA_MEMORY_USAGE_CPU_ONLY, "Texture staging buffer");
            vmaCopyMemoryToAllocation(_context->MemoryAllocator(), creation.initialData.data(), stagingBufferAllocation, 0, imageSize);
        }

        vk::ImageLayout oldLayout = vk::ImageLayout::eTransferDstOptimal;

        if (commands)
        {
            ZoneScopedN("Command upload dispatch");
            const vk::CommandBuffer& commandBuffer = commands->CommandBuffer();

            util::TransitionImageLayout(commandBuffer, image, format, vk::ImageLayout::eUndefined, oldLayout);

            util::CopyBufferToImage(commandBuffer, stagingBuffer, image, width, height);

            if (creation.mips > 1)
            {
                ZoneScopedN("Mip creation dispatch");
                util::TransitionImageLayout(commandBuffer, image, format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, 1, 0, 1);

                for (uint32_t i = 1; i < creation.mips; ++i)
                {
                    vk::ImageBlit blit {};
                    blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                    blit.srcSubresource.layerCount = 1;
                    blit.srcSubresource.mipLevel = i - 1;
                    blit.srcOffsets[1].x = width >> (i - 1);
                    blit.srcOffsets[1].y = height >> (i - 1);
                    blit.srcOffsets[1].z = 1;

                    blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                    blit.dstSubresource.layerCount = 1;
                    blit.dstSubresource.mipLevel = i;
                    blit.dstOffsets[1].x = width >> i;
                    blit.dstOffsets[1].y = height >> i;
                    blit.dstOffsets[1].z = 1;

                    util::TransitionImageLayout(commandBuffer, image, format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, i);

                    commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

                    util::TransitionImageLayout(commandBuffer, image, format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, 1, i);
                }
                oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            }

            util::TransitionImageLayout(commandBuffer, image, format, oldLayout, vk::ImageLayout::eShaderReadOnlyOptimal, 1, 0, mips);

            commands->TrackAllocation(stagingBufferAllocation, stagingBuffer);
        }
        else
        {
            ZoneScopedN("Command upload dispatch");
            vk::CommandBuffer commandBuffer = util::BeginSingleTimeCommands(_context);

            util::TransitionImageLayout(commandBuffer, image, format, vk::ImageLayout::eUndefined, oldLayout);

            util::CopyBufferToImage(commandBuffer, stagingBuffer, image, width, height);

            if (creation.mips > 1)
            {
                ZoneScopedN("Mip creation dispatch");
                util::TransitionImageLayout(commandBuffer, image, format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, 1, 0, 1);

                for (uint32_t i = 1; i < creation.mips; ++i)
                {
                    vk::ImageBlit blit {};
                    blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                    blit.srcSubresource.layerCount = 1;
                    blit.srcSubresource.mipLevel = i - 1;
                    blit.srcOffsets[1].x = width >> (i - 1);
                    blit.srcOffsets[1].y = height >> (i - 1);
                    blit.srcOffsets[1].z = 1;

                    blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                    blit.dstSubresource.layerCount = 1;
                    blit.dstSubresource.mipLevel = i;
                    blit.dstOffsets[1].x = width >> i;
                    blit.dstOffsets[1].y = height >> i;
                    blit.dstOffsets[1].z = 1;

                    util::TransitionImageLayout(commandBuffer, image, format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, i);

                    commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

                    util::TransitionImageLayout(commandBuffer, image, format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, 1, i);
                }
                oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            }

            util::TransitionImageLayout(commandBuffer, image, format, oldLayout, vk::ImageLayout::eShaderReadOnlyOptimal, 1, 0, mips);

            {
                ZoneScopedN("Waiting Image upload");
                util::EndSingleTimeCommands(_context, commandBuffer);
            }

            util::vmaDestroyBuffer(_context->MemoryAllocator(), stagingBuffer, stagingBufferAllocation);
        }
    }

    {
        ZoneScopedN("Name Settings");
        if (!creation.name.empty())
        {
            std::stringstream ss {};
            ss << "[IMAGE] ";
            ss << creation.name;
            std::string imageStr = ss.str();
            util::NameObject(image, imageStr, _context);
            ss.str("");

            for (size_t i = 0; i < imageCreateInfo.arrayLayers; ++i)
            {
                ss << "[VIEW " << i << "] ";
                ss << creation.name;
                std::string viewStr = ss.str();
                util::NameObject(layerViews[i].view, viewStr, _context);
                ss.str("");
            }

            ss << "[ALLOCATION] ";
            ss << creation.name;
            std::string str = ss.str();
            vmaSetAllocationName(_context->MemoryAllocator(), allocation, str.c_str());
        }
        else
        {
            bblog::warn("Creating an unnamed image!");
        }
    }
}

GPUImage::~GPUImage()
{
    if (!_context)
    {
        return;
    }

    util::vmaDestroyImage(_context->MemoryAllocator(), image, allocation);
    for (auto& layer : layerViews)
    {
        _context->Device().destroy(layer.view);
        for (auto& mipView : layer.mipViews)
        {
            _context->Device().destroy(mipView);
        }
    }
    if (type == ImageType::eCubeMap)
        _context->Device().destroy(view);
}

GPUImage::GPUImage(GPUImage&& other) noexcept
    : image(other.image)
    , layerViews(std::move(other.layerViews))
    , view(other.view)
    , allocation(other.allocation)
    , sampler(other.sampler)
    , width(other.width)
    , height(other.height)
    , depth(other.depth)
    , layers(other.layers)
    , mips(other.mips)
    , flags(other.flags)
    , isHDR(other.isHDR)
    , type(other.type)
    , format(other.format)
    , vkType(other.vkType)
    , name(std::move(other.name))
    , _context(other._context)
{
    other.image = nullptr;
    other.view = nullptr;
    other.allocation = nullptr;
    other._context = nullptr;
}

GPUImage& GPUImage::operator=(GPUImage&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    image = other.image;
    layerViews = std::move(other.layerViews);
    view = other.view;
    allocation = other.allocation;
    sampler = other.sampler;
    width = other.width;
    height = other.height;
    depth = other.depth;
    layers = other.layers;
    mips = other.mips;
    flags = other.flags;
    isHDR = other.isHDR;
    type = other.type;
    format = other.format;
    vkType = other.vkType;
    name = std::move(other.name);
    _context = other._context;

    other.image = nullptr;
    other.view = nullptr;
    other.allocation = nullptr;
    other._context = nullptr;

    return *this;
}