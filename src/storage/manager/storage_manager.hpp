#pragma once

#include <utility>

#include "utils/storage_config.hpp"

namespace storage::manager {

struct GraphInfo {
  text_type graph_name;

  text_type key_type;

  text_type vertex_names;
  text_type vertex_type;

  text_type edge_names;
  text_type edge_type;
};

class FieldsConstructor {
 public:
  FieldsConstructor(text_type name_mangling)
      : name_mangling_(std::move(name_mangling)) {}

  void add_field(const text_type& name, const text_type& type);
  text_type get_names();
  text_type get_type();

 private:
  void add_name_impl(const text_type& name_variable_name,
                     const text_type& name);

  void add_field_impl(const text_type& name_variable_name,
                      const text_type& type);

  void add_comma_if_it_necessary();

  text_type do_mangling(const text_type& name);

  text_type names_;
  text_type fields_;
  text_type name_mangling_;
};

class StorageManager {
 public:
  StorageManager(text_type db_name) : db_path_(std::move(db_name)) {}

  void create_db();
  void create_graph(const GraphInfo& graph_info);

  void remove_db();
  void remove_graph(const text_type& graph_name);

 private:
  void compile_graph(const path_type& graph_path);
  static void write_file(const path_type& file_path, const text_type& code);

  path_type get_compile_script_path();
  path_type get_graph_path(const text_type& graph_name);

  static path_type get_code_path(const path_type& graph_path);
  static path_type get_binary_path(const path_type& graph_path);

  std::filesystem::path db_path_;
};

}  // namespace storage::manager