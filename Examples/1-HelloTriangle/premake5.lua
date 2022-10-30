--premake5.lua

project "1-HelloTriangle"
	language "C++"
	cppdialect "C++17"
	kind "ConsoleApp"
	staticruntime "Off"
	
	targetdir ("%{wks.location}/Binary/" .. outputpath .. "/%{prj.name}")
	objdir ("%{wks.location}/Binary-Intermediate/" .. outputpath .. "/%{prj.name}")
	
	includedirs
	{
		"%{vulkanSDKpath}/Include",
		"%{wks.location}/Dependencies/GLFW/Source/include",
		"%{wks.location}/Dependencies/glm",
		"%{wks.location}/Dependencies/spdlog/include"
	}
	
	files
	{
		"Source/**.cpp",
		"Source/**.h"
	}
	
	libdirs
	{
		"%{vulkanSDKpath}/Lib"
	}
	
	links
	{
		"vulkan-1",
		"GLFW"
	}
	
	filter { "configurations:Debug" }
		defines
		{
			"VKL_DEBUG"
		}
		
		flags
		{
			"MultiProcessorCompile"
		}
		
		symbols "On"
		
	filter { "configurations:Release" }
		flags
		{
			"MultiProcessorCompile"
		}
		
		optimize "On"
		symbols "Off"
		
	filter{}
	