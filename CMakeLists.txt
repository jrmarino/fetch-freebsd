cmake_minimum_required(VERSION 3.16)
project(fetch-freebsd)

SET(CMAKE_SKIP_BUILD_RPATH  TRUE)

add_subdirectory(program)
add_subdirectory(library)
