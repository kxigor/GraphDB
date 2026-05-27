#pragma once

#include <iostream>

#include "../interprocess/interprocess.hpp"
#include "subgraph_manager.hpp"
#include "utility.hpp"

template <typename Engine>
struct Processor {
 public:
  using key_type = typename Engine::key_type;
  using vertex_type = typename Engine::vertex_type;
  using edge_type = typename Engine::edge_type;

  struct SubGraph {
    using key_vertex_type = std::pair<key_type, vertex_type>;
    using key_edge_type = std::tuple<key_type, key_type, edge_type>;

    using key_vertexes_type = std::vector<key_vertex_type>;
    using key_edges_type = std::vector<key_edge_type>;

    key_vertexes_type vertexes;
    key_edges_type edges;
  };

  Processor(Engine& engine)
      : engine_(engine), mutex_(bi::open_only, kMutexName) {}

  Processor(const Processor&) = delete;
  Processor(Processor&&) = delete;

  Processor& operator=(const Processor&) = delete;
  Processor& operator=(Processor&&) = delete;

  void launch() {
    while (process_) {
      try {
        auto query = DeserializeQuery();
        boost::apply_visitor(QueryVizitor(*this), query);
      } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
      }
    }
  }

 private:
  class QueryVizitor {
   public:
    QueryVizitor(Processor& processor) : processor_(processor) {}

    void operator()(const ast::ExitQuery&) { processor_.finish(); }
    void operator()(const ast::DeleteQuery&) { processor_.finish(); }

    void operator()(const ast::AddQuery& query) {
      if (not query.vertex.key.has_value()) {
        throw InvalidRequest("key doesn`t provided");
      }

      auto key = ConvertTo<key_type>(query.vertex.key.value());
      auto fields = ConvertProperties<vertex_type>(query.vertex.properties);
      processor_.engine_.add_vertex(key, fields);
    }

    void operator()(const ast::MatchQuery& query) {
      if (not query.match.from.key.has_value() ||
          not query.match.to.key.has_value()) {
        throw InvalidRequest("keys don`t provided");
      }

      if ((query.match.from.properties.size() != 0) ||
          (query.match.to.properties.size() != 0)) {
        throw NotImplemented("searching by parameters");
      }

      auto key_from = ConvertTo<key_type>(query.match.from.key.value());
      auto key_to = ConvertTo<key_type>(query.match.to.key.value());
      auto fields = ConvertProperties<edge_type>(query.match.edge);
      processor_.engine_.add_edge(key_from, key_to, fields);
    }

    void operator()(const ast::RemoveQuery& query) {
      for (const auto& [key, vertex] : processor_.selected_.vertexes()) {
        processor_.engine_.delete_vertex(key);
      }
      processor_.selected_.drop();
    }

    void operator()(const ast::UnmatchQuery& query) {
      for (const auto& [from, to, edge] : processor_.selected_.edges()) {
        processor_.engine_.delete_edge(from, to);
      }
      processor_.selected_.drop_edges();
    }

    void operator()(const ast::SelectQuery& query) {
      processor_.selected_.drop();
      boost::apply_visitor(SelectVizitor(processor_), query.target);
    }

    void operator()(const ast::ReturnQuery& query) {
      static auto print_param = [](const auto& param) {
        std::cout << "\t\t" << param.kName << " : " << param.value << '\n';
      };

      bi::scoped_lock<bi::named_mutex> lock(processor_.mutex_);

      std::cout << "vertexes {\n";
      for (const auto& [key, vertex] : processor_.selected_.vertexes()) {
        std::cout << '\t' << key << " {\n";
        ApplyToTuple(vertex, print_param);
        std::cout << "\t}\n";
      }
      std::cout << "}\n";

      std::cout << "edges {\n";
      for (const auto& [from, to, edge] : processor_.selected_.edges()) {
        std::cout << "\t( " << from << " -> " << to << " ) {\n";
        ApplyToTuple(edge, print_param);
        std::cout << "\t}\n";
      }
      std::cout << "}\n";
    }

    void operator()(const auto&) { throw NotImplemented("not implemented query"); }

   private:
    Processor& processor_;
  };

  class SelectVizitor {
   public:
    SelectVizitor(Processor& processor) : processor_(processor) {}

    void operator()(const ast::Vertex& vertex) {
      if (not vertex.key.has_value() || (vertex.properties.size() != 0)) {
        throw NotImplemented("searching by properties");
      }

      auto key = ConvertTo<key_type>(vertex.key.value());
      auto found = processor_.engine_.get_vertex(key);

      if (not found.has_value()) {
        return;
      }
      processor_.selected_.add_vertex(key, std::move(found.value()));
    }

    void operator()(const ast::Match& match) {
      if ((match.from.properties.size() != 0) ||
          (match.to.properties.size() != 0) || (match.edge.size() != 0)) {
        throw NotImplemented("searching by properties");
      }

      if (match.from.key.has_value() && match.to.key.has_value()) {
        return select_edge(match.from.key.value(), match.to.key.value());
      }

      if (match.from.key.has_value()) {
        return select_forward_edges(match.from.key.value());
      }

      if (match.to.key.has_value()) {
        return select_backward_edges(match.to.key.value());
      }

      throw InvalidRequest("keys don`t provided");
    }

   private:
    void select_edge(const std::string& from, const std::string& to) {
      auto key_from = ConvertTo<key_type>(from);
      auto key_to = ConvertTo<key_type>(to);

      auto found = processor_.engine_.get_edge(key_from, key_to);

      if (not found.has_value()) {
        return;
      }

      auto vertex_from = processor_.engine_.get_vertex(key_from).value();
      auto vertex_to = processor_.engine_.get_vertex(key_to).value();

      processor_.selected_.add_vertex(key_from, std::move(vertex_from));
      processor_.selected_.add_vertex(key_to, std::move(vertex_to));
      processor_.selected_.add_edge(key_from, key_to, std::move(found.value()));
    }

    void select_forward_edges(const std::string& from) {
      auto key_from = ConvertTo<key_type>(from);
      auto found = processor_.engine_.get_forward_edges(key_from);

      if (found.empty()) {
        return;
      }

      auto vertex_from = processor_.engine_.get_vertex(key_from).value();
      processor_.selected_.add_vertex(key_from, std::move(vertex_from));

      for (auto& [key_to, edge] : found) {
        auto vertex_to = processor_.engine_.get_vertex(key_to).value();
        processor_.selected_.add_vertex(key_to, std::move(vertex_to));

        processor_.selected_.add_edge(key_from, key_to, std::move(edge));
      }
    }

    void select_backward_edges(const std::string& to) {
      auto key_to = ConvertTo<key_type>(to);
      auto found = processor_.engine_.get_backward_edges(key_to);

      if (found.empty()) {
        return;
      }

      auto vertex_to = processor_.engine_.get_vertex(key_to).value();
      processor_.selected_.add_vertex(key_to, std::move(vertex_to));

      for (auto& [key_from, edge] : found) {
        auto vertex_from = processor_.engine_.get_vertex(key_from).value();
        processor_.selected_.add_vertex(key_from, std::move(vertex_from));

        processor_.selected_.add_edge(key_from, key_to, std::move(edge));
      }
    }

    Processor& processor_;
  };

  void finish() noexcept { process_ = false; }

  Engine& engine_;
  SubGraphManager<Engine> selected_;

  bool process_{true};
  bi::named_mutex mutex_;
};