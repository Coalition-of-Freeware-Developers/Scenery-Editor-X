Vulkan Agent — Example Prompts and Templates

This file contains ready-to-use prompts, editor/debugging snippets, and templates to ask the `Vulkan Graphics Engineer` agent.

1) Quick usage templates

- Design / Architecture

  "Design a bindless texture system in Vulkan supporting 1M textures. Provide descriptor set/layout design, update strategy, shader access patterns, and migration notes for Vulkan 1.2 → 1.4."

- Code snippet request

  "Show a minimal C++ example using VK_KHR_dynamic_rendering to render to an offscreen image, target Vulkan 1.3. Include required extensions and a small `VkRenderPass`-less render call sequence."

- Validation/Debugging

  "I get a validation error: '<paste exact callback text here>'. Explain the likely causes, the code locations to check, and a minimal repro test to isolate the problem."

2) Editor / IDE templates

- VS Code `launch.json` snippet (MSVC / CMake Tools):

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Launch Vulkan App (MSVC)",
      "type": "cppvsdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/bin/my_vulkan_app.exe",
      "args": [],
      "cwd": "${workspaceFolder}",
      "environment": [
        { "name": "VK_INSTANCE_LAYERS", "value": "VK_LAYER_KHRONOS_validation" }
      ],
      "preLaunchTask": "build"
    }
  ]
}
```

- VS Code `tasks.json` build task (CMake):

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "build",
      "type": "shell",
      "command": "cmake --build build --config Debug",
      "problemMatcher": ["$msCompile"]
    }
  ]
}
```

3) Enabling validation layers (Windows PowerShell)

```powershell
# Temporarily enable the standard validation layer for this process
$env:VK_INSTANCE_LAYERS = "VK_LAYER_KHRONOS_validation"

# Increase loader verbosity for diagnostics
$env:VK_LOADER_DEBUG = "all"

# Run your app in the same shell
.\build\bin\my_vulkan_app.exe
```

4) Common debugging workflow (suggested)

- Enable `VK_LAYER_KHRONOS_validation` and `VK_EXT_debug_utils` callback to log messages.
- Reproduce the failing frame and capture it with RenderDoc for pixel/command inspection.
- Use `spirv-val` to validate compiled SPIR-V for the shader stages referenced by the failing draw.
- If it's a synchronization issue, reproduce with small workloads and use validation messages to trace which resource or barrier ordering is missing.

5) RenderDoc quick capture (CLI guidance)

Open RenderDoc GUI and capture, or use the programmatic capture method described by RenderDoc documentation. The agent can provide exact steps based on OS and RenderDoc version if asked.

6) VS / Visual Studio integration hints

- Use MSVC configuration (Solution / Project) to set environment variable `VK_INSTANCE_LAYERS` for the Debug configuration.
- Use Visual Studio Diagnostic Tools and attach RenderDoc/Nsight when capturing GPU frames.

7) Prompt templates for version compatibility

"List APIs that were promoted from extensions to core between Vulkan 1.2 and 1.4 related to descriptor indexing and dynamic rendering. For each API, describe migration steps and recommended feature checks at runtime."

---
If you want additional templates (e.g., shader review templates, repro-case skeletons, or CMake example project), ask and I will add them.

8) Shader review & compiler templates

- "ReviewShader: LANGUAGE=GLSL, STAGE=fragment, TARGET=Vulkan1.3, COMPILER=glslangValidator\n<insert shader>"
- "CompileShader: Explain how to build this HLSL shader with `dxc` to SPIR-V and validate with `spirv-val`."

9) Sample project / capture templates

- "Scaffold sample: create the minimal CMake sample project that enables validation layers in Debug and provides a `build_and_capture.ps1` script to run RenderDoc captures on Windows."
- "Explain how to use RenderDoc CLI to capture a single frame of a running process and how to open the resulting `.rdc` file to inspect descriptor bindings and draw calls."
