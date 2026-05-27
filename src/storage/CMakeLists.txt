# ====================================================================
# storage_utils — header-only utility helpers
# ====================================================================
add_library(storage_utils INTERFACE)
add_library(graphdb::storage_utils ALIAS storage_utils)

target_include_directories(storage_utils INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/storage>
)
target_link_libraries(storage_utils INTERFACE fmt::fmt)
target_compile_features(storage_utils INTERFACE cxx_std_20)

# ====================================================================
# storage_files — header-only file layer
# ====================================================================
add_library(storage_files INTERFACE)
add_library(graphdb::storage_files ALIAS storage_files)

target_include_directories(storage_files INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/storage>
)
target_link_libraries(storage_files INTERFACE
  graphdb::storage_utils
  fmt::fmt
)
target_compile_features(storage_files INTERFACE cxx_std_20)

# ====================================================================
# storage_engine — header-only engine layer
# ====================================================================
add_library(storage_engine INTERFACE)
add_library(graphdb::storage_engine ALIAS storage_engine)

target_include_directories(storage_engine INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/storage>
)
target_link_libraries(storage_engine INTERFACE
  graphdb::storage_utils
  graphdb::storage_files
)
target_compile_features(storage_engine INTERFACE cxx_std_20)

# ====================================================================
# storage_manager — compiled manager + DB lifecycle
# Uses Boost.Process for compile_graph(); see GRAPHDB_BUILD_MANAGER.
# ====================================================================
if(GRAPHDB_BUILD_MANAGER)
  add_library(storage_manager STATIC
    manager/storage_manager.cpp
    manager/storage_manager.hpp
    manager/storage_manager_patterns.hpp
    manager/storage_basic_processor.hpp
  )
  add_library(graphdb::storage_manager ALIAS storage_manager)

  target_include_directories(storage_manager PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/storage>
  )
  target_link_libraries(storage_manager
    PUBLIC
      graphdb::storage_utils
      graphdb::storage_files
      graphdb::storage_engine
      fmt::fmt
    PRIVATE
      Boost::filesystem
      Boost::system
  )
  target_compile_features(storage_manager PUBLIC cxx_std_20)
endif()

# ====================================================================
# graphdb::storage — public umbrella target
# ====================================================================
add_library(storage INTERFACE)
add_library(graphdb::storage ALIAS storage)

target_link_libraries(storage INTERFACE
  graphdb::storage_engine
  graphdb::storage_files
  graphdb::storage_utils
)
if(GRAPHDB_BUILD_MANAGER)
  target_link_libraries(storage INTERFACE graphdb::storage_manager)
endif()
