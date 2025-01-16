#pragma once

#include <cstdint>

constexpr uint64_t operator""_kb(unsigned long long kilobytes)
{
    return kilobytes * 1024;
}

constexpr uint64_t operator""_mb(unsigned long long megabytes)
{
    return megabytes * 1024 * 1024;
}

constexpr uint64_t operator""_gb(unsigned long long gigabytes)
{
    return gigabytes * 1024 * 1024 * 1024;
}