#pragma once

namespace storage::manager::patterns {
constexpr auto kNameVariablePattern =
    R"###(extern constexpr const char {}[] = "{}";
)###";

constexpr auto kTypePattern = R"###(
  storage::NamedType<{}, {}>)###";

constexpr auto kFieldsPattern = R"###(storage::tuple_type<{}
>)###";

constexpr auto kCodePattern = R"###(
#include <iostream>

#include "engine/storage_engine.hpp"
#include "graph/processor.hpp"

using key_type = {0};

{1} // names
using vertex_field_type = {2};

{3} // names
using vertex_edge_type = {4};


using hash_func = std::hash<key_type>;

using storage_engine_type =
    storage::engine::StorageEngine<key_type, vertex_field_type,
                                   vertex_edge_type, hash_func>;

int main() {{
  storage_engine_type storage_engine;
  Processor processor(storage_engine);
  processor.launch();
}}

)###";

}  // namespace storage::manager::patterns