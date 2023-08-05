@echo off

IF [%VCPKG_ROOT%]==[] (
  cmake .. -B "..\build_vs2022" -G"Visual Studio 16" -A x64
) ELSE (
  cmake .. -B "..\build_vs2022" -G"Visual Studio 16" -A x64 -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
)
