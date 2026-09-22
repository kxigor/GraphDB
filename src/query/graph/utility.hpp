#pragma once

#include <iostream>

#include "parser/ast.hpp"

struct NotImplemented : std::exception {
  NotImplemented(const std::string& str) { what_ = "Not implemented: " + str; }

  virtual const char* what() const noexcept override { return what_.c_str(); }

  std::string what_;
};

struct InvalidRequest : std::exception {
  InvalidRequest(const std::string& str) { what_ = "Invalid request: " + str; }

  virtual const char* what() const noexcept override { return what_.c_str(); }

  std::string what_;
};

ast::Query DeserializeQuery() {
  ast::Query query;
  ba::binary_iarchive ar(std::cin);
  ar >> query;
  return std::move(query);
}

template <typename T>
T ConvertTo(const std::string& str) {
  return str;
}

template <>
int ConvertTo<int>(const std::string& str) {
  size_t pos = 0;
  int number;

  try {
    number = std::stoi(str, &pos);
  } catch (...) {
    throw InvalidRequest(str + " is not a number");
  }

  if (str[pos] != '\0') {
    throw InvalidRequest(str + " is not a number");
  }

  return number;
}

template <>
bool ConvertTo<bool>(const std::string& str) {
  if (str == "true") {
    return true;
  }

  if (str == "false") {
    return false;
  }

  throw InvalidRequest(str + " is not a bool");
}

template <typename Tuple, std::size_t Id = 0>
void AppendProperty(Tuple& tuple, const ast::Property& property) {
  if constexpr (Id != std::tuple_size_v<Tuple>) {
    if (property.identifier != std::get<Id>(tuple).kName) {
      return AppendProperty<Tuple, Id + 1>(tuple, property);
    }

    using type = decltype(std::get<Id>(tuple).value);
    std::get<Id>(tuple).value = ConvertTo<type>(property.property);
  } else {
    throw InvalidRequest("unknown property " + property.property);
  }
}

template <typename Tuple>
Tuple ConvertProperties(const ast::PropertyList& properties) {
  Tuple tuple;
  for (const auto& property : properties) {
    AppendProperty(tuple, property);
  }
  return tuple;
}

template <typename Tuple, typename F>
void ApplyToTuple(Tuple&& tuple, F&& func) {
  [&]<std::size_t... Is>(std::index_sequence<Is...>) {
    (std::invoke(std::forward<F>(func),
                 std::get<Is>(std::forward<Tuple>(tuple))),
     ...);
  }(std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>());
}