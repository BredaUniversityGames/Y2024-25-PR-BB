#include "gpu_resources.hpp"

#include "profile_macros.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

GPUMaterial::GPUMaterial(const MaterialCreation& creation, const std::shared_ptr<ResourceManager<GPUImage>>& imageResourceManager)
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

GPUMaterial::~GPUMaterial()
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

    util::vmaDestroyBuffer(_context->MemoryAllocator(), buffer, allocation);
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
