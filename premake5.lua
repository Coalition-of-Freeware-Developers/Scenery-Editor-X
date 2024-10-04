-- Specify the action (Visual Studio 2022)
if _ACTION == "vs2022" then
    -- Use the latest toolset
    toolset "v143"
end

workspace "Scenery Editor X"
    configurations { "Debug", "DEBUG_SHADER", "Release" }
    startproject "opengl-3d-editor"
    architecture "x64"
    location "build"

    flags { "MultiProcessorCompile" }

    filter "configurations:Debug"
        defines { "DEBUG", "DEBUG_SHADER" }
        symbols "On"

    filter "configurations:Release"
        defines { "RELEASE" }
        optimize "Speed"
        flags { "LinkTimeOptimization" }

-- Main Scenery Editor X project setup
project "opengl-3d-editor"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++17"
    architecture "x86_64"
    targetdir "bin/%{cfg.buildcfg}"

    -- Source files
    files { 
        "src/**.cpp",
        "src/**.h",
        "src/**.xaml"
    }

    -- Include directories
    includedirs {
        "src",
        "lib/glfw/include",
        "lib/glad/include",
        "lib/glm",
        "lib/soil/include",
        "lib/freetype/include",
    }

    -- Libraries
    libdirs {
        "lib/glfw/lib",
        "lib/soil/lib",
        "lib/freetype/lib",
    }

    links {
        "glfw3",
        "glad",
        "SOIL",
        "opengl32",
        "freetype",
    }

    -- Preprocessor definitions
    defines {
        "GLFW_INCLUDE_NONE",
        "UNICODE",
        "_UNICODE",
        "WIN32",
        "WINRT",
        "_WINRT_DLL"
    }

    -- Flags and settings
    flags { "MultiProcessorCompile" }
    characterset ("Unicode")

    -- Windows SDK and Toolset
    systemversion "latest"
    toolset "v143"

    -- Enable C++/WinRT support
    -- nuget {
    --     "Microsoft.Windows.CppWinRT:2.0.200316.3"
    -- }

    -- Custom build steps for XAML files
    buildcommands {
        -- Add commands to compile XAML files if necessary
    }

    -- Post-build commands
    postbuildcommands {
        -- Copy necessary DLLs or files
    }

    -- Filters for configurations
    filter "configurations:Debug"
        symbols "On"
        defines { "DEBUG" }
        optimize "Off"
    filter "configurations:Release"
        optimize "On"
        defines { "NDEBUG" }
        symbols "Off"
    filter {}

-- GLAD library setup
project "glad"
    kind "StaticLib"
    language "C"
    targetdir "bin/%{cfg.buildcfg}"

    files { "lib/glad/src/glad.c" }
    includedirs { "lib/glad/include" }

    links { "GLFW", "GLM", "GLAD", "ImGui" }

    filter "system:linux"
        links { "dl", "pthread" }

        defines { "_X11" }

    filter "system:windows"
        defines { "_WINDOWS" }

    include "lib/glfw.lua"
    include "lib/glad.lua"
    include "lib/glm.lua"
    include "lib/imgui.lua"