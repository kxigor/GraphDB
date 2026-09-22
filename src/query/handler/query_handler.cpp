#include "query_handler.hpp"

static std::string ReadQuery() {
  std::string query_str;
  std::getline(std::cin, query_str, parser::separator);

  return query_str;
}

static std::pair<bool, ast::Query> ParseQuery(const std::string &query_str) {
  ast::Query query;

  parser::iterator_type begin = query_str.begin();
  const parser::iterator_type end = query_str.end();

  bool parsed =
      phrase_parse(begin, end, parser::parser(), x3::ascii::space, query);
  parsed = parsed and (begin == end);

  return {parsed, std::move(query)};
}

static std::pair<bool, ast::Query> GetQuery() {
  std::string query_str = ReadQuery();
  if (std::cin.eof()) {
    return {true, ast::ExitQuery{}};
  }
  return ParseQuery(query_str);
}

static void SerializeQuery(auto &&query, bp::opstream &pipe) {
  ba::binary_oarchive ar(pipe);
  ar << ast::Query(std::move(query));
}

static auto GraphInfo(const ast::NewQuery &query) {
  static const storage::text_type kVertexMangling = "v_";
  static const storage::text_type kEdgeMangling = "e_";

  storage::manager::FieldsConstructor vertex_fields(kVertexMangling);
  storage::manager::FieldsConstructor edge_fields(kEdgeMangling);

  for (const auto &vertex : query.graph.vertex_fields) {
    vertex_fields.add_field(vertex.second, vertex.first);
  }

  for (const auto &edge : query.graph.edge_fields) {
    edge_fields.add_field(edge.second, edge.first);
  }

  return storage::manager::GraphInfo{.graph_name = query.graph.name,
                                     .key_type = query.graph.key_type,
                                     .vertex_names = vertex_fields.get_names(),
                                     .vertex_type = vertex_fields.get_type(),
                                     .edge_names = edge_fields.get_names(),
                                     .edge_type = edge_fields.get_type()};
}

QueryHandler &QueryHandler::GetInstance() {
  static QueryHandler handler;
  return handler;
}

void QueryHandler::Launch(const fs::path &database,
                          storage::manager::StorageManager &manager) {
  static const fs::path current = "./";

  auto &handler = GetInstance();
  handler.database_path = current / database;
  handler.manager_ = std::addressof(manager);

  for (const auto &entry : fs::directory_iterator(handler.database_path)) {
    if (entry.is_directory()) {
      handler.launch_graph(entry.path());
    }
  }
}

void QueryHandler::HandleQueries() {
  auto &handler = GetInstance();

  bool handle_next = true;
  while (handle_next) {
    handle_next = handler.handle_query();
  }
}

void QueryHandler::Finish() {
  auto &handler = GetInstance();

  for (auto &[graph, child] : handler.database_) {
    auto &[process, out] = child;
    SerializeQuery(ast::ExitQuery{}, out);

    out.close();
    process.wait();
  }

  handler.database_.clear();
  handler.manager_ = nullptr;
}

void QueryHandler::launch_graph(const fs::path &graph_dir) {
  static const std::string executable(storage::kBinaryFileName);

  auto graph = graph_dir.filename().string();
  auto &[process, out] = database_[graph];
  process = bp::child(executable, bp::start_dir = graph_dir.string(),
                      bp::std_out > stdout, bp::std_in < out);
}

bool QueryHandler::handle_query() {
  auto [parsed, query] = GetQuery();
  if (not parsed) {
    std::cerr << "invalid query!\n";
    return true;
  }
  return boost::apply_visitor(Vizitor(), std::move(query));
}

bool QueryHandler::Vizitor::operator()(ast::ExitQuery &&) { return false; }

bool QueryHandler::Vizitor::operator()(ast::NewQuery &&query) {
  auto &handler = GetInstance();

  if (handler.database_.contains(query.graph.name)) {
    std::cerr << "already exists\n";
    return true;
  }

  handler.manager_->create_graph(GraphInfo(query));
  handler.launch_graph(handler.database_path / query.graph.name);

  return true;
}

bool QueryHandler::Vizitor::operator()(ast::DeleteQuery &&query) {
  std::string graph = query.graph;
  bool sent = send_query(std::move(query));

  if (sent) {
    auto &handler = GetInstance();
    auto &[process, out] = GetInstance().database_[graph];

    out.close();
    process.wait();

    handler.database_.erase(graph);
    handler.manager_->remove_graph(graph);
  }

  return true;
}

bool QueryHandler::Vizitor::operator()(auto &&query) {
  send_query(std::move(query));
  return true;
}

bool QueryHandler::Vizitor::send_query(auto &&query) {
  auto &handler = GetInstance();
  std::string &graph = query.graph;

  if (not handler.database_.contains(graph)) {
    std::cerr << "no graph named `" << graph << "`\n";
    return false;
  }

  auto &[process, out] = handler.database_[graph];
  SerializeQuery(std::move(query), out);

  return true;
}