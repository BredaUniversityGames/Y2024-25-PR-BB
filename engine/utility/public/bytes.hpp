#pragma once

#include <cstdint>

constexpr uint64_t operator"" _kb(uint64_t kilobytes)
{
    return kilobytes * 1024;
}

constexpr uint64_t operator"" _mb(uint64_t megabytes)
{
    return megabytes * 1024 * 1024;
}

constexpr uint64_t operator"" _gb(uint64_t gigabytes)
{
    return gigabytes * 1024 * 1024 * 1024;
}