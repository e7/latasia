cmake_minimum_required(VERSION 2.8)
project(latacia)

include(CheckFunctionExists)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR})

SET(CMAKE_C_FLAGS "-Wall -ggdb -O0 -pipe -std=gnu99 -rdynamic")
SET(CMAKE_C_FLAGS "-Wl,--wrap=memcpy")
#SET(CMAKE_C_FLAGS "-W -Wall -Wextra -pedantic -ggdb -O0 -pipe -std=c99 -rdynamic")
# add_definitions(-Wall -Wextra)
add_definitions(-D_GNU_SOURCE -DPROJECT_ROOT=${PROJECT_SOURCE_DIR} -DADV_STRING_ENHANCE)

check_function_exists(epoll_create1 HAVE_FUNCTION_EPOLL_CREATE1)
check_function_exists(epoll_pwait HAVE_FUNCTION_EPOLL_PWAIT)
check_function_exists(accept4 HAVE_FUNCTION_ACCEPT4)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(-D_DEBUG)
endif()

include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_SOURCE_DIR}/lib /usr/local/lib64)
add_subdirectory(src)

configure_file(
    "${PROJECT_SOURCE_DIR}/include/build.h.in"
    "${PROJECT_SOURCE_DIR}/include/build.h"
)
