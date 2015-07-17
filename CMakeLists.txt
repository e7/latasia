cmake_minimum_required(VERSION 2.8)
project(latacia)

include (CheckFunctionExists)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR})

add_definitions(-Wall)
# add_definitions(-Wall -Wextra)
add_definitions(-D_GNU_SOURCE -DPROJECT_ROOT=${PROJECT_SOURCE_DIR})
add_definitions(-ggdb -O0 -std=gnu99 -pipe -rdynamic)

check_function_exists(epoll_create1 HAVE_FUNCTION_EPOLL_CREATE1)
check_function_exists(epoll_pwait HAVE_FUNCTION_EPOLL_PWAIT)
check_function_exists(accept4 HAVE_FUNCTION_ACCEPT4)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(-D_DEBUG)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(/usr/local/lib64)
add_subdirectory(src)
