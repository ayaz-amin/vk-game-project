@echo off

mkdir debug
pushd debug

set SRC=../src/unity.cc
set VKINC=C:\VulkanSDK\1.3.268.0\Include
set VKLIB=C:\VulkanSDK\1.3.268.0\Lib\vulkan-1.lib

cl -O2 -I%VKINC% %SRC% %VKLIB% user32.lib gdi32.lib kernel32.lib /link /SUBSYSTEM:CONSOLE /OUT:main.exe

popd
