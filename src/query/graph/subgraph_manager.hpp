#pragma once

#include <tuple>
#include <vector>

template <typename Engine>
class SubGraphManager {
 public:
  using key_type = typename Engine::key_type;
  using vertex_type = typename Engine::vertex_type;
  using edge_type = typename Engine::edge_type;

  using key_vertex_type = std::pair<key_type, vertex_type>;
  using key_edge_type = std::tuple<key_type, key_type, edge_type>;

  SubGraphManager() = default;

  void drop() noexcept {
    vertexes_.clear();
    edges_.clear();
  }

  void drop_edges() noexcept { edges_.clear(); }

  const auto& vertexes() const noexcept { return vertexes_; }

  const auto& edges() const noexcept { return edges_; }

  void add_vertex(key_type key, vertex_type vertex) {
    vertexes_.emplace_back(std::move(key), std::move(vertex));
  }

  void add_edge(key_type from, key_type to, edge_type edge) {
    edges_.emplace_back(std::move(from), std::move(to), std::move(edge));
  }

 private:
  std::vector<key_vertex_type> vertexes_;
  std::vector<key_edge_type> edges_;
};