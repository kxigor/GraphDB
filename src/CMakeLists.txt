
add_executable(db main.cpp)

set(QUERY_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/query)
set(STORAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/storage)

add_subdirectory(storage)
add_subdirectory(query)

target_link_libraries(db PRIVATE query_module)