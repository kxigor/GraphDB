#pragma once

#include <concepts>
#include <cstring>
#include <string>
#include <vector>

#include "utils/storage_config.hpp"

namespace storage {

template <typename T>
concept WriteReadBuffer = requires(T& buf, const char* src, char* dst,
                                   size_type size) {
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

template <typename ValueType, WriteReadBuffer Buffer>
struct SerializeHelper {
  static void serialize(Buffer& dst, const ValueType& value) {
    dst.write(reinterpret_cast<const char*>(&value), sizeof(ValueType));
  }
  static void deserialize(Buffer& src, ValueType& value) {
    src.read(reinterpret_cast<char*>(&value), sizeof(ValueType));
  }
};

template <WriteReadBuffer Buffer>
struct SerializeHelper<std::string, Buffer> {
  static void serialize(Buffer& dst, const std::string& value) {
    SerializeHelper<size_type, Buffer>::serialize(dst, value.size());
    dst.write(value.data(), value.size());
  }
  static void deserialize(Buffer& src, std::string& value) {
    size_type size{};
    SerializeHelper<size_type, Buffer>::deserialize(src, size);
    value.resize(size);
    src.read(value.data(), value.size());
  }
};

template <WriteReadBuffer Buffer, typename ValueType>
void serialize_type(Buffer& dst, const ValueType& value) {
  SerializeHelper<ValueType, Buffer>::serialize(dst, value);
}

template <typename ValueType, WriteReadBuffer Buffer>
void deserialize_type(Buffer& src, ValueType& value) {
  SerializeHelper<ValueType, Buffer>::deserialize(src, value);
}

template <WriteReadBuffer Buffer, typename ValueType>
Buffer serialize_type_and_get(const ValueType& value) {
  Buffer buf;
  SerializeHelper<ValueType, Buffer>::serialize(buf, value);
  return buf;
}

template <typename ValueType, WriteReadBuffer Buffer>
ValueType deserialize_type_and_get(Buffer& src) {
  ValueType value;
  SerializeHelper<ValueType, Buffer>::deserialize(src, value);
  return value;
}

template <WriteReadBuffer Buffer, typename... Ts>
void serialize_tuple(Buffer& dst, const tuple_type<Ts...>& fields) {
  (serialize_type(dst, get<Ts>(fields).value), ...);
}

template <WriteReadBuffer Buffer, typename... Ts>
void deserialize_tuple(Buffer& src, tuple_type<Ts...>& fields) {
  (deserialize_type<typename Ts::value_type>(src, get<Ts>(fields).value), ...);
}

};  // namespace storage