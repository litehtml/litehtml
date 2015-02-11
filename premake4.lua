solution "litehtml"
    configurations { "Release", "Debug" }

    targetname "litehtml"
    language "C++"
    kind "StaticLib"

    files
    {
        "src/**.cpp", "src/**.h"
    }

    configuration "Debug"
        defines     { "_DEBUG" }
        flags       { "Symbols" }

    configuration "Release"
        defines     { "NDEBUG" }
        flags       { "OptimizeSize" }

    configuration "windows"
        defines     { "WIN32" }

    project "ShaderShaker"

    configuration "Debug"
        targetdir   "bin/debug"

    configuration "Release"
        targetdir   "bin/release"