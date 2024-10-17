#pragma once

// Class Decorations

#define NON_COPYABLE(ClassName)           \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete;

#define NON_MOVABLE(ClassName)       \
    ClassName(ClassName&&) = delete; \
    ClassName& operator=(ClassName&&) = delete;

// Attribute macros

#define MAYBE_UNUSED [[maybe_unused]]
#define NO_DISCARD [[nodiscard]]

// System Macro definitions

#ifdef _WIN32
#define WINDOWS
#elif __linux__
#define LINUX
#endif

// override new and delete for Tracy Profiling

// TODO: Tracy inclusion should probably be under a build system option
// #if defined(TRACY_PROFILE)

#include <cstddef>

void* operator new(size_t size);
void operator delete(void* ptr) noexcept;

void* operator new[](size_t size);
void operator delete[](void* ptr) noexcept;

// Sized variants: preferred by C++ 14 (but they are identical to the normal deletes)
void operator delete(void* ptr, size_t) noexcept;
void operator delete[](void* ptr, size_t) noexcept;

// #endif