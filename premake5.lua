workspace "HephaestusEngine"
   configurations { "Debug", "Release" }
   startproject "Engine"

   -- Path to the Dependencies directory
   IncludeDir = {}
   IncludeDir["GLFW"] = "Dependencies/GLFW"  -- Base GLFW path
   IncludeDir["glm"] = "Dependencies/glm"
   IncludeDir["Vulkan"] = "Dependencies/VulkanSDK"

project "Engine"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++17"

   targetdir "bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/%{prj.name}"
   objdir "bin-int/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/%{prj.name}"

   files { "%{prj.name}/src/**.h", "%{prj.name}/src/**.cpp", "%{prj.name}/src/**.c" }

   -- Windows specific settings
   filter "system:windows"
      includedirs { IncludeDir["Vulkan"] .. "/Include", IncludeDir["GLFW"] .. "/Windows/include", IncludeDir["glm"] }
      libdirs { IncludeDir["Vulkan"] .. "/Lib", IncludeDir["GLFW"] .. "/Windows/lib-vc2022" }
      architecture "x64"
      systemversion "latest"
      defines { "PLATFORM_WINDOWS" }
      links { "vulkan-1", "glfw3" }

      
   -- TODO MacOS specific settings
   -- filter "system:macosx"
   --    includedirs { IncludeDir["GLFW"] .. "/MacOS/include" }
   --    libdirs { IncludeDir["GLFW"] .. "/MacOS/lib-arm64" }
   --    links { "vulkan-1", "glfw3" }
   --    architecture "arm64" -- or "x64" for Intel, "arm64" for M1 specifically, or "universal"
   --    systemversion "latest"
   --    defines { "PLATFORM_MACOS" }

   -- TODO Linux specific settings

      -- General settings for Debug and Release configurations
   filter "configurations:Debug"
   defines { "DEBUG" }
   symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"

   filter {"system:windows", "configurations:Release"}
      buildoptions "/MD"
   filter {"system:windows", "configurations:Debug"}
      buildoptions "/MDd"