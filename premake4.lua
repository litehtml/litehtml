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

        if not os.is("windows") then
            buildoptions { "-std=c++11 -Wno-error=unused-variable -Wno-error=unused-parameter" }
        end

        configuration "Debug"
            targetdir   "bin/debug"


        configuration "Release"
            targetdir   "bin/release"

