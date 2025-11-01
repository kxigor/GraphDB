add_library(query_module INTERFACE)

add_subdirectory(parser)
add_subdirectory(handler)

target_link_libraries(query_module INTERFACE
    query_handler
)

target_include_directories(query_module INTERFACE
    ${QUERY_SOURCE_DIR}
    ${STORAGE_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)