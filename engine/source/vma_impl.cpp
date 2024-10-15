#include "log.hpp"
#include <spdlog/fmt/bundled/printf.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#define VMA_IMPLEMENTATION
#define VMA_LEAK_LOG_FORMAT(format, ...)                 \
    do                                                   \
    {                                                    \
        bblog::error(fmt::sprintf(format, __VA_ARGS__)); \
    } while (false)

#include "vk_mem_alloc.h"

#pragma GCC diagnostic pop