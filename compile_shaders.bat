@echo off

set GLSLC=C:\VulkanSDK\1.3.268.0\Bin\glslc.exe

for %%f in (shaders\*) do (
	 GLSLC %%f -o compiled\%%~nxf.spv
)