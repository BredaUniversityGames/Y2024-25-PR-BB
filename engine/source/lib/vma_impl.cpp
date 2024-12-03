#include "log.hpp"
#include <spdlog/fmt/bundled/printf.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wnullability-completeness"

#define VMA_IMPLEMENTATION
#define VMA_LEAK_LOG_FORMAT(format, ...)                 \
    do                                                   \
    {                                                    \
        bblog::error(fmt::sprintf(format, __VA_ARGS__)); \
    } while (false)

#include "vk_mem_alloc.h"

#pragma clang diagnostic pop