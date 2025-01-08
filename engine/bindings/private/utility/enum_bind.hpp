#pragma once
#include "wren_common.hpp"
#include <magic_enum.hpp>

namespace detail
{
template <auto v>
auto ReturnVal() { return v; }

template <typename E, size_t... Idx>
void BindEnumSequence(wren::ForeignModule& module, const std::string& enumName, std::index_sequence<Idx...>)
{
    constexpr auto names = magic_enum::enum_names<E>();
    constexpr auto values = magic_enum::enum_values<E>();

    auto& enumClass = module.klass<E>(enumName);
    ((enumClass.template funcStaticExt<ReturnVal<values[Idx]>>(std::string(names[Idx]))), ...);
}

template <typename E, size_t... Idx>
void BindBitflagSequence(wren::ForeignModule& module, const std::string& enumName, std::index_sequence<Idx...>)
{
    constexpr auto names = magic_enum::enum_names<E>();
    constexpr auto values = magic_enum::enum_values<E>();

    auto& enumClass = module.klass<E>(enumName);
    using UnderType = std::underlying_type_t<E>;
    ((enumClass.template funcStaticExt<ReturnVal<static_cast<UnderType>(values[Idx])>>(std::string(names[Idx]))), ...);
}

}

namespace bindings
{
template <typename E>
void BindEnum(wren::ForeignModule& module, const std::string& enumName)
{
    constexpr auto enum_size = magic_enum::enum_count<E>();
    auto index_sequence = std::make_index_sequence<enum_size>();
    detail::BindEnumSequence<E>(module, enumName, index_sequence);
}

template <typename E>
void BindBitflagEnum(wren::ForeignModule& module, const std::string& enumName)
{
    constexpr auto enum_size = magic_enum::enum_count<E>();
    auto index_sequence = std::make_index_sequence<enum_size>();
    detail::BindBitflagSequence<E>(module, enumName, index_sequence);
}
}