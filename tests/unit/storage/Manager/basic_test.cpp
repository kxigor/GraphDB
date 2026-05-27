#include <gtest/gtest.h>

#include <filesystem>

#include "manager/storage_manager.hpp"
#include "utils/storage_config.hpp"

const storage::path_type kBDName = "AMOGUS_BD_SHECHKA";
const storage::path_type kGraphName = "BIBOBA_GRAPH";

TEST(Manager, CREATE_DB) {
  storage::manager::StorageManager manager(kBDName);

  manager.create_db();

  ASSERT_TRUE(std::filesystem::exists(kBDName));
  ASSERT_TRUE(std::filesystem::is_directory(kBDName));
}

TEST(Manager, CREATE_GRAPG) {
  storage::manager::StorageManager manager(kBDName);

  storage::manager::FieldsConstructor vertex_fields("v_");
  storage::manager::FieldsConstructor edge_fields("e_");

  vertex_fields.add_field("NAME", "std::string");
  edge_fields.add_field("ADDR", "std::string");

  storage::manager::GraphInfo graph_info{
      .graph_name = kGraphName,
      .key_type = "int",
      .vertex_names = vertex_fields.get_names(),
      .vertex_type = vertex_fields.get_type(),
      .edge_names = edge_fields.get_names(),
      .edge_type = edge_fields.get_type(),
  };

  manager.create_graph(graph_info);

  ASSERT_TRUE(std::filesystem::exists(kBDName / kGraphName));
  ASSERT_TRUE(std::filesystem::is_directory(kBDName / kGraphName));

  ASSERT_TRUE(
      std::filesystem::exists(kBDName / kGraphName / storage::kCodeFileName));
  ASSERT_TRUE(std::filesystem::is_regular_file(kBDName / kGraphName /
                                               storage::kCodeFileName));

  ASSERT_TRUE(
      std::filesystem::exists(kBDName / kGraphName / storage::kBinaryFileName));
  ASSERT_TRUE(std::filesystem::is_regular_file(kBDName / kGraphName /
                                               storage::kBinaryFileName));

  ASSERT_TRUE(
      std::filesystem::exists(kBDName / kGraphName / storage::kMapsDirName));
  ASSERT_TRUE(std::filesystem::is_directory(kBDName / kGraphName /
                                            storage::kMapsDirName));

  ASSERT_TRUE(
      std::filesystem::exists(kBDName / kGraphName / storage::kKeysDirName));
  ASSERT_TRUE(std::filesystem::is_directory(kBDName / kGraphName /
                                            storage::kKeysDirName));

  ASSERT_TRUE(std::filesystem::exists(kBDName / kGraphName /
                                      storage::kVertexesDirName));
  ASSERT_TRUE(std::filesystem::is_directory(kBDName / kGraphName /
                                            storage::kVertexesDirName));

  ASSERT_TRUE(
      std::filesystem::exists(kBDName / kGraphName / storage::kEdgesDirName));
  ASSERT_TRUE(std::filesystem::is_directory(kBDName / kGraphName /
                                            storage::kEdgesDirName));
}

TEST(Manager, REMOVE_GRAPH) {
  storage::manager::StorageManager manager(kBDName);

  manager.remove_graph(kGraphName);

  ASSERT_FALSE(std::filesystem::exists(kBDName / kGraphName));
}

TEST(Manager, REMOVE_DB) {
  storage::manager::StorageManager manager(kBDName);

  manager.remove_db();

  ASSERT_FALSE(std::filesystem::exists(kBDName));
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
