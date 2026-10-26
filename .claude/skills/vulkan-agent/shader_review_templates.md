Shader Review Templates & Checklist

Use these templates to submit shaders for review or to guide the agent when asking for a review.

1) Minimal submission template

```
LANGUAGE: GLSL | HLSL | SLANG
SHADER_STAGE: vertex | fragment | compute | raygen | any
TARGET: Vulkan 1.3 (or 1.2 / 1.4)
COMPILER: glslangValidator / dxc / slangc

-- PASTE SHADER BELOW --
[shader code]

HOST_BINDINGS: (optional) describe descriptor sets and binding indices used by the host
```

2) Review checklist (agent should run/apply)

- Compile with the stated compiler and return errors/warnings.
- Run `spirv-val` on produced SPIR-V and return diagnostics.
- Verify `layout(set=X,binding=Y)` annotations match host-supplied layouts.
- Check for energy-conserving BRDF formulas (for PBR shaders): for microfacet models ensure fresnel, normal distribution, and geometry terms are within expected ranges.
- Identify non-uniform control flow and divergent patterns; suggest workarounds for GPU wave/warp divergence.
- Ensure proper use of `coherent`/`volatile`/`restrict` qualifiers only when necessary.
- Suggest using `precision` qualifiers for targets where applicable.

3) Suggested commands (examples)

```bash
# GLSL -> SPIR-V
glslangValidator -V -S frag -o out.frag.spv shader.frag

# Validate SPIR-V
spirv-val out.frag.spv

# HLSL -> SPIR-V with dxc
dxc -T ps_6_0 -E main -spirv -Fo out.frag.spv shader.hlsl

# Slang use (example)
slangc -O --target spirv -o out.spriv shader.slang
```

4) Performance-focused review points

- Texture sampling count: avoid unbounded texture lookups in loops.
- Derivative usage: minimize `dFdx`/`dFdy` where possible.
- Branching: prefer arithmetic blends over heavy branching inside hot loops.
- Memory layout: favor tightly packed UBO/SSBO layouts and explicit offsets for portability.

5) Example request to the agent

"Review the following fragment shader (GLSL). Compile with `glslangValidator -V`, run `spirv-val`, and list any binding mismatches against this host layout: set 0 binding 0 = sampler2D albedo; set 1 binding 0 = uniform BufferGlobals." 
