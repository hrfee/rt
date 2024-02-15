set(IMGUI "imgui")
file(GLOB IMGUI_HEADER_LIST
    "${IMGUI}/*.h"
    "${IMGUI}/backends/imgui_impl_glfw.h"
    "${IMGUI}/misc/cpp/imgui_stdlib.h"
    "${IMGUI}/backends/imgui_impl_opengl3.h"
)
file(GLOB IMGUI_SRC_LIST
    "${IMGUI}/*.cpp"
    "${IMGUI}/backends/imgui_impl_glfw.cpp"
    "${IMGUI}/backends/imgui_impl_opengl3.cpp"
    "${IMGUI}/misc/cpp/imgui_stdlib.cpp"
)

project(third_party)

add_library(glad glad/src/glad.c glad/include/glad/glad.h)
set_target_properties(glad PROPERTIES LINKER_LANGUAGE C)
target_include_directories(glad PUBLIC glad/include/)

add_library(imgui STATIC ${IMGUI_SRC_LIST} ${IMGUI_HEADER_LIST})
set_target_properties(imgui PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(imgui PUBLIC ${IMGUI} "${IMGUI}/misc/cpp/" "${IMGUI}/backends/")
