#pragma once

#include <gtest/gtest.h>

#include <filesystem>

#include "utils/storage_config.hpp"

void RaiseBase();
void DemonishBase();

void RaiseBase() {
  DemonishBase();
  std::filesystem::create_directory(storage::kMapsDirName);
  std::filesystem::create_directory(storage::kVertexesDirName);
  std::filesystem::create_directory(storage::kEdgesDirName);
  std::filesystem::create_directory(storage::kKeysDirName);
}

void DemonishBase() {
  std::filesystem::remove(storage::kKeysConfigFileName);
  std::filesystem::remove(storage::kVertexesConfigFileName);
  std::filesystem::remove(storage::kEdgesConfigFileName);
  std::filesystem::remove_all(storage::kMapsDirName);
  std::filesystem::remove_all(storage::kVertexesDirName);
  std::filesystem::remove_all(storage::kEdgesDirName);
  std::filesystem::remove_all(storage::kKeysDirName);
}

class DatabaseTest : public ::testing::Test {
 protected:
  void SetUp() override { RaiseBase(); }
  void TearDown() override { DemonishBase(); }
};
