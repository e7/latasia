cmake_minimum_required(VERSION 2.8)
project(latacia)

set(CMAKE_BUILD_TYPE Debug)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR})

add_definitions(-Wall)
# add_definitions(-Wall -Wextra)
add_definitions(-D_GNU_SOURCE)
add_definitions(-ggdb -O0 -std=gnu99 -pipe -rdynamic)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(-D_DEBUG)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(/usr/local/lib64)
add_subdirectory(src)
