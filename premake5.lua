workspace "Misanthrope"
	configurations {"Debug", "Release"}
	location "build"

project "Misanthrope"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	targetname "Misanthrope"
	architecture "x86_64"

	-- gcc makefile have cwd at binary, msvc have cwd at project for some reason
	-- this is for loading resource at the right path
	location "build/Misanthrope"

	-- libs with stand-alone tranlations unit
	includedirs {"extern/GLFW/include/", "extern/backward/include/", "extern/imgui/include/", "extern/meshoptimizer/include/", "extern/spirv_reflect/include/"}
	-- header only libs
	includedirs {"extern/", "src/", "extern/tracy/public/tracy"}
	files {"src/Misanthrope.cpp"}
	files {"extern/tracy/public/TracyClient.cpp"}

	defines {"TRACY_ENABLE", "TRACY_VK_USE_SYMBOL_TABLE", "ENABLE_OPTIMIZE_MESH"}
	libdirs {"lib", "build/meshoptimizer/bin", "build/GLFW/bin", "build/imgui/bin", "build/spirv_reflect/bin"}
	links {"meshoptimizer"}
	links {"GLFW"}
	links {"imgui"}
	links {"spirv_reflect"}

	warnings "Default"

	filter "system:windows"
		toolset "msc"
		libdirs {"C:/VulkanSDK/1.3.283.0/Lib"}
		includedirs {"C:/VulkanSDK/1.3.283.0/Include"}
		links {"vulkan-1"}
		-- for tracy
		defines {"_WIN32_WINNT=0x0602", "WINVER=0x0602"}
		links {"ws2_32", "imagehlp"}
	filter {}

	filter "system:linux"
		-- local vulanLib = os.findlib("vulkan")
		libdirs {"/usr/local/bin/1.3.296.0/x86_64/lib"}
		-- includedirs {"/usr/local/bin/1.3.296.0/x86_64/include/"}
		-- for backward stacktrace
		links {"vulkan", "dw"}
	filter {}

	-- buildoptions {"-std=c++17"}
	-- linkoptions {"-std=c++17"}

	filter "configurations:Release"
		defines {"NDEBUG"}
		optimize "On"
		targetdir "bin/release"
	filter {}
	
	filter "configurations:Debug"
		defines {"DEBUG"}
		symbols "On"
		targetdir "bin/debug"
	filter {}

-----------------------------------------------------------------------------------------------

project "meshOptimizer"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	architecture "x86_64"
	filter "system:windows"
		toolset "msc"
	filter {}

	location "build/meshoptimizer"
	targetdir "build/meshoptimizer/bin"
	targetname "meshoptimizer"

	includedirs {"extern/meshoptimizer/include"}
	files {"extern/meshoptimizer/src/**.cpp"}
	removefiles {"extern/meshoptimizer/src/nanite.cpp", "extern/meshoptimizer/src/tests.cpp",  "extern/meshoptimizer/src/main.cpp",  "extern/meshoptimizer/src/ansi.c"}
	-- defines {}
	optimize "On"

-----------------------------------------------------------------------------------------------

project "GLFW"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	architecture "x86_64"

	location "build/GLFW"
	targetdir "build/GLFW/bin"
	targetname "GLFW"

	includedirs {"extern/GLFW/include"}
	files {"extern/GLFW/src/init.c", "extern/GLFW/src/context.c", "extern/GLFW/src/input.c", "extern/GLFW/src/vulkan.c", "extern/GLFW/src/window.c", "extern/GLFW/src/platform.c", "extern/GLFW/src/monitor.c",
								"extern/GLFW/src/null_init.c", "extern/GLFW/src/null_joystick.c", "extern/GLFW/src/null_monitor.c", "extern/GLFW/src/null_window.c"}

	filter "system:linux"
		files {"extern/GLFW/src/x11/*.c"}
		includedirs {"extern/GLFW/src/x11", "extern/GLFW/src"}
		defines {"_GLFW_X11"}
	filter {}

	filter "system:Windows"
		files {"extern/GLFW/src/win/*.c"}
		includedirs {"extern/GLFW/src/win"}
		defines {"_GLFW_WIN32"}
	filter {}

	optimize "On"

-----------------------------------------------------------------------------------------------

project "imgui"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	architecture "x86_64"
	filter "system:windows"
		toolset "msc"
	filter {}

	location "build/imgui"
	targetdir "build/imgui/bin"
	targetname "imgui"

	includedirs {"extern/imgui/include"}
	files {"extern/imgui/src/**.cpp"}
	-- defines {}
	optimize "On"

-----------------------------------------------------------------------------------------------

project "spirv_reflect"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	architecture "x86_64"
	filter "system:windows"
		toolset "msc"
	filter {}

	location "build/spirv_reflect"
	targetdir "build/spirv_reflect/bin"
	targetname "spirv_reflect"

	includedirs {"extern/spirv_reflect/include", "extern"}
	files {"extern/spirv_reflect/src/**.cpp", "extern/spirv_reflect/src/**.c"}
	-- defines {}
	optimize "On"

-----------------------------------------------------------------------------------------------
newaction {
	trigger = "clean",
	description = "clean object files",
	execute = function ()
		os.execute("./clean.ps1")
	end
}

newoption {
	trigger = "cc",
	value = "compiler",
	description = "Choose compiler to compile code",
	allowed = {
		{"gcc", "GCC"},
		{"clang", "CLANG"},
		{"msc", "MSVC"}
	},
	default = "gcc"
}
