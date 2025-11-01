#pragma once

#include <array>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "utils/storage_config.hpp"
#include "utils/storage_strings.hpp"

namespace storage::details {

template <typename T>
struct Getter;

template <typename... Ts>
struct Getter<tuple_type<Ts...>> {
 private:
  static constexpr size_type kTupleSize = sizeof...(Ts);
  static constexpr const char* kIndexOutOfRangeMessage = "Bad index ;(";
  static constexpr const char* kNameOutOfRangeMessage = "Bad name ;(";

  using current_tuple_type = tuple_type<Ts...>;
  using getter_function_type = text_type (*)(const current_tuple_type& tuple);
  using table_by_index_type = std::array<getter_function_type, kTupleSize>;
  using table_by_name_type =
      std::unordered_map<cesrt_type, getter_function_type>;

 public:
  static text_type get(const current_tuple_type& tuple,
                       const size_type& index) {
    static auto table = []<size_type... Is>(std::index_sequence<Is...>) {
      return table_by_index_type{
          +[](const current_tuple_type& tuple) -> text_type {
            return details::to_string(std::get<Is>(tuple).value);
          }...};
    }(std::make_index_sequence<kTupleSize>{});
    if (index >= kTupleSize) {
      throw std::out_of_range(kIndexOutOfRangeMessage);
    }
    return table[index](tuple);
  }

  static text_type get(const current_tuple_type& tuple,
                       const cesrt_type& name) {
    static auto table = []<size_type... Is>(std::index_sequence<Is...>) {
      return table_by_name_type{
          {std::tuple_element_t<Is, current_tuple_type>::kName,
           +[](const current_tuple_type& tuple) -> text_type {
             return details::to_string(std::get<Is>(tuple).value);
           }}...};
    }(std::make_index_sequence<kTupleSize>{});
    if (!table.contains(name)) {
      throw std::out_of_range(kNameOutOfRangeMessage);
    }
    return table[name](tuple);
  }
};

}  // namespace storage::details