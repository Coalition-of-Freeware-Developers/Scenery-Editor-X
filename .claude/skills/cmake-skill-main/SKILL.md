---
name: cmake-skill
description: |
  Agent-native CMake lifecycle automation with persistent project health monitoring. Use when Claude needs to:
    1. Configure, build, or test CMake projects,
    2. Capture and parse multi-line configuration errors or call stacks,
    3. Maintain project health visibility via a persistent dashboard.
---

# CMake Build & Diagnostics

Manage CMake projects with persistent situational awareness.

## Resources

- **Error parsing**: `references/protocol.md` for hierarchical error parsing and state management logic.
- **Dashboard**: Review `cmake_report.md` in build directory for health summary.
- **Script**: `scripts/cmake-skill` for the full pipeline.

## Workflow

1. **Orchestrate**: Run `scripts/cmake-skill pipeline` for full verification.
2. **Monitor**: Review `build/<preset>/cmake_reports/cmake_report.md` for health summary.
3. **Extract**: Parse `build/<preset>/cmake_reports/cmake_report.json` for structured diagnostic data.
4. **Customize**: Adjust `.cmake-format.py` to add specs for project-specific commands.
   **Build fails?** → Read `references/protocol.md` for hierarchical error parsing and state management.

## Execution

To run this skill manually, locate the script in the skill directory (for example, `~/.agents/skills/cmake-skill/scripts/cmake-skill`)
and execute it from the project root directory.

Example:
  ~/.agents/skills/cmake-skill/scripts/cmake-skill pipeline

Note: The `~/` in the example will be expanded to your home directory by the shell.

**Why:** Python environments managed by uv (PEP 668 externally-managed) block `pip install`. Running with `python3` directly will fail when auto-installing jinja2. `uv run --script` handles PEP 723 inline dependencies correctly.

**Note:** This skill is designed to be run from the project root directory. The skill tool handles the path resolution when invoked via `@skill`.
