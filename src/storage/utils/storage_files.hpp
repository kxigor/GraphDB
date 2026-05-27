#include "storage_config.hpp"

namespace storage::files {
struct FileArrayPos {
  size_type file_num;
  size_type file_pos;
};

FileArrayPos calculate_file_pos(const auto& identifier, const auto& file_size) {
  return {.file_num = identifier / file_size,
          .file_pos = identifier % file_size};
}

}  // namespace storage::files