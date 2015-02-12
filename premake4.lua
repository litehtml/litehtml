solution "litehtml"
    configurations { "Release", "Debug" }
    defines { "LITEHTML_UTF8" }

    targetname "litehtml"
    language "C++"
    kind "StaticLib"

    files
    {
        "src/**.c", "src/**.cpp", "src/**.h"
    }

    configuration "Debug"
        defines     { "_DEBUG" }
        flags       { "Symbols" }
        targetsuffix "_d"

    configuration "Release"
        defines     { "NDEBUG" }
        flags       { "OptimizeSize" }

    configuration "windows"
        defines     { "WIN32" }

    project "litehtml"

    configuration "Debug"
        targetdir   "bin/debug"


    configuration "Release"
        targetdir   "bin/release"