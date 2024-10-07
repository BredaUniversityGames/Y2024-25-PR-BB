#pragma clang diagnostic push

#pragma clang diagnostic ignored "-Wnullability-completeness"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-private-field"

#define VMA_IMPLEMENTATION
#define VMA_LEAK_LOG_FORMAT(format, ...)                 \
    do                                                   \
    {                                                    \
        spdlog::info(fmt::sprintf(format, __VA_ARGS__)); \
    } while (false)

#include "vk_mem_alloc.h"

#pragma clang diagnostic pop