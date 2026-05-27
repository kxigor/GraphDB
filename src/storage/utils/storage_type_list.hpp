#pragma once

#include <type_traits>

namespace storage::details {

template <typename... Ts>
struct TypeList {};

template <typename T, typename List>
struct IsTypeInList;

template <typename T, typename... Ts>
struct IsTypeInList<T, TypeList<Ts...>>
    : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

}  // namespace storage::details