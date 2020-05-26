PROJECT_NAME = "Nai"

OUTPUT_BASE_PATH = '%{cfg.buildtarget.directory}'

-- Workspace (Solution)
workspace (PROJECT_NAME)
    configurations { "Debug", "Release", "DebugClang", "ReleaseClang" }
    platforms { "x64" }
    location "build"
    filename (ENGINE_NAME)
    startproject (PROJECT_NAME)
    cppdialect "C++17"

    filter "platforms:x64"
        system "Windows"
        architecture "x64"

project (PROJECT_NAME)
    kind "ConsoleApp"
    language "C++"  
    location "build"
    filename (PROJECT_NAME)
    uuid "3F3B938C-ABC6-0051-B4D7-834520E25C54"
    objdir "build/obj"
    warnings "Extra"
    floatingpoint "Fast"
    entrypoint "mainCRTStartup"
    includedirs { "source", "dep/tracy" }
    pchheader "pch/Build.h"
    pchsource "source/pch/Build.cpp"

    postbuildmessage "Removing '%{OUTPUT_BASE_PATH}\\testresults' to fix unittests"
    postbuildcommands { "rmdir /Q /S %{OUTPUT_BASE_PATH}\\testresults" }

    files { "source/**.h", "source/**.hpp", "source/**.cpp", "dep/tracy/TracyClient.cpp" }
    flags { "MultiProcessorCompile" }
    
    filter "configurations:Debug"
        defines { "NAI_DEBUG=1", "NAI_RELEASE=0", "NAI_CLANG=0" }
        flags { "FatalWarnings" }
        symbols "On"
        targetsuffix ("_Debug")
        targetdir "bin/Debug"

    filter "configurations:Release"
        defines { "NAI_DEBUG=0", "NAI_RELEASE=1", "NAI_CLANG=0" }
        flags { "FatalWarnings" }
        targetsuffix ("_Release")
        symbols "Off"
        optimize "On"
        targetdir "bin/Release"

    filter "configurations:DebugClang"
        defines { "NAI_DEBUG=1", "NAI_RELEASE=0", "NAI_CLANG=1" }
        flags { "FatalWarnings" }
        symbols "On"
        targetsuffix ("_DebugClang")
        toolset "msc-ClangCL"
        targetdir "bin/DebugClang"

    filter "configurations:ReleaseClang"
        defines { "NAI_DEBUG=0", "NAI_RELEASE=1", "NAI_CLANG=1" }
        flags { "FatalWarnings" }
        targetsuffix ("_ReleaseClang")
        symbols "Off"
        optimize "On"
        toolset "msc-ClangCL"
        targetdir "bin/ReleaseClang"

dofile("samples/unittests/premake5.lua")