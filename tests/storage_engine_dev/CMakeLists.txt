link_libraries(storage_module)
include_directories(
    ${PROJECT_SOURCE_DIR}/src/storage
)

# Adding format test
set(CLANG_FORMAT_SCRIPT ${PROJECT_SOURCE_DIR}/tests/clang_format_tests.sh)

include_directories(
    ${PROJECT_SOURCE_DIR}/src/storage
)

file(GLOB_RECURSE SRCS_FOR_FORMAT
    "${PROJECT_SOURCE_DIR}/src/storage/**/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/storage/**/*.cpp"
)

add_test(
    NAME FormatCheck
    COMMAND ${CLANG_FORMAT_SCRIPT} ${SRCS_FOR_FORMAT}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Adding tidy test
# TODO: improve codestyle
set(CLANG_TIDY_SCRIPT 
    ${PROJECT_SOURCE_DIR}/tests/clang_tidy_tests.sh
)
set(SRCS_FOR_TIDY ${SRCS_FOR_FORMAT})
add_test(
    NAME TidyCheck
    COMMAND ${CLANG_TIDY_SCRIPT} ${SRCS_FOR_TIDY}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
