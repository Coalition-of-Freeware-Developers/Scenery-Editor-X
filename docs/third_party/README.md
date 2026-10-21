# Third-party code ported into Scenery Editor X

Code that SEDX carries from other projects, with the license each comes under. Every ported file keeps its original
copyright and license notice in its header; this page collects them so the licenses can be checked in one place (plan
`plan/feature-xp12-environment-rendering-1.md`, LEG-002: MIT, BSD, Apache-2.0 or public domain only, no GPL).

| Code in SEDX | Origin | License | Full text |
|---|---|---|---|
| `source/SceneryEditorX/renderer/bend_sss_cpu.h` (screen-space shadow dispatch list) | Bend Studio, "Screen Space Shadows" sample, Copyright 2023 Sony Interactive Entertainment | Apache License 2.0 | [LICENSE-Apache-2.0.txt](LICENSE-Apache-2.0.txt) |
| `resources/shaders/fsr1_easu.slang`, `fsr1_rcas.slang` | AMD FidelityFX Super Resolution 1.0 (`ffx_fsr1.h`), Copyright (c) 2021 Advanced Micro Devices, Inc. | MIT | in each file's header |
| `resources/shaders/cacao.slang`, `source/SceneryEditorX/renderer/cacao.cpp` (ambient occlusion) | AMD FidelityFX CACAO 1.4 (FidelityFX SDK 1.1.4: `ffx_cacao_*.h`, `ffx_cacao.cpp`), Copyright (C) 2024 Advanced Micro Devices, Inc. | MIT | [LICENSE-FidelityFX-MIT.txt](LICENSE-FidelityFX-MIT.txt), and in each file's header |

## Notes

- Bend Studio's screen-space shadows: only the CPU dispatch-list header is ported. Its GPU half (`bend_sss_gpu.h`) was
  never ported, so `sss_blend.slang` stays disabled and the `sss` renderer feature is off by default: X-Plane 12.4 has
  no screen-space shadow pass, and SEDX's sun shadows are the cascaded shadow maps (plan Phase 7). The Apache-2.0 notice
  in `bend_sss_cpu.h` is the one the sample ships with; nothing in SEDX's own code is relicensed by it.
- X-Plane 12's shaders, binaries and art are not third-party code in SEDX: they are a behavioural reference only and are
  never copied (plan LEG-001).
- AMD FidelityFX CACAO: the shaders' FP32 paths of the medium, high and highest qualities (the ones X-Plane 12's presets
  use) and the host code that fills their constants and plans their dispatches; the lowest / low kernels and the FP16
  paths were not ported. SEDX's changes (reverse-Z depth, world normals, X-Plane's fade-in, bounds checks) are listed in
  the header of `cacao.slang`.
