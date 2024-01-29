workspace "rt"
    language "C++"
    cppdialect "C++17"
    platforms { "x64" }
    configurations { "debug", "release" }
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
    links "shape"
    links "map"
    links "img"
    links "bmp"

project "map"
    kind "StaticLib"
    location "map"
    files { "map/**.cpp", "map/**.hpp" }

project "shape"
    kind "StaticLib"
    location "shape"
    files { "shape/**.cpp", "shape/**.hpp" }

project "vec"
    kind "StaticLib"
    location "vec"
    files { "vec/**.cpp", "vec/**.hpp" }

project "bmp"
    kind "StaticLib"
    location "bmp"
    files { "bmp/**.cpp", "bmp/**.hpp" }

project "img"
    kind "StaticLib"
    location "img"
    files { "img/**.cpp", "img/**.hpp" }
