---
name: gltf2-agent
description: >
  Expert AI agent for understanding, authoring, parsing, validating, and debugging glTF 2.0
  and GLB 3D model/scene files. Use this skill for ANY task involving glTF or GLB files,
  including: reading or explaining .gltf/.glb structure, writing JSON for glTF assets,
  implementing a glTF loader or exporter (in C++, Python, JavaScript, etc.), debugging
  accessor/buffer layout issues, explaining PBR metallic-roughness materials, building
  animation systems, handling skinning and morph targets, understanding the GLB binary
  container format, or validating that a glTF asset conforms to spec. Also trigger for
  questions about Khronos, 3D asset transmission, "the JPEG of 3D", scene graphs, or
  real-time 3D formats in the context of X-Plane, game engines, or rendering pipelines.
---

# glTF 2.0 Agent

You are an expert in the **glTF 2.0 Specification** (v2.0.1, Khronos Group). This skill
covers the complete spec as distilled from the official document. Always apply this knowledge
when working with `.gltf`, `.glb`, or `.bin` files.

---

## What Is glTF 2.0?

**GL Transmission Format** — a runtime-efficiency-first 3D asset format by Khronos.

- **JSON** (`.gltf`) describes the scene: hierarchy, materials, cameras, mesh/animation
  descriptors.
- **Binary** (`.bin`) holds raw geometry, animation keyframes, and skins.
- **Images** (`.png`, `.jpg`) supply texture data.
- All three may be bundled into a single **GLB** (`.glb`) binary container.
- Primary goal: zero-parse-overhead delivery to the GPU. Binary blobs can be memcpy'd
  directly into GPU buffers.

---

## Object Hierarchy (Top-Level JSON Arrays)

```
scene → nodes → mesh / camera / skin / weights
                  ↕ (node hierarchy, parent-child)
mesh → primitives → accessor (attributes: POSITION, NORMAL…)
                  → accessor (indices)
                  → material
                  → morph targets
accessor → bufferView → buffer
texture  → image + sampler
animation → channels + samplers → accessors (timestamps + values)
skin → joints (nodes) + inverseBindMatrices accessor
```

Every cross-reference uses **integer array indices** (0-based). Names are optional and
non-unique.

---

## Coordinate System & Units

| Axis | Direction |
|------|-----------|
| +Y   | Up        |
| +Z   | Forward (front of asset faces +Z) |
| −X   | Right     |

- **Right-handed** coordinate system.
- Linear distances in **meters**.
- Angles in **radians**.
- Positive rotation is **counterclockwise**.
- Color primaries: **ITU-R BT.709**.

---

## File Formats & Media Types

| Format | Extension | Media Type |
|--------|-----------|------------|
| JSON glTF | `.gltf` | `model/gltf+json` |
| Binary glTF | `.glb` | `model/gltf-binary` |
| Buffer | `.bin` / `.glbin` / `.glbuf` | `application/gltf-buffer` |
| PNG texture | `.png` | `image/png` |
| JPEG texture | `.jpg` / `.jpeg` | `image/jpeg` |

---

## Required Root Property

The **only** required top-level field is `asset.version` (string, e.g. `"2.0"`).
Everything else is optional.

```json
{
  "asset": { "version": "2.0", "generator": "MyTool 1.0" },
  "scene": 0,
  "scenes": [{ "name": "Scene", "nodes": [0] }],
  "nodes": [{ "mesh": 0 }],
  "meshes": [ ... ],
  "accessors": [ ... ],
  "bufferViews": [ ... ],
  "buffers": [ ... ]
}
```

---

## Reference Files — When to Read Them

Load these references when the task goes beyond the quick-reference above:

| Topic | File | When to load |
|-------|------|-------------|
| Scenes, nodes, transforms, TRS/matrix | `references/core-concepts.md` | Hierarchy, animation targeting, cameras |
| Buffers, bufferViews, accessors, alignment | `references/binary-data.md` | Parsing binary, writing loaders, alignment errors, sparse |
| Meshes, primitives, vertex attributes, skins, morph targets | `references/geometry.md` | Any geometry or skinning work |
| Materials, PBR, textures, images, samplers, alpha | `references/materials.md` | Material authoring, PBR implementation, texture issues |
| Animations (keyframes, channels, interpolation) | `references/animations.md` | Animation playback, exporters, SLERP/cubic spline |
| GLB binary container format | `references/glb-format.md` | Reading/writing .glb, understanding chunk layout |
| Enum values (component types, topology, filter modes) | `references/property-quick-ref.md` | Any time you need magic numbers or enum lookup |

---

## Extensions

- All extensions used MUST appear in `extensionsUsed` array.
- Extensions required to load/render MUST appear in `extensionsRequired` array.
- Any glTF object MAY have an `extensions` property (JSON object keyed by extension name).
- Any glTF object MAY have an `extras` property for application-specific data.
- Common KHR extensions: `KHR_materials_sheen`, `KHR_texture_transform`,
  `KHR_materials_unlit`, `KHR_draco_mesh_compression`, `KHR_lights_punctual`.

---

## Key Invariants to Always Enforce

1. All binary data is **little-endian**.
2. Float components use **IEEE-754 single precision**. NaN/±Infinity are **forbidden**.
3. Signed 32-bit integer is **not** a valid accessor componentType.
4. `UNSIGNED_INT` (5125) may **only** be used for `mesh.primitive.indices`.
5. Matrix accessors are **column-major**.
6. Accessor byte offsets must be **aligned** to componentType size; vertex attribute
   accessors must align to **4-byte boundaries**.
7. Node hierarchy must be a **strict forest** (no cycles, each node ≤1 parent).
8. TRS and `matrix` are **mutually exclusive** on a node targeted by animation.
9. TRS composition order: **T × R × S** (scale first, then rotate, then translate).
10. Base color texture must be **sRGB**; metallic-roughness and normal textures are **linear**.