#pragma once
#include <fmt/core.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

#include "utils/storage_config.hpp"

namespace storage {
namespace files::errors {
struct BadSyscall : public std::runtime_error {
  explicit BadSyscall(const cesrt_type& syscallname)
      : std::runtime_error(fmt::format("System call {} failed (errno={}): {}",
                                       syscallname, errno,
                                       std::strerror(errno))),
        syscallname(syscallname) {}

  cesrt_type syscallname;
};

struct BadPosition : public std::runtime_error {
  explicit BadPosition(size_type requested_position, size_type max_position)
      : std::runtime_error(fmt::format("Invalid position: {} (max_position={})",
                                       requested_position, max_position)),
        requested_position(requested_position),
        max_position(max_position) {}

  size_type requested_position;
  size_type max_position;
};

struct BadMapping : public std::runtime_error {
  BadMapping() : std::runtime_error("Invalid mapping status") {}
};
}  // namespace files::errors

namespace engine::errors {
struct BadKey : public std::runtime_error {
  /*TODO mb key to complex for conversation*/
  explicit BadKey(const auto& key)
      : std::runtime_error(fmt::format("Invalid key: {}", key)) {}
};

struct BadEdge : public std::runtime_error {
  explicit BadEdge(const id_type& id)
      : std::runtime_error(fmt::format("Invalid id: {}", id)) {}
};

struct BrokenVertex : public std::runtime_error {
  explicit BrokenVertex(const id_type& id)
      : std::runtime_error(fmt::format("Broken Vertex on ID: {}", id)) {}
};
};  // namespace engine::errors

namespace pod::errors {
struct MapFullInsert : std::out_of_range {
  MapFullInsert() : std::out_of_range(fmt::format("Insert in full map")) {}
};

struct ArrayOutOfRange : std::out_of_range {
  ArrayOutOfRange(size_type pos, size_type size)
      : std::out_of_range(fmt::format("MarkedArray size: {}, requested pos: {}",
                                      size, pos)) {}
};
}  // namespace pod::errors

}  // namespace storage