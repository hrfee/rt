workspace "rt"
    language "C++"
    cppdialect "C++17"
    platforms { "x64" }
    configurations { "debug", "release" }
includedirs { "third_party/glad/include" }
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
    files { "main/**.hpp", "main/**.cpp" }
    links "map"
    links "shape"
    links "img"
    links "render_bmp"
    links "glfw"
    links "render_gl"
    links "cam"
    filter { "system:linux" }
        buildoptions { "-lglfw -lrt -lm -ldl -lwayland-client -lm -pthread -lrt -lffi" }

project "map"
    kind "StaticLib"
    location "map"
    links "shape"
    files { "map/**.cpp", "map/**.hpp" }

project "shape"
    kind "StaticLib"
    location "shape"
    files { "shape/**.cpp", "shape/**.hpp" }

project "vec"
    kind "StaticLib"
    location "vec"
    files { "vec/**.cpp", "vec/**.hpp" }

project "img"
    kind "StaticLib"
    location "img"
    files { "img/**.cpp", "img/**.hpp" }

project "cam"
    kind "StaticLib"
    location "cam"
    files { "cam/**.cpp", "cam/**.hpp" }

project "render_bmp"
    kind "StaticLib"
    location "render_bmp"
    files { "render_bmp/**.cpp", "render_bmp/**.hpp" }

project "render_gl"
    kind "StaticLib"
    location "render_gl"
    files { "render_gl/**.cpp", "render_gl/**.hpp", "third_party/glad/src/*.c" }
    links "glfw"
    filter { "system:linux" }
        buildoptions { "-lglfw -lrt -lm -ldl -lwayland-client -lm -pthread -lrt -lffi" }

