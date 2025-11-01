#include <cassert>
#include <optional>
#include <stdexcept>

#include "utils/storage_config.hpp"
#include "utils/storage_errors.hpp"

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

template <typename ValueType>
struct Bucket {
  enum class MapStatus : char { Free, Occupied, Deleted };
  using value_type = ValueType;

  Bucket() = default;

  ~Bucket() { destroy_if_occupied(); }

  template <typename... Args>
  Bucket(Args&&... args) {
    construct(std::forward<Args>(args)...);
  }

  Bucket(const Bucket& other) : status_(other.status_) {
    if (is_occupied()) {
      construct(other.value_);
    }
  }

  Bucket(Bucket&& other) : status_(other.status_) {
    if (is_occupied()) {
      construct(std::move(other.value_));
      other.destroy();
    }
  }

  Bucket& operator=(const Bucket& other) {
    status_ = other.status_;
    if (is_occupied()) {
      emplace(other.value_);
    }
    return *this;
  }

  Bucket& operator=(Bucket&& other) {
    status_ = other.status_;
    if (is_occupied()) {
      emplace(std::move(other.value_));
    }
    return *this;
  }

  template <typename... Args>
  void emplace(Args&&... args) {
    destroy_if_occupied();
    construct(std::forward<Args>(args)...);
  }

  template <typename... Args>
  void construct(Args&&... args) {
    std::construct_at(std::addressof(value_), std::forward<Args>(args)...);
    set_occupied_status();
  }

  void destroy_if_occupied() {
    if (is_occupied()) {
      destroy();
    }
  }

  void destroy() {
    std::destroy_at(std::addressof(value_));
    set_deleted_status();
  }

  value_type& get_value() { return value_; }

  const value_type& get_value() const {
    return const_cast<Bucket*>(this)->get_value();
  }

  [[nodiscard]] bool is_free() const noexcept {
    return status_ == MapStatus::Free;
  }

  [[nodiscard]] bool is_occupied() const noexcept {
    return status_ == MapStatus::Occupied;
  }

  [[nodiscard]] bool is_deleted() const noexcept {
    return status_ == MapStatus::Deleted;
  }

 private:
  void set_free_status() noexcept { status_ = MapStatus::Free; }

  void set_occupied_status() noexcept { status_ = MapStatus::Occupied; }

  void set_deleted_status() noexcept { status_ = MapStatus::Deleted; }

  MapStatus status_;
  union {
    value_type value_;  // NOLINT
  };
};

template <typename ValueType, size_type Size = kDefaultMapCapacity>
struct KeylessMap {
  static constexpr const float kDefaultMapLoadFactor = 0.75;

 private:
  /*==================== Constants/Usings/Classes =====================*/
  using value_type = ValueType;
  using bucket_type = Bucket<value_type>;
  static constexpr const size_type kMapSize = Size;

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
    bucket.emplace(std::move(value));
    ++size_;
  }

  void erase(const auto& key, auto&& hash_func, auto&& equal_pred) {
    auto find_result = find_impl(key, hash_func, equal_pred);
    if (find_result.is_found_occupied()) {
      buckets_[find_result.position].destroy();
    }
  }

  void rehash_to_buffer(char* buffer, auto&& key_getter, auto&& hash_func,
                        auto&& equal_pred) {
    auto* new_map = reinterpret_cast<KeylessMap*>(buffer);
    for (const auto& bucket : buckets_) {
      if (bucket.is_occupied()) {
        const auto& bucket_value = bucket.get_value();
        new_map->insert(key_getter(bucket_value), hash_func, bucket_value,
                        equal_pred);
      }
    }
  }

  /*============================= LookUp ==============================*/
  optional_type<value_type> find(const auto& key, auto&& hash_func,
                                 auto&& equal_pred) const {
    auto find_result = find_impl(key, hash_func, equal_pred);
    if (find_result.is_found_occupied()) {
      const auto& bucket = buckets_[find_result.position];
      return bucket.get_value();
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
      auto& bucket = buckets_[position];
      if (bucket.is_free()) {
        return {.position = position,
                .status = FindResult::FoundStatus::FoundFree};
      }
      if (bucket.is_occupied() && equal_pred(key, bucket.get_value())) {
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
  bucket_type buckets_[kMapSize]{};
};

template <typename ValueType, size_type Size>
struct MarkedArray {
 private:
  /*======================== Constants/Usings =========================*/
  static constexpr const size_type kArraySize = Size;

 public:
  using value_type = ValueType;
  using bucket_type = Bucket<value_type>;

  /*============================= LookUp ==============================*/

  void set_value(size_type pos, value_type value) { at(pos).emplace(value); }

  const value_type& get_value(size_type pos) const {
    assert(has_value(pos));
    return at(pos).get_value();
  }

  [[nodiscard]] bool has_value(size_type pos) const {
    return at(pos).is_occupied();
  }

  [[nodiscard]] size_type size() const noexcept { return kArraySize; }

 private:
  bucket_type& at(size_type pos) {
    if (pos > kArraySize) {
      throw errors::ArrayOutOfRange(pos, size());
    }
    return this->operator[](pos);
  }

  const bucket_type& at(size_type pos) const {
    return const_cast<MarkedArray*>(this)->at(pos);
  }

  bucket_type& operator[](size_type pos) { return buckets_[pos]; }

  const bucket_type& operator[](size_type pos) const { return buckets_[pos]; }

  /*============================= Fields ==============================*/
  bucket_type buckets_[kArraySize]{};
};

}  // namespace storage::pod