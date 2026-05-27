add_subdirectory(storage)

if(GRAPHDB_BUILD_QUERY)
  add_subdirectory(query)
endif()
