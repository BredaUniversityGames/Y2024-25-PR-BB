#pragma once

#include <cstdint>

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
