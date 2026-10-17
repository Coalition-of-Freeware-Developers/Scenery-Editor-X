vulkan_sample_project — Minimal sample

This sample demonstrates a minimal headless Vulkan instance creation with a Debug build option that enables validation layers.

Prerequisites
- Vulkan SDK installed and `VULKAN_SDK` environment variable set (Windows). Alternatively, ensure system provides Vulkan headers/libs.
- CMake >= 3.16 and a suitable MSVC or clang-cl toolchain on Windows.
- Optional: RenderDoc installed for frame capture. If installed, ensure `renderdoccmd.exe` is on PATH.

Build and run (PowerShell):

```powershell
# Prepare and build
.\build_and_capture.ps1 -Build

# Run with validation layers and optional RenderDoc capture
.\build_and_capture.ps1 -Capture
```

Notes
- The sample `CMakeLists.txt` will attempt `find_package(Vulkan)` — on Windows with LunarG SDK, CMake usually picks up the SDK via `VULKAN_SDK`.
- To run RenderDoc captures, install RenderDoc and use the `-Capture` flag in the helper script. If `renderdoccmd.exe` is not available, launch RenderDoc GUI and capture manually.

Shader build
- Compile GLSL with `glslangValidator -V shaders/triangle.vert.glsl -o shaders/triangle.vert.spv` and similarly for fragment shader.
