#include "gpu_resources.hpp"

#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

#include <iostream>

SamplerCreation& SamplerCreation::SetGlobalAddressMode(vk::SamplerAddressMode addressMode)
{
    addressModeU = addressMode;
    addressModeV = addressMode;
    addressModeW = addressMode;

    return *this;
}

Sampler::Sampler(const SamplerCreation& creation, const std::shared_ptr<VulkanContext>& context)
    : _context(context)
{
    vk::SamplerCreateInfo createInfo {};
    if (creation.useMaxAnisotropy)
    {
        auto properties = _context->PhysicalDevice().getProperties();
        createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    }

    createInfo.addressModeU = creation.addressModeU;
    createInfo.addressModeV = creation.addressModeV;
    createInfo.addressModeW = creation.addressModeW;
    createInfo.mipmapMode = creation.mipmapMode;
    createInfo.minLod = creation.minLod;
    createInfo.maxLod = creation.maxLod;
    createInfo.compareOp = creation.compareOp;
    createInfo.compareEnable = creation.compareEnable;
    createInfo.unnormalizedCoordinates = creation.unnormalizedCoordinates;
    createInfo.mipLodBias = creation.mipLodBias;
    createInfo.borderColor = creation.borderColor;
    createInfo.minFilter = creation.minFilter;
    createInfo.magFilter = creation.magFilter;

    sampler = _context->Device().createSampler(createInfo);

    util::NameObject(sampler, creation.name, _context);
}

Sampler::~Sampler()
{
    if (!_context)
    {
        return;
    }

    _context->Device().destroy(sampler);
}

Sampler::Sampler(Sampler&& other) noexcept
    : sampler(other.sampler)
    , _context(other._context)
{
    other.sampler = nullptr;
    other._context = nullptr;
}

Sampler& Sampler::operator=(Sampler&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    sampler = other.sampler;
    _context = other._context;

    other.sampler = nullptr;
    other._context = nullptr;

    return *this;
}

ImageCreation& ImageCreation::SetData(std::vector<std::byte> data)
{
    initialData = std::move(data);
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

ImageCreation& ImageCreation::SetSampler(ResourceHandle<Sampler> sampler)
{
    this->sampler = sampler;
    return *this;
}

vk::ImageType ImageTypeConversion(ImageType type)
{
    switch (type)
    {
    case ImageType::e2D:
    case ImageType::e2DArray:
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
    case ImageType::e2DArray:
        return vk::ImageViewType::e2DArray;
    case ImageType::eCubeMap:
        return vk::ImageViewType::eCube;
    default:
        throw std::runtime_error("Unsupported ImageType!");
    }
}

Image::~Image()
{
    if (!_context)
    {
        return;
    }

    vmaDestroyImage(_context->MemoryAllocator(), image, allocation);
    for (auto& aView : views)
        _context->Device().destroy(aView);
    if (type == ImageType::eCubeMap)
        _context->Device().destroy(view);
}
Image::Image(const ImageCreation& creation, const std::shared_ptr<VulkanContext>& context)
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
    sampler = creation.sampler;

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

    std::cout << "Creating image with dimensions: "
              << imageCreateInfo.extent.width << "x"
              << imageCreateInfo.extent.height << std::endl;
    imageCreateInfo.usage = creation.flags;

    if (creation.initialData.data())
        imageCreateInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;

    if (creation.type == ImageType::eCubeMap)
        imageCreateInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;

    VmaAllocationCreateInfo allocCreateInfo {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    vmaCreateImage(_context->MemoryAllocator(), (VkImageCreateInfo*)&imageCreateInfo, &allocCreateInfo, reinterpret_cast<VkImage*>(&image), &allocation, nullptr);
    std::string allocName = creation.name + " texture allocation";
    vmaSetAllocationName(_context->MemoryAllocator(), allocation, allocName.c_str());

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
        viewCreateInfo.subresourceRange.baseArrayLayer = i;
        vk::ImageView imageView;
        util::VK_ASSERT(_context->Device().createImageView(&viewCreateInfo, nullptr, &imageView), "Failed creating image view!");
        views.emplace_back(imageView);
    }
    view = *views.begin();

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
        vk::DeviceSize imageSize = width * height * depth * 4;
        if (isHDR)
            imageSize *= sizeof(float);

        vk::Buffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;

        util::CreateBuffer(_context, imageSize, vk::BufferUsageFlagBits::eTransferSrc, stagingBuffer, true, stagingBufferAllocation, VMA_MEMORY_USAGE_CPU_ONLY, "Texture staging buffer");

        vmaCopyMemoryToAllocation(_context->MemoryAllocator(), creation.initialData.data(), stagingBufferAllocation, 0, imageSize);

        vk::ImageLayout oldLayout = vk::ImageLayout::eTransferDstOptimal;

        vk::CommandBuffer commandBuffer = util::BeginSingleTimeCommands(_context);

        util::TransitionImageLayout(commandBuffer, image, format, vk::ImageLayout::eUndefined, oldLayout);

        util::CopyBufferToImage(commandBuffer, stagingBuffer, image, width, height);

        if (creation.mips > 1)
        {
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

        util::EndSingleTimeCommands(_context, commandBuffer);

        vmaDestroyBuffer(_context->MemoryAllocator(), stagingBuffer, stagingBufferAllocation);
    }

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
            util::NameObject(views[i], viewStr, _context);
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

Image::Image(Image&& other) noexcept
    : image(other.image)
    , views(std::move(other.views))
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

Image& Image::operator=(Image&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    image = other.image;
    views = std::move(other.views);
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

Material::Material(const MaterialCreation& creation, const std::shared_ptr<ResourceManager<Image>>& imageResourceManager)
    : _imageResourceManager(imageResourceManager)
{
    albedoMap = creation.albedoMap;
    mrMap = creation.metallicRoughnessMap;
    normalMap = creation.normalMap;
    occlusionMap = creation.occlusionMap;
    emissiveMap = creation.emissiveMap;

    gpuInfo.useAlbedoMap = _imageResourceManager->IsValid(albedoMap);
    gpuInfo.useMRMap = _imageResourceManager->IsValid(mrMap);
    gpuInfo.useNormalMap = _imageResourceManager->IsValid(normalMap);
    gpuInfo.useOcclusionMap = _imageResourceManager->IsValid(occlusionMap);
    gpuInfo.useEmissiveMap = _imageResourceManager->IsValid(emissiveMap);

    gpuInfo.albedoMapIndex = albedoMap.Index();
    gpuInfo.mrMapIndex = mrMap.Index();
    gpuInfo.normalMapIndex = normalMap.Index();
    gpuInfo.occlusionMapIndex = occlusionMap.Index();
    gpuInfo.emissiveMapIndex = emissiveMap.Index();

    gpuInfo.albedoFactor = creation.albedoFactor;
    gpuInfo.metallicFactor = creation.metallicFactor;
    gpuInfo.roughnessFactor = creation.roughnessFactor;
    gpuInfo.normalScale = creation.normalScale;
    gpuInfo.occlusionStrength = creation.occlusionStrength;
    gpuInfo.emissiveFactor = creation.emissiveFactor;
}

Material::~Material()
{
}

BufferCreation& BufferCreation::SetSize(vk::DeviceSize size)
{
    this->size = size;
    return *this;
}

BufferCreation& BufferCreation::SetUsageFlags(vk::BufferUsageFlags usage)
{
    this->usage = usage;
    return *this;
}

BufferCreation& BufferCreation::SetIsMappable(bool isMappable)
{
    this->isMappable = isMappable;
    return *this;
}

BufferCreation& BufferCreation::SetMemoryUsage(VmaMemoryUsage memoryUsage)
{
    this->memoryUsage = memoryUsage;
    return *this;
}

BufferCreation& BufferCreation::SetName(std::string_view name)
{
    this->name = name;
    return *this;
}

Buffer::Buffer(const BufferCreation& creation, const std::shared_ptr<VulkanContext>& context)
    : _context(context)
{
    util::CreateBuffer(_context,
        creation.size,
        creation.usage,
        buffer,
        creation.isMappable,
        allocation,
        creation.memoryUsage,
        creation.name);

    size = creation.size;
    usage = creation.usage;
    name = creation.name;

    if (creation.isMappable)
    {
        util::VK_ASSERT(vmaMapMemory(_context->MemoryAllocator(), allocation, &mappedPtr),
            "Failed mapping memory for buffer: " + creation.name);
    }
}

Buffer::~Buffer()
{
    if (!_context)
    {
        return;
    }

    if (mappedPtr)
    {
        vmaUnmapMemory(_context->MemoryAllocator(), allocation);
    }

    vmaDestroyBuffer(_context->MemoryAllocator(), buffer, allocation);
}

Buffer::Buffer(Buffer&& other) noexcept
    : buffer(other.buffer)
    , allocation(other.allocation)
    , mappedPtr(other.mappedPtr)
    , size(other.size)
    , usage(other.usage)
    , name(std::move(other.name))
    , _context(other._context)
{
    other.buffer = nullptr;
    other.allocation = nullptr;
    other.mappedPtr = nullptr;
    other._context = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    buffer = other.buffer;
    allocation = other.allocation;
    mappedPtr = other.mappedPtr;
    size = other.size;
    usage = other.usage;
    name = std::move(other.name);
    _context = other._context;

    other.buffer = nullptr;
    other.allocation = nullptr;
    other.mappedPtr = nullptr;
    other._context = nullptr;

    return *this;
}
