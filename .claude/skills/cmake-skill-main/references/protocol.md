# CMake Skill Protocol

Follow this protocol for robust build and test automation:

## 1. Pipeline Execution

- **Recommended**: Use \`cmake-skill pipeline\` to ensure all phases (config,
  build, test) are synchronized.
- **Incremental**: Use \`configure\`, \`build\`, or \`test\` for targeted
  actions.

## 2. Diagnostics & Dashboard

- **Human Review**: Check \`cmake_report.md\` in the project root for a visual
  status summary.
- **Machine Reasoning**: Parse \`.lint/cmake_report.json\` for precise error
  extraction.

## 3. Surgical Triage with jq

\`\`\`bash
Show failed phases
jq '. | to_entries[] | select(.value.success == false) | .key' <machine_report>

Get the first error message of a failed phase
jq '.build.errors[0].message' <machine_report>
\`\`\`

## 4. Maintenance

- **Clean Build**: Use \`--clean\` to reset the dashboard and build artifacts.
- **Parallelism**: Use \`--jobs\` or \`-j\` to control concurrency (default: CPU
  count).
