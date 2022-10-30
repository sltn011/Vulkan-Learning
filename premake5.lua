--premake5.lua

workspace "Vulkan-Learning"

	architecture "x86_64"
	system "Windows"

	configurations 
	{
		"Debug",
		"Release" 
	}
	
	startproject "1-HelloTriangle"
	
	outputpath = "%{cfg.buildcfg}-%{cfg.architecture}"
	
	vulkanSDKpath = os.getenv("VULKAN_SDK")
	if vulkanSDKpath == nil then
		error("No VULKAN_SDK found in PATH! You must have VulkanSDK installed for code to compile and run!")
	end
	
	include "Examples"
	
group "Dependencies"
	include "Dependencies/GLFW"
group ""
	