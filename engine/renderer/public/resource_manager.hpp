#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "log.hpp"

template <typename T>
struct ResourceHandle;

template <typename T>
class ResourceManager;

template <typename T>
struct ResourceSlot
{
    std::optional<T> resource { std::nullopt };
    uint8_t version { 0 };
    uint32_t referenceCount { 0 };
};

template <typename T>
struct ResourceHandle final
{
    ResourceHandle()
        : index(RESOURCE_NULL_INDEX_VALUE)
        , version(0)
    {
    }

    ResourceHandle(uint32_t index, uint8_t version)
        : index(index)
        , version(version)
    {
    }

    ~ResourceHandle();
    ResourceHandle(const ResourceHandle<T>& other);
    ResourceHandle<T>& operator=(const ResourceHandle<T>& other);
    ResourceHandle(ResourceHandle<T>&& other) noexcept;
    ResourceHandle<T>& operator=(ResourceHandle<T>&& other) noexcept;
    ResourceHandle<T>& operator=(std::nullptr_t);

    static ResourceHandle<T> Null()
    {
        return ResourceHandle<T> {};
    }

    uint32_t Index() const { return index; }
    bool IsNull() const { return index == RESOURCE_NULL_INDEX_VALUE; }

private:
    friend class VulkanContext;
    friend ResourceManager<T>;

    const uint32_t RESOURCE_NULL_INDEX_VALUE = 0xFFFF;

    uint32_t index : 24 { 0 };
    uint32_t version : 8 { 0 };
    static std::weak_ptr<ResourceManager<T>> manager;
};

template <typename T>
class ResourceManager : public std::enable_shared_from_this<ResourceManager<T>>
{
public:
    virtual ~ResourceManager() = default;

    virtual ResourceHandle<T> Create(const T& resource)
    {
        uint32_t index {};
        if (!_freeList.empty())
        {
            index = _freeList.back();
            _freeList.pop_back();
        }
        else
        {
            index = _resources.size();
            _resources.emplace_back();
        }

        ResourceSlot<T>& resc = _resources[index];
        resc.resource = resource;

        ResourceHandle<T> handle(index, ++_resources[index].version);

        if (handle.manager.use_count() == 0)
        {
            handle.manager = this->shared_from_this();
        }

        return handle;
    }

    virtual const T* Access(ResourceHandle<T> handle) const
    {
        uint32_t index = handle.index;
        if (!IsValid(handle))
        {
            return nullptr;
        }

        return &_resources[index].resource.value();
    }

    virtual void Destroy(ResourceHandle<T> handle)
    {
        uint32_t index = handle.index;
        if (IsValid(handle))
        {
            _freeList.emplace_back(index);
            _resources[index].resource = std::nullopt;
        }
    }

    virtual bool IsValid(const ResourceHandle<T>& handle) const
    {
        uint32_t index = handle.index;
        return index < _resources.size() && _resources[index].version == handle.version && _resources[index].resource.has_value();
    }

    const std::vector<ResourceSlot<T>>& Resources() const { return _resources; }

protected:
    std::vector<ResourceSlot<T>> _resources;
    std::vector<uint32_t> _freeList;

private:
    friend struct ResourceHandle<T>;

    void IncrementReferenceCount(const ResourceHandle<T>& handle);
    void DecrementReferenceCount(const ResourceHandle<T>& handle);
};

template <typename T>
void ResourceManager<T>::IncrementReferenceCount(const ResourceHandle<T>& handle)
{
    if (handle.IsNull() || _resources[handle.index].version != handle.version)
    {
        return;
    }

    ++_resources[handle.index].referenceCount;
}

template <typename T>
void ResourceManager<T>::DecrementReferenceCount(const ResourceHandle<T>& handle)
{
    if (handle.IsNull() || _resources[handle.index].version != handle.version)
    {
        return;
    }

    --_resources[handle.index].referenceCount;
}

template <typename T>
ResourceHandle<T>::~ResourceHandle()
{
    if (auto mgr = manager.lock())
    {
        mgr->DecrementReferenceCount(*this);
    }
}

template <typename T>
ResourceHandle<T>::ResourceHandle(const ResourceHandle<T>& other)
    : index(other.index)
    , version(other.version)
{
    if (auto mgr = manager.lock())
    {
        mgr->IncrementReferenceCount(*this);
    }
}

template <typename T>
ResourceHandle<T>& ResourceHandle<T>::operator=(const ResourceHandle<T>& other)
{
    if (this == &other)
    {
        return *this;
    }

    if (auto mgr = manager.lock())
    {
        mgr->IncrementReferenceCount(*this);
    }

    index = other.index;
    version = other.version;

    return *this;
}

template <typename T>
ResourceHandle<T>::ResourceHandle(ResourceHandle<T>&& other) noexcept
    : index(other.index)
    , version(other.version)
{
    other = nullptr;
}

template <typename T>
ResourceHandle<T>& ResourceHandle<T>::operator=(ResourceHandle<T>&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    index = other.index;
    version = other.version;

    other = nullptr;

    return *this;
}

template <typename T>
ResourceHandle<T>& ResourceHandle<T>::operator=(std::nullptr_t)
{
    if (auto mgr = manager.lock())
    {
        mgr->DecrementReferenceCount(*this);
    }

    index = RESOURCE_NULL_INDEX_VALUE;
    version = 0;

    return *this;
}
