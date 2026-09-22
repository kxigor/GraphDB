
#include "query/handler/query_handler.hpp"

using handler = void (*)();

static std::unordered_map<std::string, storage::manager::StorageManager>
    databases;

static const std::unordered_map<std::string, handler> handlers = {
    {"create",
     +[]() {
       std::string database;
       std::cin >> database;

       if (databases.contains(database)) {
         std::cerr << "already exists\n";
         return;
       }

       databases.emplace(database, storage::manager::StorageManager(database));
       databases.at(database).create_db();
     }},
    {"destroy",
     +[]() {
       std::string database;
       std::cin >> database;

       if (not databases.contains(database)) {
         std::cerr << "no database named `" << database << "`\n";
         return;
       }

       databases.at(database).remove_db();
       databases.erase(database);
     }},
    {"launch",
     +[]() {
       std::string database;
       std::cin >> database;

       if (not databases.contains(database)) {
         std::cerr << "no database named `" << database << "`\n";
         return;
       }

       QueryHandler::Launch(database, databases.at(database));
       QueryHandler::HandleQueries();
       QueryHandler::Finish();
     }},
    {"exit", +[]() { exit(EXIT_SUCCESS); }}};

static void HandleQuery(const std::string &query);

int main() try {
  while (std::cin.good()) {
    std::string query;
    std::cin >> query;
    HandleQuery(query);
  }
} catch (...) {
  QueryHandler::Finish();

  std::cerr << "internal error\n";
  exit(EXIT_FAILURE);
}

void HandleQuery(const std::string &query) {
  if (handlers.contains(query)) {
    return handlers.at(query)();
  }
  std::cerr << "invalid query\n";
}