
add_library(query_handler
    query_handler.cpp
)

target_include_directories(query_handler INTERFACE
    ${STORAGE_SOURCE_DIR}
    ${QUERY_SOURCE_DIR}
)


find_package(Boost REQUIRED COMPONENTS serialization)
target_link_libraries(query_handler PRIVATE Boost::serialization)
target_link_libraries(query_handler PUBLIC
    storage_module
    query_parser
)