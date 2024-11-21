
// WARNING: THIS HEADER IS ONLY MEANT TO BE INCLUDED ONCE IN main.cpp
// DEFINES OVERRIDES FOR TRACY MEMORY ALLOCATION TRACKING

#include "profile_macros.hpp"

void* operator new(size_t size)
{
    if (auto* ptr = std::malloc(size))
    {
        TracyAlloc(ptr, size);
        return ptr;
    }
    throw std::bad_alloc();
}

void* operator new[](size_t size)
{
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

void operator delete(void* ptr, size_t) noexcept
{
    TracyFree(ptr);
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    TracyFree(ptr);
    std::free(ptr);
}

void operator delete[](void* ptr, size_t) noexcept
{
    TracyFree(ptr);
    std::free(ptr);
}