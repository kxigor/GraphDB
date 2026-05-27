# ========================================
# 1. FINDING ALL FILES FOR STATIC ANALYSIS
# ========================================

file(GLOB_RECURSE ALL_SRCS CONFIGURE_DEPENDS
  "${PROJECT_SOURCE_DIR}/apps/*.cpp"
  "${PROJECT_SOURCE_DIR}/apps/*.hpp"
  "${PROJECT_SOURCE_DIR}/src/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/*.hpp"
)

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
    ${ALL_SRCS}
  )
endif()
