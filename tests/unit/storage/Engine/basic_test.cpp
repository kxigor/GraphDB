#include <fmt/core.h>
#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <iostream>

#include "engine/storage_engine.hpp"
#include "test_helper.hpp"
#include "utils/storage_config.hpp"

using key_type = int;
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

const key_type kBaseVertexKey = 1666666666;
const key_type kBaseVertexSecondKey = 1999999999;
const value_type kBaseVertexValue = "bibobuba";
const value_type kBaseVertexSecondValue = "amogus";
const value_type kBaseEdgeValue = "druzia";

const size_type kSeveralNumber = 10;
const size_type kCapacityTimes = 5;
const size_type kMulpiplyNumber = storage::kDefaultMapCapacity * kCapacityTimes;

TEST_F(DatabaseTest, BasicVertexInsert) {
  storage_engine_type engine;
  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);
  auto finded_vertex = engine.get_vertex(kBaseVertexKey);
  ASSERT_TRUE(finded_vertex.has_value());
  ASSERT_EQ(vertex_getter::get(finded_vertex.value(), 0), kBaseVertexValue);
}

TEST_F(DatabaseTest, FindVertexAfterClose) {
  {
    storage_engine_type engine;
    engine.add_vertex(kBaseVertexKey, kBaseVertexValue);
  }
  {
    storage_engine_type engine;
    auto finded_vertex = engine.get_vertex(kBaseVertexKey);
    ASSERT_TRUE(finded_vertex.has_value());
    ASSERT_EQ(vertex_getter::get(finded_vertex.value(), 0), kBaseVertexValue);
  }
}

TEST_F(DatabaseTest, InsertVertexChangesValue) {
  storage_engine_type engine;
  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);
  engine.add_vertex(kBaseVertexKey, kBaseVertexSecondValue);
  auto finded_vertex = engine.get_vertex(kBaseVertexKey);
  ASSERT_TRUE(finded_vertex.has_value());
  ASSERT_EQ(vertex_getter::get(finded_vertex.value(), 0),
            kBaseVertexSecondValue);
}

TEST_F(DatabaseTest, MultipleVertexInserts) {
  storage_engine_type engine;

  for (size_type i = 0; i < kMulpiplyNumber; ++i) {
    engine.add_vertex(static_cast<int>(i), fmt::format("{}", i));
  }

  for (size_type i = 0; i < kMulpiplyNumber; ++i) {
    auto finded_vertex = engine.get_vertex(static_cast<int>(i));
    ASSERT_TRUE(finded_vertex.has_value());
    ASSERT_EQ(vertex_getter::get(finded_vertex.value(), 0),
              fmt::format("{}", i));
  }
}

TEST_F(DatabaseTest, BasicEdgeInsert) {
  storage_engine_type engine;
  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);
  engine.add_vertex(kBaseVertexSecondKey, kBaseVertexSecondValue);
  engine.add_edge(kBaseVertexKey, kBaseVertexSecondKey, kBaseEdgeValue);
  auto finded_edge = engine.get_edge(kBaseVertexKey, kBaseVertexSecondKey);
  ASSERT_TRUE(finded_edge.has_value());
  ASSERT_EQ(edge_getter::get(finded_edge.value(), 0), kBaseEdgeValue);
}

TEST_F(DatabaseTest, SeveralEdgesFromOneVertex) {
  storage_engine_type engine;
  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);

  for (size_type i = 0; i < kSeveralNumber; ++i) {
    engine.add_vertex(static_cast<int>(i), fmt::format("{}", i));
    engine.add_edge(kBaseVertexKey, static_cast<int>(i),
                    fmt::format("e{}e", i));
  }

  auto finded_edges = engine.get_forward_edges(kBaseVertexKey);
  ASSERT_EQ(finded_edges.size(), kSeveralNumber);

  size_type ctr = 0;
  for (auto& [key, edge] : finded_edges) {
    ASSERT_EQ(key, static_cast<int>(ctr));
    ASSERT_EQ(edge_getter::get(edge, 0), fmt::format("e{}e", ctr));
    ++ctr;
  }
}

TEST_F(DatabaseTest, MultiplyEdgesFromOneVertex) {
  storage_engine_type engine;
  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);

  for (size_type i = 0; i < kMulpiplyNumber; ++i) {
    engine.add_vertex(static_cast<int>(i), fmt::format("{}", i));
    engine.add_edge(kBaseVertexKey, static_cast<int>(i),
                    fmt::format("e{}e", i));
  }

  auto finded_edges = engine.get_forward_edges(kBaseVertexKey);
  ASSERT_EQ(finded_edges.size(), kMulpiplyNumber);

  size_type ctr = 0;
  for (auto& [key, edge] : finded_edges) {
    ASSERT_EQ(key, static_cast<int>(ctr));
    ASSERT_EQ(edge_getter::get(edge, 0), fmt::format("e{}e", ctr));
    ++ctr;
  }
}

TEST_F(DatabaseTest, SeveralEdgesIntoOneVertex) {
  storage_engine_type engine;
  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);

  for (size_type i = 0; i < kSeveralNumber; ++i) {
    engine.add_vertex(static_cast<int>(i), fmt::format("{}", i));
    engine.add_edge(static_cast<int>(i), kBaseVertexKey,
                    fmt::format("e{}e", i));
  }

  auto finded_edges = engine.get_backward_edges(kBaseVertexKey);
  ASSERT_EQ(finded_edges.size(), kSeveralNumber);

  size_type ctr = 0;
  for (auto& [key, edge] : finded_edges) {
    ASSERT_EQ(key, static_cast<int>(ctr));
    ASSERT_EQ(edge_getter::get(edge, 0), fmt::format("e{}e", ctr));
    ++ctr;
  }
}

TEST_F(DatabaseTest, MultiplyEdgesIntoOneVertex) {
  storage_engine_type engine;
  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);

  for (size_type i = 0; i < kMulpiplyNumber; ++i) {
    engine.add_vertex(static_cast<int>(i), fmt::format("{}", i));
    engine.add_edge(static_cast<int>(i), kBaseVertexKey,
                    fmt::format("e{}e", i));
  }

  auto finded_edges = engine.get_backward_edges(kBaseVertexKey);
  ASSERT_EQ(finded_edges.size(), kMulpiplyNumber);

  size_type ctr = 0;
  for (auto& [key, edge] : finded_edges) {
    ASSERT_EQ(key, static_cast<int>(ctr));
    ASSERT_EQ(edge_getter::get(edge, 0), fmt::format("e{}e", ctr));
    ++ctr;
  }
}

TEST_F(DatabaseTest, SimpleDeleteEdge) {
  storage_engine_type engine;

  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);
  engine.add_vertex(kBaseVertexSecondKey, kBaseVertexSecondValue);

  engine.add_edge(kBaseVertexKey, kBaseVertexSecondKey, kBaseEdgeValue);
  engine.delete_edge(kBaseVertexKey, kBaseVertexSecondKey);

  auto first_backward = engine.get_backward_edges(kBaseVertexKey);
  auto first_forward = engine.get_forward_edges(kBaseVertexKey);
  ASSERT_TRUE(first_backward.empty());
  ASSERT_TRUE(first_forward.empty());

  auto second_backward = engine.get_backward_edges(kBaseVertexSecondKey);
  auto second_forward = engine.get_forward_edges(kBaseVertexSecondKey);
  ASSERT_TRUE(second_backward.empty());
  ASSERT_TRUE(second_forward.empty());
}

TEST_F(DatabaseTest, InsertEdgeAfterDeleteEdge) {
  const value_type kNewEdgeValue = "agogog";

  storage_engine_type engine;

  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);
  engine.add_vertex(kBaseVertexSecondKey, kBaseVertexSecondValue);

  engine.add_edge(kBaseVertexKey, kBaseVertexSecondKey, kBaseEdgeValue);
  engine.delete_edge(kBaseVertexKey, kBaseVertexSecondKey);
  engine.add_edge(kBaseVertexKey, kBaseVertexSecondKey, kNewEdgeValue);

  auto first_backward = engine.get_backward_edges(kBaseVertexKey);
  auto first_forward = engine.get_forward_edges(kBaseVertexKey);
  ASSERT_TRUE(first_backward.empty());
  ASSERT_EQ(first_forward.size(), 1);

  ASSERT_EQ(edge_getter::get(first_forward[0].second, 0), kNewEdgeValue);

  auto second_backward = engine.get_backward_edges(kBaseVertexSecondKey);
  auto second_forward = engine.get_forward_edges(kBaseVertexSecondKey);
  ASSERT_EQ(second_backward.size(), 1);
  ASSERT_TRUE(second_forward.empty());
}

TEST_F(DatabaseTest, SeveralDeleteEdge) {
  storage_engine_type engine;

  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);

  for (size_type i = 0; i < kSeveralNumber; ++i) {
    engine.add_vertex(static_cast<int>(i), fmt::format("{}", i));
    engine.add_edge(kBaseVertexKey, static_cast<int>(i),
                    fmt::format("e{}e", i));
  }

  for (size_type i = 0; i < kSeveralNumber; ++i) {
    engine.delete_edge(kBaseVertexKey, static_cast<int>(i));
  }

  auto first_backward = engine.get_backward_edges(kBaseVertexKey);
  auto first_forward = engine.get_forward_edges(kBaseVertexKey);
  ASSERT_TRUE(first_backward.empty());
  ASSERT_TRUE(first_forward.empty());

  for (size_type i = 0; i < kSeveralNumber; ++i) {
    auto backward = engine.get_backward_edges(static_cast<int>(i));
    auto forward = engine.get_forward_edges(static_cast<int>(i));
    ASSERT_TRUE(backward.empty());
    ASSERT_TRUE(forward.empty());
  }
}

TEST_F(DatabaseTest, SimpleDeleteVertexForward) {
  storage_engine_type engine;

  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);
  engine.add_vertex(kBaseVertexSecondKey, kBaseVertexSecondValue);
  engine.add_edge(kBaseVertexKey, kBaseVertexSecondKey, kBaseEdgeValue);
  engine.delete_vertex(kBaseVertexKey);

  auto backward = engine.get_backward_edges(kBaseVertexSecondKey);
  auto forward = engine.get_forward_edges(kBaseVertexSecondKey);
  ASSERT_FALSE(engine.get_vertex(kBaseVertexKey).has_value());
  ASSERT_TRUE(backward.empty());
  ASSERT_TRUE(forward.empty());
}

TEST_F(DatabaseTest, SimpleDeleteVertexBackward) {
  storage_engine_type engine;

  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);
  engine.add_vertex(kBaseVertexSecondKey, kBaseVertexSecondValue);
  engine.add_edge(kBaseVertexKey, kBaseVertexSecondKey, kBaseEdgeValue);
  engine.delete_vertex(kBaseVertexSecondKey);

  auto backward = engine.get_backward_edges(kBaseVertexKey);
  auto forward = engine.get_forward_edges(kBaseVertexKey);
  ASSERT_FALSE(engine.get_vertex(kBaseVertexSecondKey));
  ASSERT_TRUE(backward.empty());
  ASSERT_TRUE(forward.empty());
}

TEST_F(DatabaseTest, SeveralDeleteVertexForward) {
  storage_engine_type engine;

  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);
  for (size_type i = 0; i < kSeveralNumber; ++i) {
    engine.add_vertex(static_cast<int>(i), fmt::format("{}", i));
    engine.add_edge(kBaseVertexKey, static_cast<int>(i),
                    fmt::format("e{}e", i));
  }

  engine.delete_vertex(kBaseVertexKey);

  for (size_type i = 0; i < kSeveralNumber; ++i) {
    auto backward = engine.get_backward_edges(static_cast<int>(i));
    auto forward = engine.get_forward_edges(static_cast<int>(i));
    ASSERT_TRUE(backward.empty());
    ASSERT_TRUE(forward.empty());
  }
}

TEST_F(DatabaseTest, SeveralDeleteVertexBackward) {
  storage_engine_type engine;

  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);
  for (size_type i = 0; i < kSeveralNumber; ++i) {
    engine.add_vertex(static_cast<int>(i), fmt::format("{}", i));
    engine.add_edge(static_cast<int>(i), kBaseVertexKey,
                    fmt::format("e{}e", i));
  }

  engine.delete_vertex(kBaseVertexKey);

  for (size_type i = 0; i < kSeveralNumber; ++i) {
    auto backward = engine.get_backward_edges(static_cast<int>(i));
    auto forward = engine.get_forward_edges(static_cast<int>(i));
    ASSERT_TRUE(backward.empty());
    ASSERT_TRUE(forward.empty());
  }
}

TEST_F(DatabaseTest, DeleteSeveralVertexForward) {
  storage_engine_type engine;

  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);
  for (size_type i = 0; i < kSeveralNumber; ++i) {
    engine.add_vertex(static_cast<int>(i), fmt::format("{}", i));
    engine.add_edge(kBaseVertexKey, static_cast<int>(i),
                    fmt::format("e{}e", i));
  }
  for (size_type i = 0; i < kSeveralNumber; ++i) {
    engine.delete_vertex(static_cast<int>(i));
  }

  auto backward = engine.get_backward_edges(kBaseVertexKey);
  auto forward = engine.get_forward_edges(kBaseVertexKey);
  ASSERT_TRUE(backward.empty());
  ASSERT_TRUE(forward.empty());
}

TEST_F(DatabaseTest, DeleteSeveralVertexBackward) {
  storage_engine_type engine;

  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);
  for (size_type i = 0; i < kSeveralNumber; ++i) {
    engine.add_vertex(static_cast<int>(i), fmt::format("{}", i));
    engine.add_edge(static_cast<int>(i), kBaseVertexKey,
                    fmt::format("e{}e", i));
  }
  for (size_type i = 0; i < kSeveralNumber; ++i) {
    engine.delete_vertex(static_cast<int>(i));
  }

  auto backward = engine.get_backward_edges(kBaseVertexKey);
  auto forward = engine.get_forward_edges(kBaseVertexKey);
  ASSERT_TRUE(backward.empty());
  ASSERT_TRUE(forward.empty());
}

TEST_F(DatabaseTest, SeveralDeleteVertexForwardBackward) {
  storage_engine_type engine;

  engine.add_vertex(kBaseVertexKey, kBaseVertexValue);
  for (size_type i = 0; i < kSeveralNumber; ++i) {
    engine.add_vertex(static_cast<int>(i), fmt::format("{}", i));
    engine.add_edge(static_cast<int>(i), kBaseVertexKey,
                    fmt::format("e{}e", i));
  }
  for (size_type i = kSeveralNumber; i < kSeveralNumber * 2; ++i) {
    engine.add_vertex(static_cast<int>(i), fmt::format("{}", i));
    engine.add_edge(kBaseVertexKey, static_cast<int>(i),
                    fmt::format("e{}e", i));
  }

  engine.delete_vertex(kBaseVertexKey);

  for (size_type i = 0; i < 2 * kSeveralNumber; ++i) {
    auto backward = engine.get_backward_edges(static_cast<int>(i));
    auto forward = engine.get_forward_edges(static_cast<int>(i));
    ASSERT_TRUE(backward.empty());
    ASSERT_TRUE(forward.empty());
  }
}

class WebWeaver {
 public:
  WebWeaver(storage_engine_type& storage, key_type base_key, depth_type depth,
            size_type children_per_node = kSeveralNumber)
      : storage_(storage),
        base_key_(base_key),
        depth_(depth),
        next_key_(base_key + 1),
        children_per_node_(children_per_node) {
    storage_.add_vertex(base_key_, fmt::format("{}", base_key_));
  }

  void weave() { weave_impl(base_key_, 1); }

 private:
  void weave_impl(key_type parent_key, size_type current_depth) {
    if (current_depth > depth_) {
      return;
    }
    for (size_type i = 0; i < children_per_node_; ++i) {
      const key_type child_key = next_key_++;
      storage_.add_vertex(child_key, fmt::format("{}", child_key));
      storage_.add_edge(parent_key, child_key,
                        fmt::format("e{}->{}", parent_key, child_key));
      weave_impl(child_key, current_depth + 1);
    }
  }

  storage_engine_type& storage_;
  key_type base_key_;
  depth_type depth_;
  key_type next_key_;
  size_t children_per_node_;
};

class DatabaseDepthTest : public DatabaseTest,
                          public ::testing::WithParamInterface<size_t> {
 public:
  size_type CalculatePredictedSize(size_type depth) const {
    size_type predicted_size = 0;
    size_type current_level_size = kSeveralNumber;
    for (size_type d = 1; d <= depth; ++d) {
      predicted_size += current_level_size;
      current_level_size *= kSeveralNumber;
    }
    return predicted_size;
  }

  void CheckNeighboursConsistency(storage_engine_type& engine,
                                  key_type start_key, size_type depth,
                                  size_type expected_size) {
    auto neighbours = engine.get_neighbours(start_key, depth);
    ASSERT_EQ(neighbours.size(), expected_size);

    std::ranges::sort(neighbours, {}, &decltype(neighbours)::value_type::first);
    key_type expected_key = start_key + 1;
    for (const auto& [key, fields] : neighbours) {
      ASSERT_EQ(key, expected_key);
      ASSERT_EQ(vertex_getter::get(fields, 0), fmt::format("{}", expected_key));
      ++expected_key;
    }
  }

  void WeaveComponent(storage_engine_type& engine, key_type start_key,
                      size_type depth) {
    WebWeaver weaver(engine, start_key, depth);
    weaver.weave();
  }
};

TEST_P(DatabaseDepthTest, GetNeighboursForDifferentDepths) {
  const auto depth = GetParam();
  storage_engine_type engine;

  WeaveComponent(engine, kBaseVertexKey, depth);
  const auto expected_size = CalculatePredictedSize(depth);

  CheckNeighboursConsistency(engine, kBaseVertexKey, depth, expected_size);
}

TEST_P(DatabaseDepthTest, GetNeighboursForMultipleComponents) {
  const auto depth = GetParam();
  storage_engine_type engine;
  const auto expected_size = CalculatePredictedSize(depth);

  WeaveComponent(engine, kBaseVertexKey, depth);
  CheckNeighboursConsistency(engine, kBaseVertexKey, depth, expected_size);

  WeaveComponent(engine, kBaseVertexSecondKey, depth);
  CheckNeighboursConsistency(engine, kBaseVertexSecondKey, depth,
                             expected_size);
}

INSTANTIATE_TEST_SUITE_P(DifferentDepths, DatabaseDepthTest,
                         ::testing::Values(1, 2, 3, 4));

TEST_F(DatabaseTest, GetNeighbourdsWithChain) {
  const size_type kChainLength = 20;
  storage_engine_type storage;
  storage.add_vertex(kBaseVertexKey, kBaseVertexValue);
  key_type last_key = kBaseVertexKey;
  for (size_type i = 0; i < kChainLength; ++i) {
    auto key = static_cast<int>(i);
    storage.add_vertex(key, fmt::format("{}", key));
    storage.add_edge(last_key, key, fmt::format("{}->{}", last_key, key));
    last_key = key;
  }
  for (size_type i = 0; i < kChainLength; ++i) {
    auto neighbours = storage.get_neighbours(kBaseVertexKey, i);
    ASSERT_EQ(neighbours.size(), i);
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
