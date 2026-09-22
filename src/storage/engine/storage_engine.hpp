#include <algorithm>
#include <optional>

#include "engine/storage_engine_managers.hpp"
#include "utils/storage_config.hpp"
#include "utils/storage_errors.hpp"
#include "utils/storage_foraward_like.hpp"
#include "utils/storage_getter.hpp"
#include "utils/storage_optional_handle.hpp"

namespace storage::engine {

template <typename Key, typename VertexFields, typename EdgeFields,
          typename Hash>
class StorageEngine {
 public:
  /*======================== Usings/Constants =========================*/

  // clang-format off
  using key_type    = Key;
  using vertex_type = VertexFields;
  using edge_type   = EdgeFields;
  using hash_type   = Hash;

  using key_vertex_type               = VertexUsings<vertex_type>::template typed_vertex_type<key_type>;
  using keys_vertexes_neighbours_type = VertexUsings<vertex_type>::template typed_vertexes_type<key_type>;
  using key_edge_type                 = EdgeUsings<edge_type>::template typed_edge_type<key_type>;
  using keys_edges_neighbours_type    = EdgeUsings<edge_type>::template typed_edges_type<key_type>;
  using id_vertex_type                = VertexUsings<vertex_type>::id_vertex_type;
  using ids_vertexes_type             = VertexUsings<vertex_type>::ids_vertexes_type;
  using id_edge_type                  = EdgeUsings<edge_type>::id_edge_type;
  using ids_edges_type                = EdgeUsings<edge_type>::ids_edges_type;

  using vertex_getter_type  = details::Getter<vertex_type>;
  using vertex_edge_type    = details::Getter<edge_type>;

  using key_manager_type    = KeyManager<key_type, hash_type>;
  using vertex_manager_type = VertexManager<vertex_type>;
  using edges_manager_type  = EdgeManager<edge_type>;
  // clang-format on

  template <typename Visitor, typename AdjacentIdsGetter>
  class BFSHelper {
   public:
    BFSHelper(Visitor visitor, AdjacentIdsGetter adjacent_ids_getter)
        : visitor_(std::move(visitor)),
          adjacent_ids_getter_(std::move(adjacent_ids_getter)) {}

    void visit_with_depth(const id_type& id, const depth_type& depth) {
      append_unvisited_to_queue(id);
      for (size_type cur_depth = 0;
           cur_depth < depth && !vertexes_to_visit_.empty(); ++cur_depth) {
        process_level();
      }
    }

   private:
    void process_level() {
      const size_type kLevelSize = vertexes_to_visit_.size();
      for (size_type cur_ctr = 0; cur_ctr < kLevelSize; ++cur_ctr) {
        const id_type kCurId = vertexes_to_visit_.front();
        vertexes_to_visit_.pop();
        visited_vertexes_.insert(kCurId);
        visitor_(kCurId);
        append_unvisited_to_queue(kCurId);
      }
    }

    void append_unvisited_to_queue(const id_type& id) {
      visited_vertexes_.insert(id);
      for (auto adding_id : adjacent_ids_getter_(id)) {
        if (not visited_vertexes_.contains(adding_id)) {
          vertexes_to_visit_.push(adding_id);
        }
      }
    }

    unordered_set_type<id_type> visited_vertexes_;
    queque_type<id_type> vertexes_to_visit_;
    Visitor visitor_{};
    AdjacentIdsGetter adjacent_ids_getter_{};
  };

  /*==================== Constructors/Assignments =====================*/
  StorageEngine() = default;
  ~StorageEngine() = default;
  StorageEngine(const StorageEngine& /*unused*/) = delete;
  StorageEngine(StorageEngine&& /*unused*/) = delete;
  StorageEngine& operator=(const StorageEngine& /*unused*/) = delete;
  StorageEngine& operator=(StorageEngine&& /*unused*/) = delete;

  /*============================ Add/Edit =============================*/
  void add_vertex(const key_type& key, const vertex_type& vertex_fields) {
    const auto kId = key_manager_.find_id_or_insert_key(key);
    vertex_manager_.add_vertex(kId, vertex_fields);
  }

  void add_edge(const key_type& from, const key_type& to,
                const edge_type& edge_fields) {
    const id_type kFromId = find_id_or_throw(from);
    const id_type kToId = find_id_or_throw(to);
    edge_manager_.add_edge(kFromId, kToId, edge_fields);
  }

  template <typename Apply>
  requires std::is_invocable_v<Apply, vertex_type&>
  void apply_to_neighbours(const key_type& key, const depth_type& depth,
                           Apply&& apply);

  template <typename Apply, typename Pred>
  requires std::is_invocable_v<Apply, vertex_type&> &&
           std::is_invocable_r_v<bool, Pred, const vertex_type&>
  void apply_to_neighbours_if(const key_type& key, const depth_type& depth,
                              Apply&& apply, Pred&& pred);

  template <typename Apply, typename Pred>
  requires std::is_invocable_v<Apply, vertex_type&>
  void apply_to_if(Apply&& apply, Pred&& pred);

  template <typename Apply>
  void apply_to_all(Apply&& apply);

  /*============================= Delete ==============================*/
  void delete_vertex(const key_type& key) {
    const auto kId = find_id_or_throw(key);
    key_manager_.erase_key(key);
    edge_manager_.delete_all_vertex_edges(kId);
  }

  void delete_edge(const key_type& from, const key_type& to) {
    const id_type kFromId = find_id_or_throw(from);
    const id_type kToId = find_id_or_throw(to);
    edge_manager_.delete_edge(kFromId, kToId);
  }

  /*============================= LookUp ==============================*/
  optional_type<vertex_type> get_vertex(const key_type& key) {
    const auto kFindIdResult = key_manager_.find_id_by_key(key);
    if (not kFindIdResult.has_value()) {
      return std::nullopt;
    }
    return vertex_manager_.get_vertex(kFindIdResult.value());
  }

  optional_type<edge_type> get_edge(const key_type& from, const key_type& to) {
    const auto kFromId = find_id_or_throw(from);
    const auto kToId = find_id_or_throw(to);
    return edge_manager_.get_edge(kFromId, kToId);
  }

  keys_edges_neighbours_type get_forward_edges(const key_type& key) {
    const auto kId = find_id_or_throw(key);
    auto ids_edges = edge_manager_.get_forward_edges(kId);
    auto keys_edges = ids_container_to_keys_container(std::move(ids_edges));
    return keys_edges;
  }

  keys_edges_neighbours_type get_backward_edges(const key_type& key) {
    const auto kId = find_id_or_throw(key);
    auto ids_edges = edge_manager_.get_backward_edges(kId);
    auto keys_edges = ids_container_to_keys_container(std::move(ids_edges));
    return keys_edges;
  }

  keys_vertexes_neighbours_type get_neighbours(const key_type& key,
                                               const depth_type& depth) {
    const auto kId = find_id_or_throw(key);
    auto ids_vertexes = get_neighbours(kId, depth);
    return ids_container_to_keys_container(std::move(ids_vertexes));
  }

  template <typename Pred>
  requires std::is_invocable_r_v<bool, Pred, const vertex_type&>
  keys_vertexes_neighbours_type get_neighbours_if(const key_type& key,
                                                  const depth_type& depth,
                                                  Pred&& pred);

  template <typename Pred>
  requires std::is_invocable_r_v<bool, Pred, const vertex_type&>
  keys_vertexes_neighbours_type get_if(Pred&& pred);

  keys_vertexes_neighbours_type get_all();

 private:
  id_type find_id_or_throw(const key_type& key) {
    return details::unwrap_optional_or_throw<errors::BadKey>(
        key_manager_.find_id_by_key(key), key);
  }

  template <typename Container>
  auto ids_container_to_keys_container(Container&& container) {
    using value_type =
        pure_type<decltype(std::get<1>(std::declval<Container>()[0]))>;
    using result_type = container_type<typed_type<key_type, value_type>>;
    result_type result;
    result.reserve(container.size());
    for (auto& [id, fields] : container) {
      result.emplace_back(key_manager_.find_key_by_id(id),
                          storage::details::forward_like<Container>(fields));
    }
    return result;
  }

  ids_vertexes_type get_neighbours(const id_type& id, const depth_type& depth) {
    ids_vertexes_type neighbours;

    auto adjacent_ids_getter = [&](const id_type& source_vertex_id) {
      return edge_manager_.get_forward_ids(source_vertex_id);
    };

    auto visitor = [&](const id_type& id) {
      auto vertex = vertex_manager_.get_vertex(id);
      neighbours.emplace_back(id, std::move(vertex));
    };

    BFSHelper bfs(std::move(visitor), std::move(adjacent_ids_getter));
    bfs.visit_with_depth(id, depth);

    return neighbours;
  }

  /*============================= Fields ==============================*/
  key_manager_type key_manager_{};
  vertex_manager_type vertex_manager_{};
  edges_manager_type edge_manager_{};
};

}  // namespace storage::engine
