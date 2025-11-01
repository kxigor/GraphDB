
add_library(storage_manager
    storage_manager.cpp
    storage_manager.hpp
    storage_manager_patterns.hpp
)

find_package(Boost REQUIRED COMPONENTS filesystem system)
target_include_directories(storage_manager PUBLIC ${FMT_INCLUDE_DIRS})
target_link_directories(storage_manager PUBLIC ${FMT_LIBRARY_DIRS})
target_link_libraries(storage_manager PUBLIC ${FMT_LIBRARIES})
target_link_libraries(storage_manager PRIVATE Boost::filesystem Boost::system)
target_link_libraries(storage_manager PUBLIC
    storage_utils
)
