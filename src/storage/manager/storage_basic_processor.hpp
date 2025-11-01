#pragma once

// NOLINTBEGIN
#include <iostream>

/*абсолютно тупой процессор, будет дорабатываться*/
/*TODO нормальный код, это затычка*/

template <typename Engine>
void process(Engine& storage_engine) {
  enum {
    INSERT_VERTEX = 1,
    INSERT_EDGE = 2,
    FIND_VERTEX = 3,
    FIND_FORWARD_EDGES = 4,
    FIND_BACKWARD_EDGES = 5,
    DELETE_VERTEX = 6,
    DELETE_EDGE = 7,
    EXIT = 8,
  };

  using vertex_getter = typename Engine::vertex_getter_type;
  using edge_getter = typename Engine::vertex_edge_type;

  bool loop_flag = true;
  std::cout << "commands:\n"
            << "1. insert vertex;\n"
            << "2. insert edge;\n"
            << "3. find vertex;\n"
            << "4. find forward edges;\n"
            << "5. find backward edges;\n"
            << "6. delete vertex;\n"
            << "7. delete edge;\n"
            << "8. exit;\n";

  while (loop_flag) {
    int command{};
    std::cout << "command:";
    std::cin >> command;

    switch (command) {
      case INSERT_VERTEX: {
        int key;
        std::string fields;
        std::cout << "key: ";
        std::cin >> key;
        std::cout << "fields: ";
        std::cin >> fields;
        storage_engine.add_vertex(key, fields);
        break;
      }
      case INSERT_EDGE: {
        int key_from, key_to;
        std::string fields;
        std::cout << "key_from: ";
        std::cin >> key_from;
        std::cout << "key_to: ";
        std::cin >> key_to;
        std::cout << "fields: ";
        std::cin >> fields;
        storage_engine.add_edge(key_from, key_to, fields);
        break;
      }
      case FIND_VERTEX: {
        int key;
        std::cout << "key: ";
        std::cin >> key;
        auto find_result = storage_engine.get_vertex(key);
        if (find_result.has_value()) {
          auto feilds = find_result.value();
          std::cout << "[" << vertex_getter::get(feilds, 0) << "]\n";
        } else {
          std::cout << "can't find ;(\n";
        }
        break;
      }
      case FIND_FORWARD_EDGES: {
        int key;
        std::cout << "key: ";
        std::cin >> key;
        auto edges = storage_engine.get_forward_edges(key);
        for (auto& [key, fields] : edges) {
          std::cout << '[' << key << ", " << edge_getter::get(fields, 0)
                    << "]\n";
        }
        break;
      }
      case FIND_BACKWARD_EDGES: {
        int key;
        std::cout << "key: ";
        std::cin >> key;
        auto edges = storage_engine.get_backward_edges(key);
        for (auto& [key, fields] : edges) {
          std::cout << '[' << key << ", " << edge_getter::get(fields, 0)
                    << "]\n";
        }
        break;
      }
      case DELETE_VERTEX: {
        int key;
        std::cout << "key: ";
        std::cin >> key;
        storage_engine.delete_vertex(key);
        break;
      }
      case DELETE_EDGE: {
        int key_from, key_to;
        std::string fields;
        std::cout << "key_from: ";
        std::cin >> key_from;
        std::cout << "key_to: ";
        std::cin >> key_to;
        storage_engine.delete_edge(key_from, key_to);
        break;
      }
      case EXIT: {
        loop_flag = false;
        break;
      }
      default: {
        std::cout << "Unknows command\n";
      }
    }
  }
}

// NOLINTEND