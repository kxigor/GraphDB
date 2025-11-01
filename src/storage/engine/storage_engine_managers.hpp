#pragma once

#include <filesystem>
#include <optional>

#include "files/storage_file_adapters.hpp"
#include "files/storage_path_generator.hpp"
#include "utils/storage_config.hpp"

namespace storage::engine {

using namespace path_generator;

template <typename KeyType, typename HashType>
class KeyManager {
  /*============================= Usings ==============================*/

  using id_pos_type = FileArrayPos;
  using key_adapter = files::KeyFileAdapter<KeyType>;
  using map_adapter = files::MapFileAdapter;

 public:
  using key_type = KeyType;
  using hash_type = HashType;
  using keys_type = container_type<key_type>;
  using ids_type = container_type<id_type>;

  /*============================ Modifiers ============================*/
  id_type insert_key(const key_type& key) {
    const auto kMapNumber = find_or_create_map_number();
    const auto kMapPath = path_generator::generate_map_path(kMapNumber);
    const auto kNewId = config_.generate_id();
    const auto [kFileNum, kFilePos] = calculate_id_pos(kNewId);
    const auto kIdsPath = path_generator::generate_ids_path(kFileNum);
    map_adapter map_adapter(kMapPath);
    map_adapter.insert(key, hash_, kNewId);
    key_adapter key_adapter(kIdsPath);
    key_adapter.set_value(kFilePos, key);
    return kNewId;
  }

  id_type find_id_or_insert_key(const key_type& key) {
    if (auto find_id_result = find_id_by_key(key); find_id_result.has_value()) {
      return find_id_result.value();
    }
    return insert_key(key);
  }

  void erase_key(const key_type& key) {
    for (size_type i = 0; i < config_.get_number_of_maps(); ++i) {
      files::MapFileAdapter map_adapter(path_generator::generate_map_path(i));
      if (auto result = map_adapter.find(key, hash_); result.has_value()) {
        map_adapter.erase(key, hash_);
      }
    }
  }

  /*============================= LookUp ==============================*/
  optional_type<id_type> find_id_by_key(const key_type& key) {
    for (size_type i = 0; i < config_.get_number_of_maps(); ++i) {
      files::MapFileAdapter map_adapter(path_generator::generate_map_path(i));
      if (auto result = map_adapter.find(key, hash_); result.has_value()) {
        return result.value();
      }
    }
    return std::nullopt;
  }

  key_type find_key_by_id(const id_type& id) {
    const auto [kFileNum, kFilePos] = calculate_id_pos(id);
    const auto kKeyFilePath = generate_ids_path(kFileNum);
    key_adapter keys(kKeyFilePath);
    return keys.get_value(kFilePos);
  }

  ids_type find_keys_by_ids(const keys_type& keys);

  keys_type find_ids_by_keys(const ids_type& ids);

 private:
  /*============================== Impls ==============================*/
  size_type find_or_create_map_number() {
    size_type map_number{};
    if (auto find_map_number_result = find_unfilled_map_number();
        find_map_number_result.has_value()) {
      map_number = find_map_number_result.value();
    } else {
      map_number = config_.get_number_of_maps();
      config_.inc_number_of_maps();
    }
    return map_number;
  }

  optional_type<size_type> find_unfilled_map_number() {
    const size_type kTotalMaps = config_.get_number_of_maps();
    for (size_type current_map = 0; current_map < kTotalMaps; ++current_map) {
      const auto kMapPath = path_generator::generate_map_path(current_map);
      files::MapFileAdapter map_adapter(kMapPath);
      if (not map_adapter.is_need_to_rehash()) {
        return current_map;
      }
    }
    return std::nullopt;
  }

  id_pos_type calculate_id_pos(const id_type& id) {
    /*TODO: add new constant*/
    return {.file_num = id / kDefaultKeyArrayCapacity,
            .file_pos = id % kDefaultKeyArrayCapacity};
  }

  /*============================= Fields ==============================*/
  hash_type hash_{};
  files::KeysConfigFileAdapter config_{kKeysConfigFileName};
};

template <typename VertexType>
class VertexManager {
  /*============================= Usings ==============================*/
  using vertex_pos = FileArrayPos;
  using vertex_type = VertexType;
  using vertex_adapter_type = files::VertexFileAdapter<VertexType>;

 public:
  /*============================ Modifiers ============================*/
  void add_vertex(const id_type& id, const vertex_type& vertex_fields) {
    perform_on_vertex(id, [&](auto& vertexes, auto& pos_in_file) {
      vertexes.set_value(pos_in_file, vertex_fields);
    });
  }

  /*============================= LookUp ==============================*/
  VertexType get_vertex(const id_type& id) {
    return perform_on_vertex(id, [&](auto& vertexes, auto& pos_in_file) {
      return vertexes.get_value(pos_in_file);
    });
  }

 private:
  /*============================== Impls ==============================*/
  auto perform_on_vertex(const id_type& id, auto&& action) {
    const auto [kFileNum, kFilePos] = calculate_vertex_pos(id);
    const auto kVertexesFilePath = generate_vertex_path(kFileNum);
    vertex_adapter_type vertexes(kVertexesFilePath);
    return std::forward<decltype(action)>(action)(vertexes, kFilePos);
  }

  static vertex_pos calculate_vertex_pos(const id_type& id) noexcept {
    return {.file_num = id / kDefaultVertexArrayCapacity,
            .file_pos = id % kDefaultVertexArrayCapacity};
  }

  /*============================= Fields ==============================*/
  files::VertexesConfigFileAdapter config_{kVertexesConfigFileName};
};

template <typename EdgeType>
class EdgeManager {
  /*============================= Usings ==============================*/
  using ids_edges_type = typename EdgeUsings<EdgeType>::ids_edges_type;

  using forward_edge_adapter = files::ForwardEdgeFileAdapter;
  using backward_edge_adapter = files::BackwardEdgeFileAdapter;
  enum class EdgeStatus : char { Forward, Backward };

 public:
  using edge_type = EdgeType;

  /*============================ Modifiers ============================*/
  /*TODO: add perfect forwarding*/
  void add_edge(const id_type& id_from, const id_type& id_to,
                const EdgeType& edge_fields) {
    const auto kForwardEdgesPath = generate_forward_edge_path(id_from);
    const auto kBackwardEdgesPath = generate_backward_edge_path(id_to);
    forward_edge_adapter forward_edges(kForwardEdgesPath);
    backward_edge_adapter reverse_edges(kBackwardEdgesPath);
    const auto kEdgesLastPos = forward_edges.get_end_of_file_pos();
    const auto kReverseEdgesLastPos = reverse_edges.get_end_of_file_pos();
    forward_edges.delete_edge_if_exist(id_to);
    reverse_edges.delete_edge_if_exist(id_from);
    forward_edges.add_edge(id_to, kReverseEdgesLastPos, edge_fields);
    reverse_edges.add_edge(id_from, kEdgesLastPos);
  };

  void delete_edge(const id_type& id_from, const id_type& id_to) {
    const auto kForwardEdgesPath = generate_forward_edge_path(id_from);
    const auto kBackwardEdgesPath = generate_backward_edge_path(id_to);
    forward_edge_adapter forward_edges(kForwardEdgesPath);
    backward_edge_adapter reverse_edges(kBackwardEdgesPath);
    forward_edges.delete_edge(id_to);
    reverse_edges.delete_edge(id_from);
  }

  void delete_all_vertex_edges(const id_type& id) {
    delete_all_forward_edges(id);
    delete_all_reverse_edges(id);
  }

  /*============================= LookUp ==============================*/
  optional_type<edge_type> get_edge(const id_type& id_from,
                                    const id_type& id_to) {
    forward_edge_adapter edges(generate_forward_edge_path(id_from));
    while (auto finded_edge =
               edges.template get_next_edge<file_offset_type, edge_type>()) {
      if (auto [id, offset, edge] = finded_edge.value(); id == id_to) {
        return edge;
      }
    }
    return std::nullopt;
  }

  ids_type get_backward_ids(const id_type& id) {
    backward_edge_adapter reverse_edges(generate_backward_edge_path(id));

    ids_type backward_ids;
    while (auto finded_edge =
               reverse_edges.template get_next_id<file_offset_type>()) {
      const auto kFromId = finded_edge.value();
      backward_ids.emplace_back(kFromId);
    }
    return backward_ids;
  }

  ids_edges_type get_backward_edges(const id_type& id) {
    backward_edge_adapter reverse_edges(generate_backward_edge_path(id));

    ids_edges_type backward_edges;
    while (auto finded_edge =
               reverse_edges.template get_next_edge<file_offset_type>()) {
      const auto [kFromId, kOffset] = finded_edge.value();
      auto edge = get_edge(kFromId, id).value();
      backward_edges.emplace_back(kFromId, std::move(edge));
    }
    return backward_edges;
  }

  ids_edges_type get_forward_edges(const id_type& id) {
    forward_edge_adapter edges(generate_forward_edge_path(id));

    ids_edges_type forward_edges;
    while (auto finded_edge =
               edges.get_next_edge<file_offset_type, edge_type>()) {
      auto [to_id, to_offset, edge] = finded_edge.value();
      forward_edges.emplace_back(to_id, std::move(edge));
    }
    return forward_edges;
  }

  ids_type get_forward_ids(const id_type& id) {
    forward_edge_adapter forward_edges(generate_forward_edge_path(id));

    ids_type forward_ids;
    while (
        auto finded_id =
            forward_edges.template get_next_id<file_offset_type, edge_type>()) {
      const auto kFromId = finded_id.value();
      forward_ids.emplace_back(kFromId);
    }
    return forward_ids;
  }

 private:
  void delete_all_forward_edges(const id_type& id) {
    const auto kForwardEdgesPath = generate_forward_edge_path(id);
    forward_edge_adapter edges(kForwardEdgesPath);
    while (auto finded_edge =
               edges.template get_next_edge<file_offset_type, edge_type>()) {
      auto [id_to, file_offset, edge] = finded_edge.value();
      backward_edge_adapter reverse_edges(generate_backward_edge_path(id_to));
      reverse_edges.delete_edge_by_offset(file_offset);
    }
    std::filesystem::remove_all(kForwardEdgesPath);
  }

  void delete_all_reverse_edges(const id_type& id) {
    const auto kReverseEdgesPath = generate_backward_edge_path(id);
    backward_edge_adapter reverse_edges(kReverseEdgesPath);
    while (auto finded_edge =
               reverse_edges.template get_next_edge<file_offset_type>()) {
      auto [id_from, file_offset] = finded_edge.value();
      forward_edge_adapter edges(generate_forward_edge_path(id_from));
      edges.delete_edge_by_offset(file_offset);
    }
    std::filesystem::remove_all(kReverseEdgesPath);
  }

  /*============================= Fields ==============================*/
  files::EdgesConfigFileAdapter config_{kEdgesConfigFileName};
};
};  // namespace storage::engine