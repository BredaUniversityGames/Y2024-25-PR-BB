#if defined TRACY_ENABLE
#include "profile_macros.hpp"

void* operator new(size_t size)
{
    if (auto* ptr = std::malloc(size)) // NOLINT
    {
        TracyAlloc(ptr, size);
        return ptr;
    }
    throw std::bad_alloc();
}

void* operator new[](size_t size)
{
    if (auto* ptr = std::malloc(size)) // NOLINT
    {
        TracyAlloc(ptr, size);
        return ptr;
    }
    throw std::bad_alloc();
}

void operator delete(void* ptr) noexcept
{
    TracyFree(ptr);
    std::free(ptr); // NOLINT
}

void operator delete(void* ptr, size_t) noexcept
{
    TracyFree(ptr);
    std::free(ptr); // NOLINT
}

void operator delete[](void* ptr) noexcept
{
    TracyFree(ptr);
    std::free(ptr); // NOLINT
}

void operator delete[](void* ptr, size_t) noexcept
{
    TracyFree(ptr);
    std::free(ptr); // NOLINT
}
#endif