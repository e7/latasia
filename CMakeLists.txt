cmake_minimum_required(VERSION 2.8)
project(latacia)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

add_definitions(-Wall)
# add_definitions(-Wall -Wextra)
add_definitions(-std=gnu99)
add_definitions(-D_GNU_SOURCE)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(-D_DEBUG)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(/usr/local/lib64)
add_subdirectory(src)
