#include "manager/storage_manager.hpp"

#include <fmt/core.h>

#include <boost/process.hpp>
#include <filesystem>
#include <fstream>

#include "manager/storage_manager_patterns.hpp"
#include "storage_manager.hpp"
#include "utils/storage_config.hpp"

namespace storage::manager {

void StorageManager::create_db() {
  const path_type kCopiedCompileScript = get_compile_script_path();
  std::filesystem::create_directory(db_path_);
  std::filesystem::copy(kGraphCompileScriptPath, kCopiedCompileScript);
}

void StorageManager::create_graph(const GraphInfo& graph_info) {
  const path_type kGraphDirectoryPath = get_graph_path(graph_info.graph_name);
  const path_type kCodePath = get_code_path(kGraphDirectoryPath);

  std::filesystem::create_directory(kGraphDirectoryPath);
  std::filesystem::create_directory(kGraphDirectoryPath / kMapsDirName);
  std::filesystem::create_directory(kGraphDirectoryPath / kKeysDirName);
  std::filesystem::create_directory(kGraphDirectoryPath / kVertexesDirName);
  std::filesystem::create_directory(kGraphDirectoryPath / kEdgesDirName);

  const text_type kCode = fmt::format(
      patterns::kCodePattern, graph_info.key_type, graph_info.vertex_names,
      graph_info.vertex_type, graph_info.edge_names, graph_info.edge_type);

  write_file(kCodePath, kCode);
  compile_graph(kGraphDirectoryPath);
}

void StorageManager::remove_db() { std::filesystem::remove_all(db_path_); }

void StorageManager::remove_graph(const text_type& graph_name) {
  std::filesystem::remove_all(get_graph_path(graph_name));
}

void StorageManager::write_file(const path_type& file_path,
                                const text_type& code) {
  std::ofstream file(file_path);
  file << code;
}

path_type StorageManager::get_compile_script_path() {
  return db_path_ / kGraphCompileScriptFileName;
}

path_type StorageManager::get_graph_path(const text_type& graph_name) {
  return db_path_ / graph_name;
}

path_type StorageManager::get_code_path(const path_type& graph_path) {
  return graph_path / kCodeFileName;
}

path_type StorageManager::get_binary_path(const path_type& graph_path) {
  return graph_path / kBinaryFileName;
}

void StorageManager::compile_graph(const path_type& graph_path) {
  static constexpr const char* kBashCommand = "bash";
  const path_type& k_graph_path = graph_path;
  const path_type kCompileScriptPath = get_compile_script_path();
  const path_type kCodePath = get_code_path(k_graph_path);
  const path_type kBinaryPath = get_binary_path(k_graph_path);
  boost::process::child cmake(boost::process::search_path(kBashCommand).c_str(),
                              kCompileScriptPath.c_str(), kCodePath.c_str(),
                              kBinaryPath.c_str());
  cmake.wait();
}

void FieldsConstructor::add_field(const text_type& name,
                                  const text_type& type) {
  const text_type kNameVariableName = name_mangling_ + name;
  add_name_impl(kNameVariableName, name);
  add_field_impl(kNameVariableName, type);
}

text_type FieldsConstructor::get_names() { return names_; }

text_type FieldsConstructor::get_type() {
  return fmt::format(patterns::kFieldsPattern, fields_);
}

void FieldsConstructor::add_name_impl(const text_type& name_variable_name,
                                      const text_type& name) {
  names_ +=
      fmt::format(patterns::kNameVariablePattern, name_variable_name, name);
}

void FieldsConstructor::add_field_impl(const text_type& name_variable_name,
                                       const text_type& type) {
  add_comma_if_it_necessary();
  fields_ += fmt::format(patterns::kTypePattern, name_variable_name, type);
}

void FieldsConstructor::add_comma_if_it_necessary() {
  if (!fields_.empty()) {
    fields_ += ",";
  }
}

text_type FieldsConstructor::do_mangling(const text_type& name) {
  return name_mangling_ + name;
}

}  // namespace storage::manager