# Define the utils library
add_library(storage_utils INTERFACE)

target_include_directories(storage_utils INTERFACE
    ${STORAGE_SOURCE_DIR}
    ${FMT_INCLUDE_DIRS}
)
