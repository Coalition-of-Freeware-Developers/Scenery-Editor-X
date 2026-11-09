## Summary

<!-- What changes and why, in a few sentences. Pull requests target `dev`, not `master`. -->

## Issues

<!-- `master` is the default branch, so "Closes #N" in a pull request into dev doesn't close anything: list the issues
     here and close them when this merges. -->

- Fixes #

## Changes

-

## Testing

<!-- What you ran, and what you couldn't (the editor needs a GPU; Windows-only code needs a Windows host). -->

- [ ] Catch2 suites that cover the change (AGENTS.md):
- [ ] Ran the editor:
- [ ] Windows
- [ ] Linux
- [ ] macOS

## Checklist

- [ ] Builds on Windows, Linux and macOS (no Win32-only APIs or MSVC-only flags outside per-OS code)
- [ ] Version bumped in `source/resource.h` (app changes only):
- [ ] No change to the files the editor writes, or the change is described above (.edX / .sex `FORMAT_VERSION`, apt.dat, DSF)
- [ ] User manual (`docs/manual`) updated for user-visible changes
- [ ] Screenshots before / after for visual changes
