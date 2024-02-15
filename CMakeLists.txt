cmake_minimum_required(VERSION 3.14)

project(
    rt 
    VERSION 0.0
    DESCRIPTION "COMP3931 Individual Project - rt"
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

find_package(glfw REQUIRED)

add_subdirectory(third_party)

add_subdirectory(src)

add_subdirectory(main)
