#pragma once

#include <cassert>
#include <optional>
#include <stdexcept>

#include "utils/storage_config.hpp"

namespace storage::pod {

struct KeysConfig {
  id_type max_id{};
  size_type number_of_maps{};
};

struct VertexesConfig {
  size_type number_of_vertexes{};
};

struct EdgesConfig {
  size_type number_of_edges{};
};

struct PayloadInfo {
  size_type information_amount;
  size_type useful_information_amount;
};

template <typename ValueType, size_type Size = kDefaultMapCapacity>
struct KeylessMap {
  static constexpr const float kDefaultMapLoadFactor = 0.75;

 private:
  /*==================== Constants/Usings/Classes =====================*/
  using value_type = ValueType;
  static constexpr const size_type kMapSize = Size;

  struct Bucket {
    enum class MapStatus : char { Free, Occupied, Deleted };

    [[nodiscard]] bool is_free() const noexcept {
      return status == MapStatus::Free;
    }

    [[nodiscard]] bool is_occupied() const noexcept {
      return status == MapStatus::Occupied;
    }

    [[nodiscard]] bool is_deleted() const noexcept {
      return status == MapStatus::Deleted;
    }

    void set_free_status() noexcept { status = MapStatus::Free; }

    void set_occupied_status() noexcept { status = MapStatus::Occupied; }

    void set_deleted_status() noexcept { status = MapStatus::Deleted; }

    ValueType value;
    MapStatus status;
  };

  struct FindResult {
    enum class FoundStatus : char { FoundOccupied, FoundFree, FoundNothing };

    [[nodiscard]] bool is_found_occupied() const noexcept {
      return status == FoundStatus::FoundOccupied;
    }

    [[nodiscard]] bool is_found_free() const noexcept {
      return status == FoundStatus::FoundFree;
    }

    [[nodiscard]] bool is_found_nothing() const noexcept {
      return status == FoundStatus::FoundNothing;
    }

    size_type position{};
    FoundStatus status{};
  };

 public:
  /*============================ Modifiers ============================*/
  void insert(const auto& key, auto&& hash_func, value_type value,
              auto&& equal_pred) {
    auto find_result = find_impl(key, hash_func, equal_pred);
    if (is_full() || find_result.is_found_nothing()) {
      throw std::out_of_range("Map is full, can't insert");
    }
    auto& bucket = buckets_[find_result.position];
    bucket.value = value;
    bucket.set_occupied_status();
    ++size_;
  }

  void erase(const auto& key, auto&& hash_func, auto&& equal_pred) {
    auto find_result = find_impl(key, hash_func, equal_pred);
    if (find_result.is_found_occupied()) {
      buckets_[find_result.position].set_deleted_status();
    }
  }

  void rehash_to_buffer(char* buffer, auto&& key_getter, auto&& hash_func,
                        auto&& equal_pred) {
    auto* new_map = reinterpret_cast<KeylessMap*>(buffer);
    for (const auto& bucket : buckets_) {
      if (bucket.is_occupied()) {
        new_map->insert(key_getter(bucket.value), hash_func, bucket.value,
                        equal_pred);
      }
    }
  }

  /*============================= LookUp ==============================*/
  optional_type<value_type> find(const auto& key, auto&& hash_func,
                                 auto&& equal_pred) const {
    auto find_result = find_impl(key, hash_func, equal_pred);
    if (find_result.is_found_occupied()) {
      return buckets_[find_result.position].value;
    }
    return std::nullopt;
  }

  [[nodiscard]] bool contains(const auto& key, auto&& hash_func,
                              auto&& equal_pred) const {
    return find(key, hash_func, equal_pred).has_value();
  }

  [[nodiscard]] bool is_full() const noexcept { return size_ == kMapSize; }

  [[nodiscard]] bool is_need_to_rehash() const noexcept {
    return static_cast<float>(size_) / static_cast<float>(kMapSize) >
           kDefaultMapLoadFactor;
  }

  [[nodiscard]] size_type size() const noexcept { return size_; }

 private:
  /*============================== Impls ==============================*/
  FindResult find_impl(const auto& key, auto&& hash_func,
                       auto&& equal_pred) const {
    size_type position = calculate_hash(key, hash_func);
    size_type difference = 0;
    for (size_type iterations_limit = 0; iterations_limit < kMapSize;
         ++iterations_limit) {
      if (buckets_[position].is_free()) {
        return {.position = position,
                .status = FindResult::FoundStatus::FoundFree};
      }
      if (buckets_[position].is_occupied() &&
          equal_pred(key, buckets_[position].value)) {
        return {.position = position,
                .status = FindResult::FoundStatus::FoundOccupied};
      }
      next(position, difference);
    }
    return {.status = FindResult::FoundStatus::FoundNothing};
  }

  static size_type calculate_hash(const auto& key, auto&& hash_func) noexcept {
    return hash_func(key) & (kMapSize - 1);
  }

  static void next(size_type& position, size_type& difference) noexcept {
    ++difference;
    position += difference;
    position &= kMapSize - 1;
  }

  /*============================= Fields ==============================*/
  size_type size_{};
  Bucket buckets_[kMapSize]{};
};

template <typename ValueType, size_type Size = kDefaultVertexArrayCapacity>
struct Array {
 private:
  /*======================== Constants/Usings =========================*/
  static constexpr const size_type kArraySize = Size;

 public:
  using value_type = ValueType;

  /*============================= LookUp ==============================*/
  value_type& operator[](size_type pos) { return data_[pos]; }
  const value_type& operator[](size_type pos) const { return data_[pos]; }
  [[nodiscard]] size_type size() const noexcept { return kArraySize; }

 private:
  /*============================= Fields ==============================*/
  value_type data_[kArraySize]{};
};

}  // namespace storage::pod