#pragma once

#include <concepts>
#include <cstring>
#include <string>
#include <vector>

#include "utils/storage_config.hpp"

namespace storage {

template <typename T>
concept WriteReadBuffer =
    requires(T& buf, const char* src, char* dst, size_type size) {
      { buf.write(src, size) } -> std::same_as<void>;
      { buf.read(dst, size) } -> std::same_as<void>;
    };

struct PrimitiveBuffer {
  using base_container_type = std::vector<char>;

  void write(const void* buffer, size_type size) {
    buffer_.resize(buffer_.size() + size);
    memcpy(&buffer_[buffer_.size() - size], buffer, size);
  }

  void read(void* buffer, size_type size) {
    memcpy(buffer, &buffer_[buffer_.size() - size], size);
    buffer_.resize(buffer_.size() - size);
  }

  [[nodiscard]] auto* get_data() noexcept { return buffer_.data(); }

  [[nodiscard]] auto get_size() const noexcept { return buffer_.size(); }

 private:
  base_container_type buffer_;
};

template <typename ValueType, WriteReadBuffer BufferType>
struct SerializeHelper {
  static void serialize(BufferType& dst, const ValueType& value) {
    dst.write(reinterpret_cast<const char*>(&value), sizeof(ValueType));
  }
  static void deserialize(BufferType& src, ValueType& value) {
    src.read(reinterpret_cast<char*>(&value), sizeof(ValueType));
  }
};

template <WriteReadBuffer BufferType>
struct SerializeHelper<std::string, BufferType> {
  static void serialize(BufferType& dst, const std::string& value) {
    SerializeHelper<size_type, BufferType>::serialize(dst, value.size());
    dst.write(value.data(), value.size());
  }
  static void deserialize(BufferType& src, std::string& value) {
    size_type size{};
    SerializeHelper<size_type, BufferType>::deserialize(src, size);
    value.resize(size);
    src.read(value.data(), value.size());
  }
};

template <WriteReadBuffer BufferType, typename ValueType>
void serialize_type(BufferType& dst, const ValueType& value) {
  SerializeHelper<ValueType, BufferType>::serialize(dst, value);
}

template <typename ValueType, WriteReadBuffer BufferType>
void deserialize_type(BufferType& src, ValueType& value) {
  SerializeHelper<ValueType, BufferType>::deserialize(src, value);
}

template <WriteReadBuffer BufferType, typename ValueType>
BufferType serialize_type_and_get(const ValueType& value) {
  BufferType buf;
  SerializeHelper<ValueType, BufferType>::serialize(buf, value);
  return buf;
}

template <typename ValueType, WriteReadBuffer BufferType>
ValueType deserialize_type_and_get(BufferType& src) {
  ValueType value;
  SerializeHelper<ValueType, BufferType>::deserialize(src, value);
  return value;
}

template <WriteReadBuffer BufferType, typename... Ts>
void serialize_tuple(BufferType& dst, const tuple_type<Ts...>& fields) {
  (serialize_type(dst, get<Ts>(fields).value) && ...);
}

template <WriteReadBuffer BufferType, typename... Ts>
void deserialize_tuple(BufferType& src, tuple_type<Ts...>& fields) {
  (deserialize_type<typename Ts::value_type>(src, get<Ts>(fields).value) &&
   ...);
}

};  // namespace storage