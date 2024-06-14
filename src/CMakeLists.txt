file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${rt_SOURCE_DIR}/include/*.hpp")

add_compile_options(-O3)

add_library(util STATIC util.cpp ${HEADER_LIST})
set_target_properties(util PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(util PUBLIC ../include)

add_library(shape STATIC shape.cpp ${HEADER_LIST})
set_target_properties(shape PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(shape PUBLIC ../include)
target_link_libraries(shape PUBLIC vec tex)
target_link_libraries(shape PRIVATE util)

add_library(vec STATIC ${HEADER_LIST})
set_target_properties(vec PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(vec PUBLIC ../include)

add_library(ray STATIC ray.cpp ${HEADER_LIST})
set_target_properties(ray PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(ray PUBLIC ../include)
target_link_libraries(ray PUBLIC shape vec)

add_library(accel STATIC accel.cpp ${HEADER_LIST})
set_target_properties(accel PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(accel PUBLIC ../include)
target_link_libraries(accel PUBLIC shape)
target_link_libraries(accel PRIVATE vec ray)

add_library(map STATIC map.cpp ${HEADER_LIST})
set_target_properties(map PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(map PUBLIC ../include)
target_link_libraries(map PUBLIC shape cam img accel vec tex)
target_link_libraries(map PRIVATE util ray aa)

add_library(img STATIC img.cpp ${HEADER_LIST})
set_target_properties(img PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(img PUBLIC ../include)
target_link_libraries(img PUBLIC vec)

add_library(render_bmp STATIC bmp.cpp ${HEADER_LIST})
set_target_properties(render_bmp PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(render_bmp PUBLIC ../include)
target_link_libraries(render_bmp PUBLIC img)

add_library(render_tga STATIC tga.cpp ${HEADER_LIST})
set_target_properties(render_tga PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(render_tga PUBLIC ../include)
target_link_libraries(render_tga PUBLIC img)

add_library(tex STATIC tex.cpp ${HEADER_LIST})
set_target_properties(tex PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(tex PUBLIC ../include)
target_link_libraries(tex PUBLIC vec img)
target_link_libraries(tex PRIVATE render_tga)

add_library(aa STATIC aa.cpp ${HEADER_LIST})
set_target_properties(aa PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(aa PUBLIC ../include)
target_link_libraries(aa PUBLIC vec img)

add_library(cam STATIC cam.cpp ${HEADER_LIST})
set_target_properties(cam PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(cam PUBLIC ../include)
target_link_libraries(cam PUBLIC vec)

add_library(render_gl STATIC gl.cpp ${HEADER_LIST})
set_target_properties(render_gl PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(render_gl PUBLIC ../include "../third_party/imgui/" "../third_party/imgui/misc/cpp/" "../third_party/imgui/backends/" "../third_party/glad/include/")
target_link_libraries(render_gl PUBLIC img map)
target_link_libraries(render_gl PRIVATE render_tga glfw glad imgui accel aa shape)

add_library(test STATIC test.cpp ${HEADER_LIST})
set_target_properties(test PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(test PUBLIC ../include)
target_link_libraries(test PUBLIC shape)
target_link_libraries(test PRIVATE util ray vec)

source_group(
    TREE "${PROJECT_SOURCE_DIR}/include"
    PREFIX "Header Files"
    FILES ${HEADER_LIST}
)
