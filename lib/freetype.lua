project "freetype"
    kind "StaticLib"
    language "C"
    architecture "x86_64"

    targetdir "../bin/%{cfg.buildcfg}"
	objdir "../obj/%{cfg.buildcfg}"
    
    includedirs { "freetype/include/" }

	files { "freetype/src/glad.c" }
    
    filter "system:windows"
        systemversion "latest"
        staticruntime "On"

        defines 
        { 
            "_GLAD_WIN32",
            "_CRT_SECURE_NO_WARNINGS"
        }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"