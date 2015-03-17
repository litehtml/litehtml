solution "litehtml"
    configurations { "release", "debug" }
    platforms { "x32", "x64" }
    defines { "LITEHTML_UTF8" }

    targetname "litehtml"
    language "C++"
    kind "StaticLib"

    files
    {
        "src/**.c", "src/**.cpp", "src/**.h"
    }

    configuration "debug"
        defines     { "_DEBUG" }
        flags       { "Symbols" }
        targetsuffix "_d"

    configuration "release"
        defines     { "NDEBUG" }
        flags       { "OptimizeSize" }

    configuration "windows"
        defines     { "WIN32" }

    project "litehtml"

        if not os.is("windows") then
            buildoptions { "-std=c++11 -Wno-error=unused-variable -Wno-error=unused-parameter" }
        end

        configuration "x32"
            targetdir   "bin/i386"

        configuration "x64"
            targetdir   "bin/x64_86"
