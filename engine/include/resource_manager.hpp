#pragma once

#include <optional>
#include <cstdint>
#include <vector>

template <typename T>
class ResourceManager;

template <typename T>
struct ResourceHandle
{

    ResourceHandle()
        : index(0xFFFFFF)
        , version(0)
    {
    }
    static ResourceHandle<T> Invalid() { return ResourceHandle<T> {}; }

    uint32_t index : 24 { 0 };

private:
    friend class VulkanBrain;
    friend ResourceManager<T>;
    uint32_t version : 8 { 0 };

};

template <typename T>
struct ResourceSlot
{
    std::optional<T> resource;
    uint8_t version;
};

template <typename T>
class ResourceManager
{
public:
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


        ResourceHandle<T> handle {};

        handle.index = index;
        handle.version = ++_resources[index].version;

        return handle;
    }

    virtual const T* Access(ResourceHandle<T> handle) const
    {
        uint32_t index = handle.index;

        if (!IsValid(handle))
            return nullptr;

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

    virtual bool IsValid(ResourceHandle<T> handle) const
    {
        uint32_t index = handle.index;

        return index < _resources.size() && _resources[index].version == handle.version && _resources[index].resource.has_value();
    }

    const std::vector<ResourceSlot<T>>& Resources() const { return _resources; }

protected:
    std::vector<ResourceSlot<T>> _resources;
    std::vector<uint32_t> _freeList;
};