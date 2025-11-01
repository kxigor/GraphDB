#pragma once

#include <fmt/core.h>

#include "utils/storage_config.hpp"

namespace storage::path_generator {
path_type generate_path(const cesrt_type& path_name,
                        const cesrt_type& entity_name,
                        const auto& file_number) {
  return fmt::format("{}/{}_{}.{}", path_name, entity_name, file_number,
                     kDefaultEntityExtension);
}
path_type generate_map_path(const auto& identifier) {
  return generate_path(kMapsDirName, kMapEntityName, identifier);
}
path_type generate_ids_path(const auto& identifier) {
  return generate_path(kKeysDirName, kKeyEntityName, identifier);
}
path_type generate_vertex_path(const auto& identifier) {
  return generate_path(kVertexesDirName, kVertexesEntityName, identifier);
}
path_type generate_forward_edge_path(const auto& identifier) {
  return generate_path(kEdgesDirName, kForwardEdgeEntityName, identifier);
}
path_type generate_backward_edge_path(const auto& identifier) {
  return generate_path(kEdgesDirName, kReverseEdgeEntityName, identifier);
}
}  // namespace storage::path_generator