#pragma once

#include <cstdint>

inline uint32_t DivideRoundingUp(
    uint32_t _dividend,
    uint32_t _divisor)
{
    return (_dividend + _divisor - 1) / _divisor;
}