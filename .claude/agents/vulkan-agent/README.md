Vulkan Agent Skill
==================

Installation
------------

1. Place the `vulkan-agent` folder under your Copilot skills location (e.g. `%USERPROFILE%/.copilot/skills` or the path used by your Copilot installation). The folder already contains `SKILL.md`, `references.md`, and `prompts.md`.
2. Restart VS Code or the Copilot extension host so the new skill is discovered.

Usage
-----

- Use the `Vulkan Graphics Engineer` agent from your Copilot chat UI or prompt system.
- Ask targeted prompts (see `prompts.md`) for code examples, debugging help, or compatibility advice.

IDE Integration Notes
---------------------

- Visual Studio (2022/2026/Insiders): agent includes tips and templates for configuring Debug environment variables and attaching GPU capture tools.
- VS Code: use the included `launch.json`/`tasks.json` snippets in `prompts.md` as starting points for integrating validation layers and RenderDoc captures.

Debugging & Validation
----------------------

- Enable `VK_LAYER_KHRONOS_validation` during development to surface API misuse; use `VK_EXT_debug_utils` to register callbacks and collect structured messages.
- Use RenderDoc and GPU vendor tools to capture frames and inspect resource state, descriptor bindings, and GPU commands.

Support
-------

If you want additional templates, example projects, or integration with your specific engine (CMake project, MSBuild, or premade solution), ask the agent to scaffold them.
