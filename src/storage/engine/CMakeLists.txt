add_library(storage_engine INTERFACE)

target_include_directories(storage_engine INTERFACE
    ${STORAGE_SOURCE_DIR}
)

target_link_libraries(storage_engine INTERFACE
    storage_utils
    storage_files
)
