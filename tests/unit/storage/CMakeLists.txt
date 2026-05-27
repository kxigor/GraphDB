function(add_storage_test name)
  add_gtest(${name}
    SOURCES ${ARGN}
    LIBRARIES
      graphdb::storage
      project_options
      project_sanitizers
      project_coverage
  )
endfunction()

# --- Engine ---
add_storage_test(Storage_Engine_Basic       Engine/basic_test.cpp)
add_storage_test(Storage_Engine_LargeKey    Engine/large_key_test.cpp)
add_storage_test(Storage_Engine_LargeVertex Engine/large_vertex_test.cpp)

# --- Manager ---
# Needs storage::manager (compiled .cpp) which needs Boost.Process v1.
if(GRAPHDB_BUILD_MANAGER)
  add_storage_test(Storage_Manager_Basic    Manager/basic_test.cpp)
endif()

# --- POD ---
add_storage_test(Storage_POD_Stress         POD/stress_test.cpp)

# `test_helper.hpp` is shared between Engine tests via a relative include
# (it sits next to the .cpp files), so no extra include_directories are needed.
