workspace "Exporter"
	configurations {"Debug", "Release"}
	location "build"

	filter "configurations:Release"
		defines {"NDEBUG"}
		optimize "On"
	filter {}
	
	filter "configurations:Debug"
		defines {"DEBUG"}
		symbols "On"
	filter {}

	filter "system:windows"
		defines {"NODEFAULTLIB"}
	filter {}

project "Exporter"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	targetname "Exporter"
	architecture "x86_64"

	filter "configurations:Release"
		targetdir "bin/release"
	filter {}
	
	filter "configurations:Debug"
		filter "system:linux"
			targetdir "bin/linux/debug"
		filter {}

		filter "system:window"
			targetdir "bin/window/debug"
		filter {}
	filter {}

	filter "configurations:Release"
		filter "system:linux"
			targetdir "bin/linux/release"
		filter {}

		filter "system:window"
			targetdir "bin/window/release"
		filter {}
	filter {}

	location "build"

	-- header only libs
	includedirs {"../extern", "include", "../extern/backward/include"}
	files {"src/*.cpp", "../extern/backward/src/backward.cpp"}

	filter "system:linux"
		-- for backward stacktrace
		links {"vulkan", "dw"}
	filter {}

	warnings "Default"

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

