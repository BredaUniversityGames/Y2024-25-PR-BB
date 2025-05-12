#pragma once
#include "wren_common.hpp"
#include <magic_enum.hpp>

namespace detail
{
template <auto v>
auto ReturnVal() { return v; }

template <typename E, size_t... Idx>
void BindEnumSequence(wren::ForeignKlassImpl<E>& enumClass, std::index_sequence<Idx...>)
{
    constexpr auto names = magic_enum::enum_names<E>();
    constexpr auto values = magic_enum::enum_values<E>();
    ((enumClass.template funcStaticExt<ReturnVal<values[Idx]>>(std::string(names[Idx]))), ...);
}

template <typename E, size_t... Idx>
void BindBitflagSequence(wren::ForeignKlassImpl<E>& enumClass, std::index_sequence<Idx...>)
{
    constexpr auto names = magic_enum::enum_names<E>();
    constexpr auto values = magic_enum::enum_values<E>();

    using UnderType = std::underlying_type_t<E>;
    ((enumClass.template funcStaticExt<ReturnVal<static_cast<UnderType>(values[Idx])>>(std::string(names[Idx]))), ...);
}

}

namespace bindings
{

template <typename E>
void BindEnum(wren::ForeignKlassImpl<E>& enumClass)
{
    constexpr auto enum_size = magic_enum::enum_count<E>();
    auto index_sequence = std::make_index_sequence<enum_size>();
    detail::BindEnumSequence<E>(enumClass, index_sequence);
}

template <typename E>
void BindEnum(wren::ForeignModule& module, const std::string& enumName)
{
    BindEnum<E>(module.klass<E>(enumName));
}

template <typename E>
void BindBitflagEnum(wren::ForeignModule& module, const std::string& enumName)
{
    constexpr auto enum_size = magic_enum::enum_count<E>();
    auto index_sequence = std::make_index_sequence<enum_size>();
    detail::BindBitflagSequence<E>(module.klass<E>(enumName), index_sequence);
}
}