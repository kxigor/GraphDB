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

struct MappingAll {
  static void init_mapping(StorageFile& file, size_type /*unused*/) {
    file.start_mapping(file.get_file_size());
  }
};
struct MappingOnlyPodStructs {
  static void init_mapping(StorageFile& file, size_type /*unused*/) {
    file.start_mapping(file.get_file_size());
  }
};

template <typename MappingPolicy, typename... PodStructs>
struct PodFileAdapter {
  /*======================== Constants/Usings =========================*/
 private:
  using payloadinfo_type = pod::PayloadInfo;
  using original_types = details::TypeList<PodStructs...>;
  using pod_structs_type = tuple_type<PodStructs*...>;
  using mapping_policy = MappingPolicy;

  static constexpr const size_type kOverallSize = (sizeof(PodStructs) + ...);

 public:
  /*==================== Constructors/Destructors =====================*/
  PodFileAdapter(const path_type& path) {
    const bool kIsExist = std::filesystem::exists(path);
    file.open(path);
    if (not kIsExist) {
      file.truncate(kOverallSize);
    }
    init_mapping();
  }

  // NOLINTBEGIN
  ~PodFileAdapter() {
    file.end_mapping_if_mapped();
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
  void init_mapping() {
    mapping_policy::init_mapping(file, kOverallSize);
    init_pod_pointers();
  }

  void update_mapping_with_file_size() {
    file.update_mapping(file.get_file_size());
    init_pod_pointers();
  }

  void init_pod_pointers() {
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
    : public PodFileAdapter<
          MappingOnlyPodStructs, pod::PayloadInfo,
          pod::KeylessMap<file_ptr_type, kDefaultMapCapacity>> {
  /*======================== Constants/Usings =========================*/
  using base_adapter_type =
      PodFileAdapter<MappingOnlyPodStructs, pod::PayloadInfo,
                     pod::KeylessMap<file_ptr_type, kDefaultMapCapacity>>;
  using map_type = pod::KeylessMap<file_ptr_type, kDefaultMapCapacity>;

  auto get_equal_pred() {
    return [&](const auto& key, size_type offset) mutable {
      file.set_map_pos(offset);
      file.inc_map_pos(sizeof(id_type));
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
    file.set_map_pos(finded_offset.value());
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
    const auto kOldFileSize = file.get_file_size();
    get_struct<map_type>()->insert(key, hashfunc, kOldFileSize,
                                   get_equal_pred());
    PrimitiveBuffer buf;
    serialize_type(buf, id);
    serialize_type(buf, key);
    file.append(buf.get_data(), buf.get_size());
  }

  void erase(const auto& key, auto& hashfunc) {
    get_struct<map_type>()->erase(key, hashfunc, get_equal_pred());
  }
};

struct TypePolicy {
  template <WriteReadBuffer BufferType>
  static void serialize(BufferType& buffer, const auto& value) {
    serialize_type(buffer, value);
  }
  template <WriteReadBuffer BufferType>
  static void deserialize(BufferType& buffer, auto& value) {
    deserialize_type(buffer, value);
  }
};

struct TuplePolicy {
  template <WriteReadBuffer BufferType>
  static void serialize(BufferType& buffer, const auto& value) {
    serialize_tuple(buffer, value);
  }
  template <WriteReadBuffer BufferType>
  static void deserialize(BufferType& buffer, auto& value) {
    deserialize_tuple(buffer, value);
  }
};

/*TODO: occupied flags*/
template <typename Policy, typename ValueType, size_type ArrSize>
class BaseArrayFileAdapter
    : public PodFileAdapter<MappingOnlyPodStructs, pod::PayloadInfo,
                            pod::MarkedArray<file_ptr_type, ArrSize>> {
  /*============================= Usings ==============================*/
  using base_adapter_type =
      PodFileAdapter<MappingOnlyPodStructs, pod::PayloadInfo,
                     pod::MarkedArray<file_ptr_type, ArrSize>>;
  using array_type = pod::MarkedArray<file_ptr_type, ArrSize>;

 public:
  using policy_type = Policy;
  using value_type = ValueType;
  using poses_type = container_type<size_type>;
  using poses_with_values_type =
      container_type<pair_type<size_type, value_type>>;
  using values_type = container_type<value_type>;

  /*========================== Constructors ===========================*/
  using base_adapter_type::base_adapter_type;

  /*============================ Modifiers ============================*/
  void set_value(size_type pos, const value_type& value) {
    multiset_values({{pos, value}});
  }

  void multiset_values(const poses_with_values_type& poses_with_values) {
    PrimitiveBuffer buffer;
    auto old_buffer_size = buffer.get_size();
    const auto kOldFileSize = this->file.get_file_size();
    for (const auto& [pos, value] : poses_with_values) {
      policy_type::serialize(buffer, value);
      set_offset(pos, kOldFileSize + old_buffer_size);
      old_buffer_size = buffer.get_size();
    }
    this->file.append(buffer.get_data(), buffer.get_size());
  }

  /*============================= LookUp ==============================*/
  [[nodiscard]] value_type get_value(size_type pos) {
    return std::move(multiget_values({pos}).front());
  }

  [[nodiscard]] values_type multiget_values(const poses_type& poses) {
    const auto kPosesValuesSize = poses.size();
    values_type values(kPosesValuesSize);
    for (size_type i = 0; i < kPosesValuesSize; ++i) {
      const auto kOffset = get_offset(poses.at(i));
      this->file.set_map_pos(kOffset);
      policy_type::deserialize(this->file, values.at(i));
    }
    return values;
  }

 private:
  /*============================== Impls ==============================*/
  [[nodiscard]] file_ptr_type get_offset(size_type pos) const {
    return this->template get_struct<array_type>()->get_value(pos);
  }

  void set_offset(size_type pos, file_ptr_type new_offset) {
    this->template get_struct<array_type>()->set_value(pos, new_offset);
  }
};

// NOLINTBEGIN
template <typename KeyType>
using KeyFileAdapter =
    BaseArrayFileAdapter<TypePolicy, KeyType, kDefaultKeyArrayCapacity>;

template <typename VertexType>
using VertexFileAdapter =
    BaseArrayFileAdapter<TuplePolicy, VertexType, kDefaultVertexArrayCapacity>;
// NOLINTEND

struct ForwardEdgePolicy {
  static void skip_data(files::StorageFile& file) {
    file.inc_map_pos(sizeof(file_offset_type));
    file.follow();
  }

  static void skip_size(files::StorageFile& file) {
    file.inc_map_pos(sizeof(size_type));
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
    file.inc_map_pos(sizeof(file_offset_type));
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
class BaseEdgeFileAdapter
    : public PodFileAdapter<MappingAll, pod::PayloadInfo> {
  /*======================== Constants/Usings =========================*/
  enum class Status : char { Occupied, Deleted };
  using base_adapter_type = PodFileAdapter<MappingAll, pod::PayloadInfo>;
  using policy_type = Policy;

 public:
  using file_offset_type = size_type;

  /*==================== Constructors/Destructors =====================*/
  using base_adapter_type::base_adapter_type;

  template <typename... Args>
  void add_edge(const id_type& id, Args&&... args) {
    serialize_status_and_id(Status::Occupied, id);
    policy_type::serialize(file, std::forward<Args>(args)...);
    update_mapping_with_file_size();
  }

  template <typename... Ts>
  std::optional<id_type> get_next_id() {
    follow_to_next_occuipied_id();
    return get_id_impl<Ts...>();
  }

  template <typename... Ts>
  std::optional<id_type> get_next_id_by_file_position(
      const file_offset_type& offset) {
    file.set_map_pos(offset);
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
    file.set_map_pos(offset);
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
    file.set_map_pos(offset);
    serialize_type(file, Status::Deleted);
  }

  [[nodiscard]] file_offset_type get_end_of_file_pos() {
    return file.get_file_size();
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
        return file.get_map_pos();
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
    auto id = deserialize_type_and_get<id_type>(file);
    auto data = policy_type::template deserialize<Ts...>(file);
    return std::tuple_cat(tuple_type{std::move(id)}, std::move(data));
  }

  void save_pos_of_begin_struct() { file.store_map_pos(); }

  void return_pos_to_begin_struct() { file.restore_map_pos(); }

  void follow_to_next_occuipied_id() {
    while (is_next_transition_exist()) {
      if (deserialize_status() == Status::Occupied) {
        break;
      }
      skip_id_and_data();
    }
  }

  bool is_next_transition_exist() {
    return file.get_map_pos() != file.get_file_size();
  }

  Status deserialize_status() { return deserialize_type_and_get<Status>(file); }
  id_type deserialize_id() { return deserialize_type_and_get<id_type>(file); }

  void skip_id_and_data() {
    file.inc_map_pos(sizeof(id_type));
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

class KeysConfigFileAdapter
    : public PodFileAdapter<MappingOnlyPodStructs, pod::KeysConfig> {
  using config_type = pod::KeysConfig;
  using base_adapter_type =
      PodFileAdapter<MappingOnlyPodStructs, pod::KeysConfig>;

 public:
  using base_adapter_type::base_adapter_type;

  [[nodiscard]] id_type generate_id() noexcept {
    return this->get_struct<config_type>()->max_id++;
  }
  [[nodiscard]] size_type get_number_of_maps() const noexcept {
    auto a = this->get_struct<config_type>()->max_id;
    std::ignore = a;
    return this->get_struct<config_type>()->number_of_maps;
  }
  void inc_number_of_maps(size_type off = 1) noexcept {
    this->get_struct<config_type>()->number_of_maps += off;
  }
  void dec_number_of_maps(size_type off = 1) noexcept {
    this->get_struct<config_type>()->number_of_maps -= off;
  }
};

class VertexesConfigFileAdapter
    : public PodFileAdapter<MappingOnlyPodStructs, pod::VertexesConfig> {
  using config_type = pod::VertexesConfig;
  using base_adapter_type =
      PodFileAdapter<MappingOnlyPodStructs, pod::VertexesConfig>;

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

class EdgesConfigFileAdapter
    : public PodFileAdapter<MappingOnlyPodStructs, pod::EdgesConfig> {
  using config_type = pod::EdgesConfig;
  using base_adapter_type =
      PodFileAdapter<MappingOnlyPodStructs, pod::EdgesConfig>;

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
