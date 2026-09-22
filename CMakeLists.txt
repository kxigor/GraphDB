cmake_minimum_required(VERSION 3.25)

project(GraphDB
  VERSION 0.1.0
  LANGUAGES CXX
)

# =========================================================
# 0. STANDALONE vs SUBPROJECT
# =========================================================
if(PROJECT_IS_TOP_LEVEL)
  set(GRAPHDB_DEV_DEFAULT ON)
else()
  set(GRAPHDB_DEV_DEFAULT OFF)
endif()

# =========================================================
# 1. OPTIONS
# =========================================================
option(GRAPHDB_BUILD_APPS            "Build the graphdb executable"                              ${GRAPHDB_DEV_DEFAULT})
option(GRAPHDB_BUILD_MANAGER         "Build storage::manager (needs Boost.Process v1 API)"       OFF)
option(GRAPHDB_BUILD_QUERY           "Build the query module (needs Boost.Process v1 API)"       OFF)
option(GRAPHDB_BUILD_TESTS           "Build the GTest test suite"                                ${GRAPHDB_DEV_DEFAULT})

# Cascade: app needs query, query needs manager.
if(NOT GRAPHDB_BUILD_MANAGER)
  set(GRAPHDB_BUILD_QUERY OFF CACHE BOOL "Disabled: requires GRAPHDB_BUILD_MANAGER=ON" FORCE)
endif()
if(NOT GRAPHDB_BUILD_QUERY)
  set(GRAPHDB_BUILD_APPS OFF CACHE BOOL "Disabled: requires GRAPHDB_BUILD_QUERY=ON" FORCE)
endif()
option(GRAPHDB_ENABLE_FORMAT_CHECK "Register clang-format as CTest"      ${GRAPHDB_DEV_DEFAULT})
option(GRAPHDB_ENABLE_TIDY_CHECK   "Register clang-tidy as CTest"        ${GRAPHDB_DEV_DEFAULT})
option(GRAPHDB_ENABLE_COVERAGE     "Enable gcov/lcov coverage targets"   OFF)

if(NOT GRAPHDB_BUILD_TESTS)
  set(GRAPHDB_ENABLE_FORMAT_CHECK OFF CACHE BOOL "Disabled: requires GRAPHDB_BUILD_TESTS=ON" FORCE)
  set(GRAPHDB_ENABLE_TIDY_CHECK   OFF CACHE BOOL "Disabled: requires GRAPHDB_BUILD_TESTS=ON" FORCE)
  set(GRAPHDB_ENABLE_COVERAGE     OFF CACHE BOOL "Disabled: requires GRAPHDB_BUILD_TESTS=ON" FORCE)
endif()

# =========================================================
# 2. TOP-LEVEL-ONLY GLOBAL SETTINGS
# =========================================================
if(PROJECT_IS_TOP_LEVEL)
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
  endif()
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endif()

# =========================================================
# 3. EXTERNAL DEPENDENCIES
# =========================================================
find_package(Boost REQUIRED COMPONENTS filesystem system serialization)

# fmt 11.1 fixes compile-time format checks with recent Clang versions.
# Prefer a compatible system install, otherwise fetch the pinned release.
find_package(fmt 11.1 CONFIG QUIET)
if(NOT fmt_FOUND)
  message(STATUS "fmt >= 11.1 not found — pulling 11.2.0 via FetchContent")
  include(FetchContent)
  FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.2.0
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(fmt)
endif()

# Treat fmt headers as system to silence -Winline (and other) warnings
# raised by its template internals.
if(TARGET fmt)
  set_target_properties(fmt PROPERTIES SYSTEM TRUE)
endif()

# =========================================================
# 4. LIBRARIES
# =========================================================
add_subdirectory(src)

# =========================================================
# 5. DEV-ONLY MACHINERY
# =========================================================
if(GRAPHDB_BUILD_APPS OR GRAPHDB_BUILD_TESTS OR GRAPHDB_ENABLE_COVERAGE)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
  include(ProjectOptions)
  include(ProjectSanitizers)
  include(ProjectCoverage)
endif()

if(GRAPHDB_BUILD_APPS)
  add_subdirectory(apps)
endif()

if(GRAPHDB_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
