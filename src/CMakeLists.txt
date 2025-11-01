add_executable(graph_database_main main.cpp)

add_subdirectory(storage)

target_include_directories(graph_database_main PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(graph_database_main PRIVATE storage_module)

message(WARNING "fmt_LIBS: ${FMT_LIBRARY_DIRS}")
