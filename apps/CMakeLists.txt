add_executable(graphdb graphdb/main.cpp)

target_link_libraries(graphdb PRIVATE
  graphdb::query
  graphdb::storage
  project_options
  project_sanitizers
  project_coverage
)
