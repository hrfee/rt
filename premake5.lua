workspace "rt"
    language "C++"
    cppdialect "C++17"
    platforms { "x64" }
    configurations { "debug", "release" }
includedirs { "third_party/glad/include", "third_party/imgui", "third_party/imgui/backends", "third_party/imgui/misc/cpp" }
    filter "configurations:debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:release"
        defines { "NDEBUG" }
        optimize "On"

project "main"
    kind "ConsoleApp"
    targetdir "bin/${cfg.buildcfg}"
    location "main"
    files { "main/**.hpp", "main/**.cpp", "render_tga/**.cpp" }
    links "map"
    links "shape"
    links "img"
    links "util"
    links "glfw"
    links "render_tga"
    links "render_gl"
    links "cam"
    filter { "system:linux" }
        buildoptions { "-lglfw -lrt -lm -ldl -lwayland-client -lm -pthread -lrt -lffi" }

project "util"
    kind "StaticLib"
    location "util"
    links "util"
    files { "util/**.cpp", "util/**.hpp" }

project "map"
    kind "StaticLib"
    location "map"
    links "shape"
    links "util"
    files { "map/**.cpp", "map/**.hpp" }

project "shape"
    kind "StaticLib"
    location "shape"
    links "util"
    files { "shape/**.cpp", "shape/**.hpp" }

project "vec"
    kind "StaticLib"
    location "vec"
    files { "vec/**.cpp", "vec/**.hpp" }

project "img"
    kind "StaticLib"
    location "img"
    files { "img/**.cpp", "img/**.hpp" }

project "render_bmp"
    kind "StaticLib"
    location "render_bmp"
    files { "render_bmp/**.cpp", "render_bmp/**.hpp" }

project "render_tga"
    kind "StaticLib"
    location "render_tga"
    files { "render_tga/**.cpp", "render_tga/**.hpp" }

project "cam"
    kind "StaticLib"
    location "cam"
    files { "cam/**.cpp", "cam/**.hpp" }

project "render_gl"
    kind "StaticLib"
    location "render_gl"
    links "glfw"
    links "render_bmp"
    files { "render_gl/**.cpp", "render_gl/**.hpp",
            "third_party/glad/src/*.c",
            "third_party/imgui/*.cpp", "third_party/imgui/*.h",
            "third_party/imgui/backends/imgui_impl_glfw.cpp", "third_party/imgui/backends/imgui_impl_glfw.h",
            "third_party/imgui/backends/imgui_impl_opengl3.cpp", "third_party/imgui/backends/imgui_impl_opengl3.h",
            "third_party/imgui/misc/cpp/imgui_stdlib.cpp", "third_party/imgui/misc/cpp/imgui_stdlib.h"
    }
    filter { "system:linux" }
        buildoptions { "-lglfw -lrt -lm -ldl -lwayland-client -pthread -lrt -lffi" }

