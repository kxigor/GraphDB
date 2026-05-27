#pragma once

#include <boost/process.hpp>
#include <filesystem>
#include <iostream>
#include <unordered_map>

#include "../parser/query_parser.hpp"
#include "../../storage/manager/storage_manager.hpp"
#include "../interprocess/interprocess.hpp"

namespace fs = std::filesystem;
namespace bp = boost::process;
namespace ba = boost::archive;

class QueryHandler {
 public:
  static void Launch(const fs::path &database, storage::manager::StorageManager &manager);
  static void HandleQueries();
  static void Finish();

  ~QueryHandler() {
    bi::named_mutex::remove(kMutexName);
  }

 private:
  QueryHandler(): mutex_(bi::create_only, kMutexName) {}

  QueryHandler(const QueryHandler &) = delete;
  QueryHandler(QueryHandler &&) = delete;

  QueryHandler &operator=(const QueryHandler &) = delete;
  QueryHandler &operator=(QueryHandler &&) = delete;

  static QueryHandler &GetInstance();

  void launch_graph(const fs::path &);
  bool handle_query();

  class Vizitor {
   public:
    Vizitor() = default;

    bool operator()(ast::ExitQuery &&);

    bool operator()(ast::NewQuery &&);
    bool operator()(ast::DeleteQuery &&);

    bool operator()(auto &&);

    bool send_query(auto &&);
  };

  struct GraphProcess {
    GraphProcess() = default;

    bp::child proc;
    bp::opstream out;
  };

  fs::path database_path;
  std::unordered_map<std::string, GraphProcess> database_;
  storage::manager::StorageManager *manager_;

  bi::named_mutex mutex_;
};