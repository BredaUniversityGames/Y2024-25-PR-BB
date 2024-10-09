#include "common.hpp"
#include "log.hpp"
#include "profile_macros.hpp"

void* operator new(size_t size)
{
    bblog::info("Allocating memory for {} bytes", size);
    if (auto* ptr = std::malloc(size))
    {
        TracyAlloc(ptr, size);
        return ptr;
    }
    throw std::bad_alloc();
}

void* operator new[](size_t size)
{
    bblog::info("Allocating array memory for {} bytes", size);
    if (auto* ptr = std::malloc(size))
    {
        TracyAlloc(ptr, size);
        return ptr;
    }
    throw std::bad_alloc();
}

void operator delete(void* ptr) noexcept
{
    TracyFree(ptr);
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    TracyFree(ptr);
    std::free(ptr);
}