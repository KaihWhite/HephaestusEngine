@echo off
setlocal
set "VULKAN_DIR=%~dp0..\..\Dependencies\VulkanSDK\Bin"
set "ROOT_DIR=%~dp0"
cd /d "%ROOT_DIR%"
@echo on

"%VULKAN_DIR%\glslc.exe" "%ROOT_DIR%\shader.vert" -o vert.spv
"%VULKAN_DIR%\glslc.exe" "%ROOT_DIR%\shader.frag" -o frag.spv
pause