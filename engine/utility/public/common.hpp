#pragma once

// Class Decorations

#define NON_COPYABLE(ClassName)           \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete;

#define NON_MOVABLE(ClassName)       \
    ClassName(ClassName&&) = delete; \
    ClassName& operator=(ClassName&&) = delete;

// System Macro definitions

#ifdef _WIN32
#define WINDOWS
#elif __linux__
#define LINUX
#endif

// Profiling override new and delete

// TODO: Tracy inclusion should probably be under a build system option
// #if defined(TRACY_PROFILE)

// #include <cstddef>

// void* operator new(size_t size);
// void operator delete(void* ptr) noexcept;
//
// void* operator new[](size_t size);
// void operator delete[](void* ptr) noexcept;

// #endif