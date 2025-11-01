cmake_minimum_required(VERSION 3.16)
project(GraphDatabase LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra -Werror")


find_package(Boost REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FMT REQUIRED fmt)


add_subdirectory(src)