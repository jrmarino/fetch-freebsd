cmake_minimum_required(VERSION 3.16)
project(fetch-freebsd)

SET(CMAKE_SKIP_BUILD_RPATH  TRUE)

option(FETCH_PROGRAM "Build fetch program" ON)
option(FETCH_LIBRARY "Build fetch library" OFF)
option(USE_SYSTEM_SSL "Don't statically link the Ravenport-compatible SSL" OFF)

if(FETCH_PROGRAM)
	add_subdirectory(program)
endif()

if(FETCH_LIBRARY)
	add_subdirectory(library)
endif()
