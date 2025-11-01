#pragma once

#include <filesystem>
#include <optional>
#include <tuple>
#include <utility>

#include "files/storage_file.hpp"
#include "files/storage_serialize_type.hpp"
#include "storage_pod_structures.hpp"
#include "utils/storage_config.hpp"
#include "utils/storage_errors.hpp"
#include "utils/storage_type_list.hpp"

/*TODO: всем похуй на payloadinfo_type, а надо написать*/

namespace storage::files {

template <typename... PodStructs>
struct PodFileAdapter {
  /*======================== Constants/Usings =========================*/
 private:
  using payloadinfo_type = pod::PayloadInfo;
  using original_types = details::TypeList<PodStructs...>;
  using pod_structs_type = tuple_type<PodStructs*...>;

  static constexpr const size_type kOverallSize = (sizeof(PodStructs) + ...);

 public:
  /*==================== Constructors/Destructors =====================*/
  PodFileAdapter(const path_type& path) {
    const bool kIsExist = std::filesystem::exists(path);
    file.open(path);
    if (not kIsExist) {
      file.truncate(kOverallSize);
    }
    /*TODO in one func*/
    file.start_mapping();
    update_mapping();
  }

  // NOLINTBEGIN
  ~PodFileAdapter() {
    /*TODO: logging errors*/
    file.end_mapping();
    file.close();
  }
  // NOLINTEND

  PodFileAdapter() = delete;
  PodFileAdapter(const PodFileAdapter&) = delete;
  PodFileAdapter(PodFileAdapter&&) = delete;
  PodFileAdapter& operator=(const PodFileAdapter&) = delete;
  PodFileAdapter& operator=(PodFileAdapter&&) = delete;

  /*============================= LookUp ==============================*/
  template <typename PodStruct>
  [[nodiscard]] PodStruct* get_struct() {
    return std::get<PodStruct*>(pod_structs);
  }

  template <typename PodStruct>
  [[nodiscard]] const PodStruct* get_struct() const {
    return const_cast<PodFileAdapter*>(this)->get_struct<PodStruct>();
  }

  /*============================= Mapping =============================*/
  void update_mapping() {
    (
        [&]() {
          std::get<PodStructs*>(pod_structs) =
              file.map_pod_struct<PodStructs>();
        }(),
        ...);
  }

  /*============================= Fields ==============================*/
  pod_structs_type pod_structs;
  files::StorageFile file;
};

class MapFileAdapter
    : public PodFileAdapter<pod::PayloadInfo, pod::KeylessMap<file_ptr_type>> {
  /*======================== Constants/Usings =========================*/
  using base_adapter_type =
      PodFileAdapter<pod::PayloadInfo, pod::KeylessMap<file_ptr_type>>;
  using map_type = pod::KeylessMap<file_ptr_type>;

  auto get_equal_pred() {
    return [&](const auto& key, size_type offset) mutable {
      file.set_pos(offset);
      auto value = deserialize_type_and_get<pure_type<decltype(key)>>(file);
      return value == key;
    };
  }

 public:
  /*========================== Constructors ===========================*/
  using base_adapter_type::base_adapter_type;

  /*============================= LookUp ==============================*/
  template <typename KeyType, typename HashType>
  std::optional<id_type> find(const KeyType& key, HashType hashfunc) {
    auto finded_offset =
        get_struct<map_type>()->find(key, hashfunc, get_equal_pred());
    if (not finded_offset.has_value()) {
      return std::nullopt;
    }
    file.set_pos(finded_offset.value());
    std::ignore = deserialize_type_and_get<pure_type<decltype(key)>>(file);
    auto finded_id = deserialize_type_and_get<id_type>(file);
    return finded_id;
  }

  bool contains(const auto& key, auto& hashfunc) {
    return get_struct<map_type>()->contains(key, hashfunc, get_equal_pred());
  }

  size_type size() { return get_struct<map_type>()->size(); }

  bool is_full() { return get_struct<map_type>()->is_full(); }

  bool is_need_to_rehash() {
    return get_struct<map_type>()->is_need_to_rehash();
  }

  /*============================ Modifiers ============================*/
  void insert(const auto& key, auto& hashfunc, id_type id) {
    const auto kOldFileSize = file.get_size();
    PrimitiveBuffer buf;
    get_struct<map_type>()->insert(key, hashfunc, kOldFileSize,
                                   get_equal_pred());
    serialize_type(buf, key);
    serialize_type(buf, id);
    file.append(buf.get_data(), buf.get_size());
    file.update_mapping(kOldFileSize);
    file.set_pos(0);
    update_mapping();
  }

  void erase(const auto& key, auto& hashfunc) {
    get_struct<map_type>()->erase(key, hashfunc, get_equal_pred());
  }
};

struct TypePolicy {
  static void serialize(StorageFile& file, const auto& value) {
    PrimitiveBuffer buffer;
    serialize_type(buffer, value);
    file.append(buffer.get_data(), buffer.get_size());
  }
  static void deserialize(StorageFile& file, auto& value) {
    deserialize_type(file, value);
  }
};

struct TuplePolicy {
  static void serialize(StorageFile& file, const auto& value) {
    PrimitiveBuffer buffer;
    serialize_tuple(buffer, value);
    file.append(buffer.get_data(), buffer.get_size());
  }
  static void deserialize(StorageFile& file, auto& value) {
    deserialize_tuple(file, value);
  }
};

/*TODO: occupied flags*/
template <typename Policy, typename ValueType>
class BaseArrayFileAdapter
    : public PodFileAdapter<pod::PayloadInfo, pod::Array<file_ptr_type>> {
  /*============================= Usings ==============================*/
  using base_adapter_type =
      PodFileAdapter<pod::PayloadInfo, pod::Array<file_ptr_type>>;
  using array_type = pod::Array<file_ptr_type>;

 public:
  using policy_type = Policy;
  using value_type = ValueType;

  /*========================== Constructors ===========================*/
  using base_adapter_type::base_adapter_type;

  /*============================ Modifiers ============================*/
  /*WARNING: invalidate mapping*/
  void set_value(size_type pos, const value_type& value) {
    auto new_offset = file.get_size();
    auto old_offset = get_offset(pos);
    set_offset(pos, new_offset);
    file.set_pos(old_offset);
    policy_type::serialize(file, value);
  }

  /*============================= LookUp ==============================*/
  [[nodiscard]] ValueType get_value(size_type pos) {
    ValueType value{};
    auto offset = get_offset(pos);
    file.set_pos(offset);
    policy_type::deserialize(file, value);
    return value;
  }

 private:
  /*============================== Impls ==============================*/
  [[nodiscard]] file_ptr_type get_offset(size_type pos) const {
    return get_struct<array_type>()->operator[](pos);
  }

  void set_offset(size_type pos, file_ptr_type new_offset) {
    get_struct<array_type>()->operator[](pos) = new_offset;
  }
};

// NOLINTBEGIN
template <typename KeyType>
using KeyFileAdapter = BaseArrayFileAdapter<TypePolicy, KeyType>;

template <typename VertexType>
using VertexFileAdapter = BaseArrayFileAdapter<TuplePolicy, VertexType>;
// NOLINTEND

struct ForwardEdgePolicy {
  static void skip_data(files::StorageFile& file) {
    file.inc_pos(sizeof(file_offset_type));
    file.follow();
  }

  static void skip_size(files::StorageFile& file) {
    file.inc_pos(sizeof(size_type));
  }

  template <typename FileOffset, typename EdgeFields>
  requires std::is_same_v<FileOffset, file_offset_type>
  static tuple_type<FileOffset, EdgeFields> deserialize(
      files::StorageFile& file) {
    FileOffset file_offset{};
    deserialize_type(file, file_offset);

    skip_size(file);

    EdgeFields fields{};
    deserialize_tuple(file, fields);

    return tuple_type{file_offset, fields};
  }

  template <typename FileOffset, typename EdgeFields>
  requires std::is_same_v<FileOffset, file_offset_type>
  static void serialize(files::StorageFile& file, const FileOffset& file_offset,
                        const EdgeFields& fields) {
    PrimitiveBuffer fields_buffer;
    serialize_tuple(fields_buffer, fields);

    PrimitiveBuffer supportive_buffer;
    serialize_type(supportive_buffer, file_offset);
    serialize_type(supportive_buffer, fields_buffer.get_size());

    file.append(supportive_buffer.get_data(), supportive_buffer.get_size());
    file.append(fields_buffer.get_data(), fields_buffer.get_size());
  }
};

struct ReverseEdgePolicy {
  static void skip_data(files::StorageFile& file) {
    file.inc_pos(sizeof(file_offset_type));
  }

  template <typename FileOffset>
  requires std::is_same_v<FileOffset, file_offset_type>
  static tuple_type<FileOffset> deserialize(files::StorageFile& file) {
    return deserialize_type_and_get<FileOffset>(file);
  }

  template <typename FileOffset>
  requires std::is_same_v<FileOffset, file_offset_type>
  static void serialize(files::StorageFile& file,
                        const FileOffset& file_offset) {
    auto buf = serialize_type_and_get<PrimitiveBuffer>(file_offset);
    file.append(buf.get_data(), buf.get_size());
  }
};

template <typename Policy>
class BaseEdgeFileAdapter : public PodFileAdapter<pod::PayloadInfo> {
  /*======================== Constants/Usings =========================*/
  enum class Status : char { Occupied, Deleted };
  enum : char { Deleted, NotDeleted };
  using base_adapter_type = PodFileAdapter<pod::PayloadInfo>;
  using policy_type = Policy;

 public:
  using file_offset_type = size_type;

  /*==================== Constructors/Destructors =====================*/
  using base_adapter_type::base_adapter_type;

  template <typename... Args>
  void add_edge(const id_type& id, Args&&... args) {
    auto old_file_size = file.get_size();

    serialize_status_and_id(Status::Occupied, id);
    policy_type::serialize(file, std::forward<Args>(args)...);

    file.update_mapping(old_file_size);
    file.set_pos(0);
    update_mapping();
  }

  template <typename... Ts>
  std::optional<id_type> get_next_id() {
    follow_to_next_occuipied_id();
    return get_id_impl<Ts...>();
  }

  template <typename... Ts>
  std::optional<id_type> get_next_id_by_file_position(
      const file_offset_type& offset) {
    file.set_pos(offset);
    return get_id_impl<Ts...>();
  }

  template <typename... Ts>
  std::optional<tuple_type<id_type, Ts...>> get_next_edge() {
    follow_to_next_occuipied_id();
    return get_edge_impl<Ts...>();
  }

  template <typename... Ts>
  std::optional<tuple_type<id_type, Ts...>> get_edge_by_file_position(
      const file_offset_type& offset) {
    file.set_pos(offset);
    return get_edge_impl<Ts...>();
  }

  void delete_edge_if_exist(const id_type& id_to_delete) {
    if (auto finded_offset = find_edge(id_to_delete);
        finded_offset.has_value()) {
      delete_edge_by_offset(finded_offset.value());
    }
  }

  void delete_edge(const id_type& id_to_delete) {
    std::optional<file_offset_type> finded_offset;
    if (finded_offset = find_edge(id_to_delete);
        not finded_offset.has_value()) {
      throw engine::errors::BadEdge(id_to_delete);
    }
    delete_edge_by_offset(finded_offset.value());
  }

  void delete_edge_by_offset(const file_offset_type& offset) {
    file.set_pos(offset);
    serialize_type(file, Status::Deleted);
  }

  [[nodiscard]] file_offset_type get_end_of_file_pos() {
    return file.get_size();
  }

 private:
  std::optional<file_offset_type> find_edge(const id_type& id_to) {
    while (is_next_transition_exist()) {
      save_pos_of_begin_struct();
      if (deserialize_status() != Status::Occupied) {
        skip_id_and_data();
        continue;
      }
      if (deserialize_id() == id_to) {
        return_pos_to_begin_struct();
        return file.get_pos();
      }
      skip_data();
    }
    return std::nullopt;
  }

  template <typename... Ts>
  std::optional<id_type> get_id_impl() {
    if (not is_next_transition_exist()) {
      return std::nullopt;
    }
    id_type id = deserialize_type_and_get<id_type>(file);
    skip_data();
    return id;
  }

  template <typename... Ts>
  std::optional<tuple_type<id_type, Ts...>> get_edge_impl() {
    if (not is_next_transition_exist()) {
      return std::nullopt;
    }
    id_type id = deserialize_type_and_get<id_type>(file);
    return std::tuple_cat(tuple_type{id},
                          policy_type::template deserialize<Ts...>(file));
  }

  void save_pos_of_begin_struct() { file.store_pos(); }

  void return_pos_to_begin_struct() { file.restore_pos(); }

  void follow_to_next_occuipied_id() {
    while (is_next_transition_exist()) {
      if (deserialize_status() == Status::Occupied) {
        break;
      }
      skip_id_and_data();
    }
  }

  bool is_next_transition_exist() { return file.get_pos() != file.get_size(); }

  Status deserialize_status() { return deserialize_type_and_get<Status>(file); }
  id_type deserialize_id() { return deserialize_type_and_get<id_type>(file); }

  void skip_id_and_data() {
    file.inc_pos(sizeof(id_type));
    skip_data();
  }

  void skip_data() { policy_type::skip_data(file); }

  void serialize_status_and_id(const Status& status, const id_type& id) {
    PrimitiveBuffer buffer;
    serialize_type(buffer, status);
    serialize_type(buffer, id);
    file.append(buffer.get_data(), buffer.get_size());
  }
};

using ForwardEdgeFileAdapter =               // NOLINT
    BaseEdgeFileAdapter<ForwardEdgePolicy>;  // NOLINT

using BackwardEdgeFileAdapter =              // NOLINT
    BaseEdgeFileAdapter<ReverseEdgePolicy>;  // NOLINT

class KeysConfigFileAdapter : public PodFileAdapter<pod::KeysConfig> {
  using config_type = pod::KeysConfig;
  using base_adapter_type = PodFileAdapter<pod::KeysConfig>;

 public:
  using base_adapter_type::base_adapter_type;

  [[nodiscard]] id_type generate_id() noexcept {
    return this->get_struct<config_type>()->max_id++;
  }
  [[nodiscard]] size_type get_number_of_maps() const noexcept {
    return this->get_struct<config_type>()->number_of_maps;
  }
  void inc_number_of_maps(size_type off = 1) noexcept {
    this->get_struct<config_type>()->number_of_maps += off;
  }
  void dec_number_of_maps(size_type off = 1) noexcept {
    this->get_struct<config_type>()->number_of_maps -= off;
  }
};

class VertexesConfigFileAdapter : public PodFileAdapter<pod::VertexesConfig> {
  using config_type = pod::VertexesConfig;
  using base_adapter_type = PodFileAdapter<pod::VertexesConfig>;

 public:
  using base_adapter_type::base_adapter_type;

  [[nodiscard]] size_type get_number_of_vertexes() const noexcept {
    return this->get_struct<config_type>()->number_of_vertexes;
  }
  void inc_number_of_vertexes(size_type off = 1) noexcept {
    this->get_struct<config_type>()->number_of_vertexes += off;
  }
  void dec_number_of_vertexes(size_type off = 1) noexcept {
    this->get_struct<config_type>()->number_of_vertexes -= off;
  }
};

class EdgesConfigFileAdapter : public PodFileAdapter<pod::EdgesConfig> {
  using config_type = pod::EdgesConfig;
  using base_adapter_type = PodFileAdapter<pod::EdgesConfig>;

 public:
  using base_adapter_type::base_adapter_type;

  [[nodiscard]] size_type get_number_of_edges() const noexcept {
    return this->get_struct<config_type>()->number_of_edges;
  }
  void inc_number_of_edges(size_type off = 1) noexcept {
    this->get_struct<config_type>()->number_of_edges += off;
  }
  void dec_number_of_edges(size_type off = 1) noexcept {
    this->get_struct<config_type>()->number_of_edges -= off;
  }
};

}  // namespace storage::files