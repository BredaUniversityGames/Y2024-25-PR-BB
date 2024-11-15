#pragma once

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

    uint32_t Index() const { return index; }

private:
    friend class VulkanContext;
    friend ResourceManager<T>;

    uint32_t index : 24 { 0 };
    uint32_t version : 8 { 0 };
};
