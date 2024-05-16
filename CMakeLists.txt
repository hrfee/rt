cmake_minimum_required(VERSION 3.14)

project(
    rt 
    VERSION 0.0
    DESCRIPTION "COMP3931 Individual Project - rt"
    LANGUAGES CXX
)

add_compile_options(-O3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# if(NOT CMAKE_BUILD_TYPE)
#   set(CMAKE_BUILD_TYPE Release)
# endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -ffast-math -march=native")
set(CMAKE_BUILD_TYPE Release)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

find_package(glfw REQUIRED)

add_subdirectory(third_party)

add_subdirectory(src)

add_subdirectory(main)

add_subdirectory(timings)

add_subdirectory(tgareader)
