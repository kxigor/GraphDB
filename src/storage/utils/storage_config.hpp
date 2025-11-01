#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "utils/storage_type_list.hpp"

// clang-format off
namespace storage {

using allowed_types = details::TypeList<bool, int, std::string>;

template <typename ValueType>
concept AllowableValueType = details::IsTypeInList<ValueType, allowed_types>::value;

using text_type         = std::string;
using hash_type         = std::size_t;
using size_type         = std::size_t;
using cesrt_type        = std::string_view;
using path_type         = std::filesystem::path;
using id_type           = std::size_t;
using file_ptr_type     = std::size_t;
using file_offset_type  = std::size_t;
using depth_type        = std::size_t;

template <typename... Ts>
using tuple_type = std::tuple<Ts...>;

template <typename T>
using pure_type = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T>
using container_type = std::vector<T>;

template <typename T>
using unordered_set_type = std::unordered_set<T>;

template <typename T>
using stack_type = std::vector<T>;

template <typename T>
using queque_type = std::queue<T>;

template <typename T>
using optional_type = std::optional<T>;

template <typename TypingType, typename Type>
using typed_type = std::pair<TypingType, Type>;

using ids_type = container_type<id_type>;

template <typename VertexType>
struct VertexUsings {
  template <typename T>
  using typed_vertex_type = typed_type<T, VertexType>;
  using id_vertex_type    = typed_vertex_type<id_type>;

  template <typename T>
  using typed_vertexes_type = container_type<typed_vertex_type<T>>;
  using ids_vertexes_type   = typed_vertexes_type<id_type>;
};

template <typename EdgeType>
struct EdgeUsings {
  template <typename T>
  using typed_edge_type = typed_type<T, EdgeType>;
  using id_edge_type    = typed_edge_type<id_type>;

  template <typename T>
  using typed_edges_type  = container_type<typed_edge_type<T>>;
  using ids_edges_type    = typed_edges_type<id_type>;
};

static constexpr const cesrt_type kMapsDirName            = "maps";
static constexpr const cesrt_type kVertexesDirName        = "vertexes";
static constexpr const cesrt_type kEdgesDirName           = "edges";
static constexpr const cesrt_type kKeysDirName            = "keys";
static constexpr const cesrt_type kMapEntityName          = "map";
static constexpr const cesrt_type kVertexesEntityName     = "vertex";
static constexpr const cesrt_type kForwardEdgeEntityName  = "for_edge";
static constexpr const cesrt_type kReverseEdgeEntityName  = "rev_edge";
static constexpr const cesrt_type kKeyEntityName          = "key";
static constexpr const cesrt_type kKeysConfigFileName     = "KeyConfig.cfg";
static constexpr const cesrt_type kVertexesConfigFileName = "VertexesConfig.cfg";
static constexpr const cesrt_type kEdgesConfigFileName    = "EdgesConfig.cfg";
static constexpr const cesrt_type kBinaryFileName         = "graph.out";
static constexpr const cesrt_type kCodeFileName           = "graph.cpp";
static constexpr const cesrt_type kDefaultEntityExtension = "db";

static constexpr const cesrt_type kGraphCompileScriptPath = "src/storage/manager/graph_compiler.sh";
static constexpr const cesrt_type kGraphCompileScriptFileName = "graph_compiler.sh";

static constexpr const size_type kDefaultMapCapacity        = 0b10000000000;
static constexpr const size_type kDefaultVertexArrayCapacity= 0b10000000;
static constexpr const size_type kDefaultKeyArrayCapacity   = 0b10000000;

static_assert(
  kDefaultMapCapacity > 0 &&
  (kDefaultMapCapacity & (kDefaultMapCapacity - 1)) == 0,
  "kDefaultMapCapacity must be a power of two"
);

template <const char* Name, typename ValueType>
requires AllowableValueType<ValueType>
struct NamedType {
  using name_type   = cesrt_type;
  using value_type  = ValueType;

  static constexpr const name_type kName = Name;

  template <typename... Args>
  NamedType(Args&&... args) : value(std::forward<Args>(args)...) {}

  value_type value;
};

struct FileArrayPos {
  size_type file_num;
  size_type file_pos;
};
// clang-format on

};  // namespace storage