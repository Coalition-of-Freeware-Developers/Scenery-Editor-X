# glTF 2.0 — Geometry Reference

## Meshes

A mesh is a collection of **primitives** (draw calls). It's placed in the scene by a node.

```json
{
  "meshes": [
    {
      "name": "Duck",
      "primitives": [ ... ],   // REQUIRED: at least one
      "weights": [0.0, 0.5]    // optional: default morph target weights
    }
  ]
}
```

The same mesh CAN be referenced by multiple nodes (instancing). Each node may apply
different transforms and different morph `weights`.

---

## Primitives

A primitive maps geometry (vertex attributes + optional indices) to a material and draw mode.

```json
{
  "primitives": [
    {
      "attributes": {       // REQUIRED: at least one attribute
        "POSITION":    22,  // accessor index
        "NORMAL":      23,
        "TANGENT":     24,
        "TEXCOORD_0":  25,
        "COLOR_0":     26,
        "JOINTS_0":    27,
        "WEIGHTS_0":   28
      },
      "indices":  21,       // optional: accessor index for index buffer (SCALAR unsigned int)
      "material": 3,        // optional: material index (default material if absent)
      "mode":     4,        // optional, default 4 (TRIANGLES)
      "targets": [ ... ]    // optional: morph targets
    }
  ]
}
```

### Topology Modes

| Code | Name           | Minimum vertices   |
| ---- | -------------- | ------------------ |
| 0    | POINTS         | 1                  |
| 1    | LINES          | 2 (divisible by 2) |
| 2    | LINE_LOOP      | 2                  |
| 3    | LINE_STRIP     | 2                  |
| 4    | TRIANGLES      | 3 (divisible by 3) |
| 5    | TRIANGLE_STRIP | 3                  |
| 6    | TRIANGLE_FAN   | 3                  |

**Default is TRIANGLES (4).**

### Geometry Assembly Equations

```
POINTS:          pᵢ = {vᵢ}
LINE_STRIP:      pᵢ = {vᵢ, vᵢ₊₁}
LINE_LOOP:       same as strip + final segment back to v₀
LINES:           pᵢ = {v₂ᵢ, v₂ᵢ₊₁}
TRIANGLES:       pᵢ = {v₃ᵢ, v₃ᵢ₊₁, v₃ᵢ₊₂}
TRIANGLE_STRIP:  pᵢ = {vᵢ, vᵢ₊(1+i%2), vᵢ₊(2-i%2)}
TRIANGLE_FAN:    pᵢ = {vᵢ₊₁, vᵢ₊₂, v₀}
```

### Indices Rules

* `indices` accessor MUST be `SCALAR` type with UNSIGNED_BYTE, UNSIGNED_SHORT, or UNSIGNED_INT.
* Maximum index value (255, 65535, 4294967295) is **forbidden** (triggers primitive restart).
* All index values MUST be < `accessor.count` of all attribute accessors.

### Render Counts

* Without indices: `count` of attribute accessors = vertex count.
* With indices: `count` of indices accessor = render vertex count; attribute count is upper bound.

---

## Standard Vertex Attribute Semantics

| Name       | Accessor Type | Component Type                 | Description                                       |
| ---------- | ------------- | ------------------------------ | ------------------------------------------------- |
| POSITION   | VEC3          | float                          | XYZ positions (meters).**min/max REQUIRED** |
| NORMAL     | VEC3          | float                          | Normalized XYZ vertex normals                     |
| TANGENT    | VEC4          | float                          | XYZ normalized + W=±1 handedness                 |
| TEXCOORD_n | VEC2          | float, UBYTE_norm, USHORT_norm | ST texture coords                                 |
| COLOR_n    | VEC3/VEC4     | float, UBYTE_norm, USHORT_norm | Linear RGB(A) multiplier [0,1]                    |
| JOINTS_n   | VEC4          | UBYTE, USHORT                  | Joint indices into skin.joints                    |
| WEIGHTS_n  | VEC4          | float, UBYTE_norm, USHORT_norm | Joint influence weights                           |

### Attribute Rules

* All attribute accessors in a primitive MUST have the  **same `count`** .
* `TANGENT.w` MUST be exactly +1 or −1.
* When `COLOR_n` is VEC3, alpha is assumed 1.0. All components MUST be clamped to [0,1].
* `TEXCOORD_n`, `COLOR_n`, `JOINTS_n`, `WEIGHTS_n` indices start at 0 and are consecutive
  (no skipping: `TEXCOORD_0`, `TEXCOORD_1`, not `TEXCOORD_0`, `TEXCOORD_2`).
* No leading zeros in index (e.g., `TEXCOORD_01` is  **invalid** ).
* Client SHOULD support at least: 2 UV sets, 1 color set, 1 joints/weights set.

### Application-Specific Attributes

Custom attributes MUST start with underscore: `_TEMPERATURE`, `_WIND_SPEED`.
Custom attributes MUST NOT use UNSIGNED_INT component type.

### Missing Attributes — Fallback Behavior

| Missing  | Required behavior                                                              |
| -------- | ------------------------------------------------------------------------------ |
| POSITION | Skip rendering (unless extension provides positions)                           |
| NORMAL   | Calculate**flat normals** ; ignore provided tangents                     |
| TANGENT  | Calculate using**MikkTSpace**algorithm from positions, normals, TEXCOORD |

### Tangent Space (Bitangent)

```
bitangent = cross(normal.xyz, tangent.xyz) × tangent.w
```

All vertices of a triangle SHOULD have the same `tangent.w` value.

---

## Morph Targets

Morph targets store **delta** values (displacements) for vertex attributes.

```json
{
  "primitives": [{
    "attributes": { "POSITION": 22, "NORMAL": 23, "TANGENT": 24 },
    "targets": [
      { "POSITION": 32, "NORMAL": 33, "TANGENT": 34 },  // target 0 deltas
      { "POSITION": 42, "NORMAL": 43, "TANGENT": 44 }   // target 1 deltas
    ]
  }],
  "weights": [0.0, 0.5]    // mesh default weights
}
```

### Computing Morphed Vertex Positions

```
final_POSITION = base.POSITION
               + weights[0] × targets[0].POSITION
               + weights[1] × targets[1].POSITION + ...
```

Same formula for NORMAL, TANGENT, TEXCOORD_n, COLOR_n.

### Morph Target Accessor Types

| Attribute  | Accessor Type | Component Types                                         | Notes                                                     |
| ---------- | ------------- | ------------------------------------------------------- | --------------------------------------------------------- |
| POSITION   | VEC3          | float                                                   | Deltas.**min/max REQUIRED**                         |
| NORMAL     | VEC3          | float                                                   | Deltas                                                    |
| TANGENT    | VEC3          | float                                                   | XYZ only — W is omitted (handedness cannot be displaced) |
| TEXCOORD_n | VEC2          | float, SBYTE_norm, SSHORT_norm, UBYTE_norm, USHORT_norm |                                                           |
| COLOR_n    | VEC3/VEC4     | float, SBYTE_norm, SSHORT_norm, UBYTE_norm, USHORT_norm |                                                           |

### Morph Rules

* Morph displacement MUST be applied **before** skinning or node transforms.
* Attributes in base mesh but absent from a morph target retain their base values.
* All primitives in a mesh MUST have the same number of targets in the same order.
* If base mesh has no NORMAL, recalculate flat normals per target; ignore tangent deltas.
* If base mesh has no TANGENT, recalculate tangents per target using MikkTSpace.
* COLOR_n VEC3 deltas → alpha assumed 0.0 (no alpha displacement).
* After applying COLOR_n deltas, clamp all components to [0,1].
* All morph target accessors MUST have the same `count` as base attribute accessors.
* Client SHOULD support ≥8 morphed attributes simultaneously.

### Morph Weight Precedence

1. `node.weights` (overrides all)
2. `mesh.weights` (mesh default)
3. All zeros (if neither defined)

### Target Names (Common Convention)

Not in spec, but widely used:

```json
"extras": { "targetNames": ["smile", "blink", "jawOpen"] }
```

---

## Skins (Linear Blend Skinning)

A skin drives skeletal animation by applying per-vertex weighted transforms from joint nodes.

```json
{
  "skins": [
    {
      "name": "Armature",
      "joints": [1, 2, 3],          // REQUIRED: node indices used as joints
      "inverseBindMatrices": 29,    // optional: accessor index (MAT4, float)
      "skeleton": 1                 // optional: common root node
    }
  ]
}
```

### Joint Hierarchy

* All joints MUST share a common root ancestor (**common root** node).
* The common root need not be a joint itself.
* When a skin is referenced in a scene, the common root MUST be in that scene.
* A joint node MAY have non-skinned meshes attached (e.g., weapon attached to hand joint).

### Inverse Bind Matrices

The `inverseBindMatrices` accessor MUST be:

* Type: **MAT4**
* Component type: **float**
* Count: ≥ number of joints

Each matrix transforms a vertex from mesh space into the corresponding joint's local space.
When omitted, each inverse bind matrix is the  **identity** .

The fourth row of each matrix MUST be `[0, 0, 0, 1]`.

### Skinned Mesh Attributes

Every skinned mesh primitive MUST include `JOINTS_0` and `WEIGHTS_0`.

```
JOINTS_n  → VEC4, UBYTE or USHORT: indices into skin.joints array
WEIGHTS_n → VEC4, float or normalized UBYTE/USHORT: influence weights
```

### Weight Constraints

* Weights MUST NOT be negative.
* A joint MUST NOT have more than one non-zero weight per vertex.
* Float weights: linear sum SHOULD be ≈ 1.0 (threshold: 2e-7 × non-zero weights).
* UBYTE normalized: sum before normalization MUST = 255.
* USHORT normalized: sum before normalization MUST = 65535.
* Unused joints (weight=0) SHOULD have index 0.

### Multiple Joint Sets

More than 4 joints/vertex → use `JOINTS_1`/`WEIGHTS_1`, etc.
`count(JOINTS_n) == count(WEIGHTS_n)` for every n.

### Skinning Transform

The transform of the  **skinned mesh node itself is IGNORED** . Only joint transforms apply.

### Per-Vertex Transform Calculation

```
skin_matrix = sum over i:
    weights[i] × joint_global_transform[i] × inverseBindMatrix[i]

final_position = skin_matrix × position
```

---

## Instantiation

The same mesh at multiple nodes = **instancing** (no data duplication):

```json
{
  "nodes": [
    { "mesh": 5 },
    { "mesh": 5, "translation": [-20, 0, 0] }
  ]
}
```

After applying global transform, vertex positions are in  **meters** .
