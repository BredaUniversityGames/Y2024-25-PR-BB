#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "common.hpp"
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

constexpr uint32_t RESOURCE_NULL_INDEX_VALUE = 0xFFFF;

template <typename T>
struct ResourceHandle final
{
    ResourceHandle();

    ResourceHandle(std::weak_ptr<ResourceManager<T>> owner, uint32_t index, uint8_t version);

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

    std::weak_ptr<ResourceManager<T>> manager;
    uint32_t index { 0 };
    uint32_t version { 0 };
};

template <typename T>
class ResourceManager : public std::enable_shared_from_this<ResourceManager<T>>
{
public:
    ResourceManager() = default;
    virtual ~ResourceManager();

    ResourceHandle<T> Create(T&& resource)
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

        ResourceSlot<T>& slot = _resources[index];
        slot.resource = std::move(resource);

        auto self_ptr = ResourceManager<T>::shared_from_this();
        ResourceHandle<T> handle(self_ptr, index, slot.version);

        return handle;
    }

    const T* Access(const ResourceHandle<T>& handle) const
    {
        uint32_t index = handle.index;
        if (!IsValid(handle))
        {
            return nullptr;
        }

        return &_resources[index].resource.value();
    }

    const T* Access(uint32_t index) const
    {
        if (!IsValid(index))
        {
            return nullptr;
        }

        return &_resources[index].resource.value();
    }

    void Destroy(const ResourceHandle<T>& handle)
    {
        uint32_t index = handle.index;
        if (IsValid(handle))
        {
            _freeList.emplace_back(index);
            _resources[index].resource = std::nullopt;
            ++_resources[index].version;
        }
    }

    bool IsValid(const ResourceHandle<T>& handle) const
    {
        uint32_t index = handle.index;
        return index < _resources.size() && _resources[index].version == handle.version && _resources[index].resource.has_value();
    }

    bool IsValid(uint32_t index) const
    {
        return index < _resources.size() && _resources[index].resource.has_value();
    }

    void Clean()
    {
        for (const auto& handle : _bin)
        {
            Destroy(handle);
        }

        _bin.clear();
    }

    const std::vector<ResourceSlot<T>>& Resources() const { return _resources; }

protected:
    std::vector<ResourceSlot<T>> _resources;
    std::vector<uint32_t> _freeList;
    std::vector<ResourceHandle<T>> _bin;

private:
    friend struct ResourceHandle<T>;

    void IncrementReferenceCount(const ResourceHandle<T>& handle);
    void DecrementReferenceCount(const ResourceHandle<T>& handle);
};

template <typename T>
ResourceManager<T>::~ResourceManager()
{
}

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

    assert(_resources[handle.index].referenceCount != 0 && "Reference count can never be 0 before a decrement!");

    --_resources[handle.index].referenceCount;

    if (_resources[handle.index].referenceCount == 0)
    {
        _bin.emplace_back(handle);
    }
}

template <typename T>
ResourceHandle<T>::ResourceHandle()
    : ResourceHandle({}, RESOURCE_NULL_INDEX_VALUE, 0)
{
}

template <typename T>
ResourceHandle<T>::ResourceHandle(std::weak_ptr<ResourceManager<T>> owner, uint32_t index, uint8_t version)
    : manager(owner)
    , index(index)
    , version(version)
{
    if (auto mgr = manager.lock())
    {
        mgr->IncrementReferenceCount(*this);
    }
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
    : manager(other.manager)
    , index(other.index)
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
        mgr->DecrementReferenceCount(*this);
    }

    index = other.index;
    version = other.version;
    manager = other.manager;

    if (auto mgr = manager.lock())
    {
        mgr->IncrementReferenceCount(*this);
    }

    return *this;
}

template <typename T>
ResourceHandle<T>::ResourceHandle(ResourceHandle<T>&& other) noexcept
    : manager(other.manager)
    , index(other.index)
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

    if (auto mgr = manager.lock())
    {
        mgr->DecrementReferenceCount(*this);
    }

    index = other.index;
    version = other.version;
    manager = other.manager;

    other = nullptr;

    return *this;
}

template <typename T>
ResourceHandle<T>& ResourceHandle<T>::operator=(std::nullptr_t)
{
    manager = {};
    index = RESOURCE_NULL_INDEX_VALUE;
    version = 0;

    return *this;
}
