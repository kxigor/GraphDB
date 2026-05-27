# ====================================================================
# query_parser — Boost.Spirit X3 query grammar
# ====================================================================
add_library(query_parser STATIC
  parser/query_parser.cpp
  parser/query_parser.hpp
  parser/query_parser_def.hpp
  parser/query_parser_rules.hpp
  parser/ast.hpp
  parser/ast_adapted.hpp
  parser/config.hpp
)
add_library(graphdb::query_parser ALIAS query_parser)

target_include_directories(query_parser PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/query>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/query/parser>
)
target_link_libraries(query_parser PUBLIC Boost::headers)
target_compile_features(query_parser PUBLIC cxx_std_20)

# ====================================================================
# query_handler — top-level query dispatch + AST execution
# ====================================================================
add_library(query_handler STATIC
  handler/query_handler.cpp
  handler/query_handler.hpp
)
add_library(graphdb::query_handler ALIAS query_handler)

target_include_directories(query_handler PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/query>
)
target_link_libraries(query_handler
  PUBLIC
    graphdb::query_parser
    graphdb::storage
  PRIVATE
    Boost::serialization
)
target_compile_features(query_handler PUBLIC cxx_std_20)

# ====================================================================
# graphdb::query — public umbrella target
# ====================================================================
add_library(query INTERFACE)
add_library(graphdb::query ALIAS query)

target_link_libraries(query INTERFACE
  graphdb::query_parser
  graphdb::query_handler
)
