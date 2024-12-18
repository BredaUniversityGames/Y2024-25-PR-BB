#pragma once
#include "wren_common.hpp"
#include <magic_enum.hpp>

namespace detail
{
template <auto v>
auto ReturnVal() { return v; }

template <typename T, size_t... Idx>
void BindEnumSequence(wren::ForeignModule& module, const std::string& enumName, std::index_sequence<Idx...>)
{
    constexpr auto names = magic_enum::enum_names<T>();
    constexpr auto values = magic_enum::enum_values<T>();

    auto& enumClass = module.klass<T>(enumName);
    ((enumClass.template funcStaticExt<ReturnVal<values[Idx]>>(std::string(names[Idx]))), ...);
}

}

namespace bindings
{
template <typename T>
void BindEnum(wren::ForeignModule& module, const std::string& enumName)
{
    constexpr auto enum_size = magic_enum::enum_count<T>();
    auto index_sequence = std::make_index_sequence<enum_size>();
    detail::BindEnumSequence<T>(module, enumName, index_sequence);
}
}