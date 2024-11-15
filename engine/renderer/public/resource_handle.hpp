#pragma once

#include <cstdint>

template <typename T>
class ResourceManager;

template <typename T>
struct ResourceHandle
{
    ResourceHandle()
        : index(0xFFFF)
        , version(0)
    {
    }
    static ResourceHandle<T> Null() { return ResourceHandle<T> {}; }

    uint32_t Index() const { return index; }
    bool IsNull() const { return index == 0xFFFF; }

private:
    friend class VulkanContext;
    friend ResourceManager<T>;

    uint32_t index : 24 { 0 };
    uint32_t version : 8 { 0 };
};
