#pragma once

#include <type_traits>

#define GENERATE_ENUM_FLAG_OPERATORS(EnumType)                                                                                                                                                   \
    extern "C++"                                                                                                                                                                                 \
    {                                                                                                                                                                                            \
        inline EnumType operator|(EnumType a, EnumType b) { return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(a) | static_cast<std::underlying_type_t<EnumType>>(b)); } \
        inline EnumType operator&(EnumType a, EnumType b) { return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(a) & static_cast<std::underlying_type_t<EnumType>>(b)); } \
        inline EnumType operator~(EnumType a) { return static_cast<EnumType>(~static_cast<std::underlying_type_t<EnumType>>(a)); }                                                               \
        inline EnumType operator^(EnumType a, EnumType b) { return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(a) ^ static_cast<std::underlying_type_t<EnumType>>(b)); } \
    }

template <typename EnumType>
inline bool HasAnyFlags(EnumType lhs, EnumType rhs)
{
    return static_cast<int>(lhs & rhs) != 0;
}
