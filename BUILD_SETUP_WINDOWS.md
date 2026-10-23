Title: Windows build setup for Scenery Editor X

Short checklist
- Install Ninja and ensure `ninja.exe` is on your PATH.
- Install a compatible MinGW-w64 toolchain (or use MSVC) and make sure `gcc/g++` are available.
- Ensure Vulkan SDK path is set (e.g. `C:/VulkanSDK/1.4.341.1`).

Recommended install commands
- Install Ninja (winget):

```powershell
winget install ninja --source winget
# or Chocolatey:
choco install ninja
```

- Install MSYS2 + MinGW (recommended for MinGW/GCC):

```powershell
# Follow MSYS2 instructions: https://www.msys2.org/
# After installing, open MSYS2 MinGW64 shell and install toolchain:
pacman -Syu
pacman -S mingw-w64-x86_64-toolchain
```

Configure with CMake (examples)

Single-config Ninja (recommended):

```powershell
cmake -DSEDX_PREFERRED_COMPILER=GCC -DVCPKG_TARGET_TRIPLET=x64-mingw -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/sedx-windows.cmake -G Ninja -S . -B build
```

If you prefer multi-config generators (Visual Studio / Ninja Multi-Config), ensure the generator is available on your system. The error "CMake was unable to find a build program corresponding to 'Ninja Multi-Config'" typically means the selected generator is not installed or not supported. Use `-G Ninja` on MinGW/GCC systems.

vcpkg note (mingw triplets)
- This repo normalizes the triplet `x64-mingw` → `x64-mingw-dynamic` for vcpkg. vcpkg may still refuse to build some ports on MinGW (for example, `skia` is known to be unsupported on mingw). If vcpkg fails with an unsupported-port message you can:
  - Switch to an MSVC triplet (`x64-windows`) and build with MSVC, or
  - Re-run vcpkg with `--allow-unsupported` (may produce broken/unstable binaries), or
  - Remove / replace the offending package from `vcpkg.json`.

Where to go next
- After installing Ninja and the toolchain, re-run the CMake configure command using `-G Ninja` (single-config) or install a matching multi-config ninja if you need that workflow.
- To package a build for testers (MSVC builds), see `installer/README.md`.
