if os.ishost "windows" then

    -- Windows
    newaction
    {
        trigger     = "build",
        description = "Build the litehtml library",
        execute = function ()
--             os.rmdir "_build32"
--             os.execute "mkdir _build32 & pushd _build32 \z
-- && cmake -G \"Visual Studio 15 2017\" ..\\ \z
-- && popd \z
-- && cmake --build _build32 --config Debug"
--             os.rmdir "_build64"
--             os.execute "mkdir _build & pushd _build \z
-- && cmake -G \"Visual Studio 15 2017 Win64\" ..\\ \z
-- && popd \z
-- && cmake --build _build --config Debug"
             os.rmdir "_build"
             os.execute "mkdir _build & pushd _build \z
&& cmake ..\\ \z
&& popd \z
&& cmake --build _build --config Debug"
        end
    }

    newaction
    {
        trigger     = "test",
        description = "Build and run all unit tests",
        execute = function ()
            os.execute "premake5 build"
            os.execute "pushd _build \z
&& ctest -C Debug \z
&& popd"
        end
    }

else

     -- MacOSX and Linux. 
    newaction
    {
        trigger     = "build",
        description = "Build the litehtml library",
        execute = function ()
            os.rmdir "_build"
            os.execute "mkdir _build && cd _build \z
&& cmake ../ \z
&& cd .. \z
&& cmake --build _build --config Debug"
        end
    }

    newaction
    {
        trigger     = "test",
        description = "Build and run all unit tests",
        execute = function ()
            os.execute "premake5 build"
            os.execute "cd _build \z
&& ctest -C Debug \z
&& cd .."
        end
    }

    newaction
    {
        trigger     = "loc",
        description = "Count lines of code",
        execute = function ()
            os.execute "wc -l *.cs"
        end
    }
end

newaction
{
    trigger     = "clean",
    description = "Clean all build files and output",
    execute = function ()
        files_to_delete = 
        {
            "*.make",
            "*.zip",
            "*.tar.gz",
            "*.db",
            "*.opendb"
        }
        directories_to_delete = 
        {
            "_build",
            "_build32",
            "_build64",
            "Testing",
            "obj",
            "ipch",
            "bin",
            "nupkgs",
            ".vs",
            ".vscode",
            "Debug",
            "Release",
            "release"
        }
        for i,v in ipairs( directories_to_delete ) do
          os.rmdir( v )
        end
        if not os.ishost "windows" then
            os.execute "find . -name .DS_Store -delete"
            for i,v in ipairs( files_to_delete ) do
              os.execute( "rm -f " .. v )
            end
        else
            for i,v in ipairs( files_to_delete ) do
              os.execute( "del /F /Q  " .. v )
            end
        end
    end
}
