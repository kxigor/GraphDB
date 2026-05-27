#include <gtest/gtest.h>

#include <array>
#include <memory>

#include "files/storage_pod_structures.hpp"

static constexpr const std::size_t kMapSize = 1024;
const auto kLeftBound = kMapSize / 4 * 1;
const auto kRightBound = kMapSize / 4 * 3;
using key_type = int;
using value_type = std::size_t;
using map_type = storage::pod::KeylessMap<value_type, kMapSize>;
using array_type = std::array<key_type, kMapSize>;
using value_type_hash = std::hash<value_type>;
using hash_type = std::hash<key_type>;

struct BaseKeylessMapTests {
  auto MakeEqualFn() const {
    return [this](const key_type& key, const value_type& value) {
      return (*keys_)[value] == key;
    };
  }

  auto MakeKeyGetter() const {
    return [this](const value_type& value) { return (*keys_)[value]; };
  }

  void InsertRange(auto& map, std::size_t from, std::size_t to) {
    for (std::size_t i = from; i < to; ++i) {
      const auto key = static_cast<key_type>(i * i);
      map->insert(key, hash_, i, MakeEqualFn());
      (*keys_)[i] = key;
    }
  }

  void EraseRange(auto& map, std::size_t from, std::size_t to) {
    for (std::size_t i = from; i < to; ++i) {
      const auto key = static_cast<key_type>(i * i);
      map->erase(key, hash_, MakeEqualFn());
    }
  }

  void VerifyRangeExisting(auto& map, std::size_t from, std::size_t to) const {
    for (std::size_t i = from; i < to; ++i) {
      const auto key = static_cast<key_type>(i * i);
      ASSERT_EQ(map->find(key, hash_, MakeEqualFn()), i);
    }
  }

  void VerifyRangeMissing(auto& map, std::size_t from, std::size_t to) const {
    for (std::size_t i = from; i < to; ++i) {
      const auto key = static_cast<key_type>(i * i);
      ASSERT_FALSE(map->find(key, hash_, MakeEqualFn()));
    }
  }

  std::unique_ptr<array_type> keys_;
  hash_type hash_;
};

class SingleOperationsTest : public BaseKeylessMapTests,
                             public ::testing::Test {
 protected:
  void SetUp() override {
    map_ = std::make_unique<map_type>();
    keys_ = std::make_unique<array_type>();
  }
  std::unique_ptr<map_type> map_;
};

class RehashOperationsTest : public BaseKeylessMapTests,
                             public ::testing::Test {
 protected:
  void SetUp() override {
    map_ = std::make_unique<map_type>();
    rehashed_map_ = std::make_unique<map_type>();
    keys_ = std::make_unique<array_type>();
  }

  void InitRehashedMap() {
    map_->rehash_to_buffer(reinterpret_cast<char*>(rehashed_map_.get()),
                           MakeKeyGetter(), hash_, MakeEqualFn());
  }
  std::unique_ptr<map_type> map_;
  std::unique_ptr<map_type> rehashed_map_;
};

TEST_F(SingleOperationsTest, SimpleInsert) {
  InsertRange(map_, 1, 2);
  VerifyRangeExisting(map_, 1, 2);
  ASSERT_FALSE(map_->is_full());
  ASSERT_EQ(map_->size(), 1);
}

TEST_F(SingleOperationsTest, FullInsert) {
  InsertRange(map_, 0, kMapSize);
  VerifyRangeExisting(map_, 0, kMapSize);
  ASSERT_TRUE(map_->is_full());
  ASSERT_EQ(map_->size(), kMapSize);
}

TEST_F(SingleOperationsTest, SegmentInsert) {
  InsertRange(map_, kLeftBound, kRightBound);
  VerifyRangeExisting(map_, kLeftBound, kRightBound);
  VerifyRangeMissing(map_, 0, kLeftBound);
  VerifyRangeMissing(map_, kRightBound, kMapSize);
  ASSERT_EQ(map_->size(), kRightBound - kLeftBound);
}

TEST_F(SingleOperationsTest, SimpleErase) {
  InsertRange(map_, 1, 2);
  EraseRange(map_, 1, 2);
  VerifyRangeMissing(map_, 1, 2);
}

TEST_F(SingleOperationsTest, FullErase) {
  InsertRange(map_, 0, kMapSize);
  EraseRange(map_, 0, kMapSize);
  VerifyRangeMissing(map_, 0, kMapSize);
}

TEST_F(SingleOperationsTest, SegmentErase) {
  InsertRange(map_, 0, kMapSize);
  EraseRange(map_, kLeftBound, kRightBound);
  VerifyRangeExisting(map_, 0, kLeftBound);
  VerifyRangeMissing(map_, kLeftBound, kRightBound);
  VerifyRangeExisting(map_, kRightBound, kMapSize);
}

TEST_F(SingleOperationsTest, EraseWithCollisions) {
  InsertRange(map_, 0, kMapSize);
  EraseRange(map_, 0, kMapSize - 1);
  VerifyRangeMissing(map_, 0, kMapSize - 1);
  VerifyRangeExisting(map_, kMapSize - 1, kMapSize);
}

TEST_F(SingleOperationsTest, Overflow) {
  InsertRange(map_, 0, kMapSize);
  ASSERT_THROW(InsertRange(map_, 1, 2), std::out_of_range);
}

TEST_F(RehashOperationsTest, SimpleRehash) {
  InsertRange(map_, 0, kMapSize);
  InitRehashedMap();
  VerifyRangeExisting(rehashed_map_, 0, kMapSize);
  ASSERT_TRUE(rehashed_map_->is_full());
}

TEST_F(RehashOperationsTest, SegmentRehash) {
  InsertRange(map_, kLeftBound, kRightBound);
  InitRehashedMap();
  VerifyRangeMissing(rehashed_map_, 0, kLeftBound);
  VerifyRangeExisting(rehashed_map_, kLeftBound, kRightBound);
  VerifyRangeMissing(rehashed_map_, kRightBound, kMapSize);
}

TEST_F(RehashOperationsTest, SegmentsRehash) {
  InsertRange(map_, 0, kMapSize);
  EraseRange(map_, kLeftBound, kRightBound);
  InitRehashedMap();
  VerifyRangeExisting(rehashed_map_, 0, kLeftBound);
  VerifyRangeMissing(rehashed_map_, kLeftBound, kRightBound);
  VerifyRangeExisting(rehashed_map_, kRightBound, kMapSize);
  ASSERT_FALSE(rehashed_map_->is_full());
  ASSERT_EQ(rehashed_map_->size(), kMapSize - (kRightBound - kLeftBound));
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}