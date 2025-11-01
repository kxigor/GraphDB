
add_library(storage_files INTERFACE)

target_include_directories(storage_files INTERFACE
    ${STORAGE_SOURCE_DIR}
    ${FMT_INCLUDE_DIRS}
)

target_link_libraries(storage_files INTERFACE
    storage_utils
)
