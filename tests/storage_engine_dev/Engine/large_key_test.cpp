#include <fmt/core.h>
#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <iostream>

#include "engine/storage_engine.hpp"
#include "test_helper.hpp"
#include "utils/storage_config.hpp"

using key_type = std::string;
using value_type = std::string;
using hash_func = std::hash<key_type>;

extern constexpr const char v_name[] = "name";
using vertex_field_type =
    storage::tuple_type<storage::NamedType<v_name, value_type>>;
extern constexpr const char e_status[] = "status";
using vertex_edge_type =
    storage::tuple_type<storage::NamedType<e_status, value_type>>;

using storage_engine_type =
    storage::engine::StorageEngine<key_type, vertex_field_type,
                                   vertex_edge_type, hash_func>;

using vertex_getter = storage_engine_type::vertex_getter_type;
using edge_getter = storage_engine_type::vertex_edge_type;
using size_type = storage::size_type;
using depth_type = storage::depth_type;

const value_type kBaseVertexValue = "bibobuba";

const std::size_t kLargeSize = 1'000'000;

key_type get_large_key(char base);

key_type get_large_key(char base) {
  std::string big_string(kLargeSize, base);
  return big_string;
}

TEST_F(DatabaseTest, BasicVertexInsert) {
  storage_engine_type engine;
  const auto key = get_large_key('0');
  engine.add_vertex(key, kBaseVertexValue);
  auto finded_vertex = engine.get_vertex(key);
  ASSERT_TRUE(finded_vertex.has_value());
  ASSERT_EQ(vertex_getter::get(finded_vertex.value(), 0), kBaseVertexValue);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
