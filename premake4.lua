#!lua

-- Additional Linux libs: "X11", "Xxf86vm", "Xi", "Xrandr", "stdc++"

includeDirList = {
    "./include",
    "./gl3w",
    "./imgui",
    "./irrKlang/include",
    "."
}

libDirectories = {
    "./lib",
    "."
}

PLATFORM = os.get()

if PLATFORM == "macosx" then
    linkLibs = {
        "framework",
        "imgui",
        "glfw3",
        "lua"
    }
end

if not os.isfile("lib/libglfw3.a") then
    os.chdir("glfw-3.1.1")
    os.mkdir("build")
    os.chdir("build")
    os.execute("cmake ../")
    os.execute("make")
    os.chdir("../../")
    os.mkdir("lib")
    os.execute("cp glfw-3.1.1/build/src/libglfw3.a lib/")
end

if not os.isfile("lib/liblua.a") then
    os.chdir("lua-5.3.1")

    if PLATFORM == "macosx" then
        os.execute("make macosx")
    elseif PLATFORM == "linux" then
        os.execute("make linux")
    elseif PLATFORM == "windows" then
        os.execute("make mingw")
    end

    os.chdir("../")
    os.execute("cp lua-5.3.1/src/liblua.a lib/")
end

if PLATFORM == "linux" then
    linkLibs = {
        "framework",
        "imgui",
        "glfw3",
	"IrrKlang",
        "lua",
        "GL",
        "Xinerama",
        "Xcursor",
        "Xxf86vm",
        "Xi",
        "Xrandr",
        "X11",
        "stdc++",
        "dl",
        "pthread"
    }
end

-- Build Options:
if PLATFORM == "macosx" then
    linkOptionList = { "-framework IOKit", "-framework Cocoa", "-framework CoreVideo", "-framework OpenGL" }
end

linkOptionList = { "-Wl,-z,origin,-rpath='$$ORIGIN'" }

buildOptions = {"-std=c++11"}

solution "survey-neo"
    configurations { "Debug", "Release" }

    -- base framework from graphics course ui
    project "framework"
        kind "StaticLib"
        language "C++"
        location "build"
        objdir "build"
        targetdir "lib"
        buildoptions (buildOptions)
        includedirs (includeDirList)
        files { "framework/*.cpp" }

    -- Build imgui static library
    project "imgui"
        kind "StaticLib"
        language "C++"
        location "build"
        objdir "build"
        targetdir "lib"
        includedirs (includeDirList)
        includedirs {
            "imgui/examples/opengl3_example",
            "imgui/examples/libs/gl3w/",
        }
        files {
            "imgui/*.cpp",
            "gl3w/GL/gl3w.c"
        }

    -- Build lodepng static library
    project "lodepng"
        kind "StaticLib"
        language "C++"
        location "build"
        objdir "build"
        targetdir "lib"
        includedirs (includeDirList)
        includedirs {
            "lodepng"
        }
        files {
            "lodepng/lodepng.cpp"
        }


    project "Project"
        kind "ConsoleApp"
        language "C++"
        location "build"
        objdir "build"
        targetdir "."
        buildoptions (buildOptions)
        libdirs (libDirectories)
        links (linkLibs)
        linkoptions (linkOptionList)
        includedirs (includeDirList)
        files { "*.cpp" }

    configuration "Debug"
        defines { "DEBUG" }
        flags { "Symbols" }

    configuration "Release"
        defines { "NDEBUG" }
        flags { "Optimize" }
