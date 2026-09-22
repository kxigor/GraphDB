# ========================================
# 1. FINDING ALL FILES FOR STATIC ANALYSIS
# ========================================

file(GLOB_RECURSE ALL_SRCS CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/apps/*.cpp"
  "${PROJECT_SOURCE_DIR}/apps/*.hpp"
  "${PROJECT_SOURCE_DIR}/src/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/*.hpp"
)

# Formatting applies to the whole repository. Static analysis needs the
# compiler flags and dependencies of modules enabled in this build.
set(TIDY_SRCS ${ALL_SRCS})
if(NOT GRAPHDB_BUILD_MANAGER)
  list(FILTER TIDY_SRCS EXCLUDE REGEX "/src/storage/manager/")
endif()
if(NOT GRAPHDB_BUILD_QUERY)
  list(FILTER TIDY_SRCS EXCLUDE REGEX "/src/query/")
endif()
if(NOT GRAPHDB_BUILD_APPS)
  list(FILTER TIDY_SRCS EXCLUDE REGEX "/apps/")
endif()

# ========================================
# 2. SETTING SCRIPTS/CONFIGS
# ========================================

set(SCRIPTS_DIR "${PROJECT_SOURCE_DIR}/tests/scripts")
set(FORMAT_CONFIG "${PROJECT_SOURCE_DIR}/.clang-format")
set(TIDY_CONFIG "${PROJECT_SOURCE_DIR}/.clang-tidy")

# ========================================
# 3. SETTING TESTS
# ========================================

if(GRAPHDB_ENABLE_FORMAT_CHECK)
  add_test(
    NAME FormatCheck
    COMMAND bash ${SCRIPTS_DIR}/check_format.sh
    --format-exec ${CLANG_FORMAT_EXE}
    --config ${FORMAT_CONFIG}
    --files ${ALL_SRCS}
  )
endif()

if(GRAPHDB_ENABLE_TIDY_CHECK)
  add_test(
    NAME TidyCheck
    COMMAND ${Python3_EXECUTABLE} ${SCRIPTS_DIR}/check_tidy.py
    --tidy-exec ${CLANG_TIDY_EXE}
    --build-dir ${CMAKE_BINARY_DIR}
    --config ${TIDY_CONFIG}
    ${TIDY_SRCS}
  )
endif()
