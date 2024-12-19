#pragma once

// Class Decorations

// NOLINTBEGIN
#define NON_COPYABLE(ClassName)           \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete;

#define NON_MOVABLE(ClassName)       \
    ClassName(ClassName&&) = delete; \
    ClassName& operator=(ClassName&&) = delete;
// NOLINTEND

// Attribute macros

#define MAYBE_UNUSED [[maybe_unused]]
#define NO_DISCARD [[nodiscard]]

// System Macro definitions

#ifdef _WIN32
#define WINDOWS
#elif __linux__
#define LINUX
#endif
