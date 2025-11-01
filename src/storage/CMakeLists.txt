add_library(storage_module INTERFACE)

add_subdirectory(engine)
add_subdirectory(manager)
add_subdirectory(files)
add_subdirectory(utils)

link_directories(/opt/homebrew/Cellar/fmt/11.1.4/lib)

target_link_libraries(storage_module INTERFACE
    storage_engine
    storage_manager
    storage_files
    storage_utils
)

target_include_directories(storage_module INTERFACE
    ${STORAGE_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${FMT_INCLUDE_DIRS}
)