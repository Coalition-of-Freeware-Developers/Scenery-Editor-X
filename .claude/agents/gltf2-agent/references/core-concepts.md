# glTF 2.0 — Core Concepts Reference

## Asset Metadata

Every glTF MUST have an `asset` object with at least `version`.

```json
{
  "asset": {
    "version": "2.0",        // REQUIRED
    "minVersion": "2.0",     // Optional: minimum required version
    "generator": "Blender",  // Optional: tool info for debugging
    "copyright": "2024 ..."  // Optional: display string
  }
}
```

Client implementations MUST check `minVersion` before loading. If absent, check `version`.
Major version changes may be incompatible. Minor versions are backward + forward compatible.

---

## Scenes

```json
{
  "scene": 0,          // Index of default scene to display
  "scenes": [
    {
      "name": "MyScene",
      "nodes": [0, 3]  // Root node indices (MUST be root nodes; MUST NOT be child of any node)
    }
  ]
}
```

- A glTF asset MAY have zero or more scenes.
- The same root node MAY appear in multiple scenes.
- A glTF with no scenes = library of materials/meshes.
- When `scene` is undefined, clients MAY delay rendering until a scene is requested.

---

## Nodes and Hierarchy

Nodes form a **strict forest** (set of disjoint strict trees):
- No cycles.
- Each node has at most one parent.
- Root nodes are not listed in any `node.children`.

```json
{
  "nodes": [
    {
      "name": "Car",
      "children": [1, 2, 3, 4],   // indices into nodes array
      "mesh": 0,                   // optional: mesh index
      "camera": 0,                 // optional: camera index
      "skin": 0,                   // optional: skin index (requires mesh)
      "weights": [0.0, 0.5],       // optional: morph target weights (overrides mesh.weights)
      "translation": [0, 0, 0],    // optional: XYZ in meters
      "rotation": [0, 0, 0, 1],    // optional: XYZW unit quaternion (W is scalar)
      "scale": [1, 1, 1],          // optional: XYZ scale factors
      "matrix": [ ... 16 floats ]  // optional: column-major 4x4 (mutually exclusive with TRS)
    }
  ]
}
```

A node can reference at most one mesh, camera, and skin. All are optional.

---

## Transformations

### TRS vs Matrix

- Node transform = either `matrix` OR any combination of `translation`, `rotation`, `scale`.
- **Cannot mix** `matrix` with TRS on the same node.
- When a node is **targeted by animation**, `matrix` MUST NOT be present.

### TRS Composition (T × R × S)

```
local_matrix = translate(T) × rotate(R) × scale(S)
```

Order of application to vertices: **Scale first, then Rotate, then Translate.**

### Global Transform

```
global_transform = parent.global_transform × local_transform
```

Root nodes: global = local.

### Defaults (when not specified)

| Property | Default |
|----------|---------|
| translation | [0, 0, 0] |
| rotation | [0, 0, 0, 1] (identity quaternion) |
| scale | [1, 1, 1] |
| matrix | identity 4×4 |

### Quaternion Convention

Rotation is stored as `[x, y, z, w]` where `w` is the scalar component.
Must be a **unit quaternion**.

### Column-Major Matrix

Matrix 16 elements in column-major order:

```
[m00, m10, m20, m30,   // column 0
 m01, m11, m21, m31,   // column 1
 m02, m12, m22, m32,   // column 2
 m03, m13, m23, m33]   // column 3
```

`node.matrix` MUST be decomposable to TRS (no skew/shear).

### Winding Order

When a mesh has triangle topology, the **sign of the determinant** of the node's global
transform determines winding:
- Positive determinant → counterclockwise (front-facing)
- Negative determinant → clockwise (mirrored via negative scale)

### Scale-to-Zero Visibility Trick

Setting all three scale components to zero is a valid way to hide a mesh. Implementations
MAY skip rendering such nodes. **Exception**: skinned meshes must not use this unless ALL
joints are zeroed simultaneously.

---

## Cameras

Cameras are placed in the scene by attaching them to a node (`node.camera = index`).
The node defines the **view matrix** (global transform with scale ignored).
The camera object defines the **projection matrix**.

```json
{
  "cameras": [
    {
      "name": "PerspCam",
      "type": "perspective",    // or "orthographic"
      "perspective": {
        "aspectRatio": 1.7778,  // optional; use viewport ratio if absent
        "yfov": 0.6911,         // REQUIRED: vertical FOV in radians (< π recommended)
        "znear": 0.01,          // REQUIRED: > 0
        "zfar": 1000.0          // optional; omit for infinite projection
      }
    },
    {
      "name": "OrthoCam",
      "type": "orthographic",
      "orthographic": {
        "xmag": 10.0,   // REQUIRED: half-width (≠ 0, should be > 0)
        "ymag": 7.5,    // REQUIRED: half-height (≠ 0, should be > 0)
        "znear": 0.0,   // REQUIRED: ≥ 0
        "zfar": 100.0   // REQUIRED: > 0, > znear
      }
    }
  ]
}
```

### Camera Orientation

The camera looks **−Z** in local space. +X is right, +Y is up.

### Projection Matrices (client should implement)

**Infinite perspective** (zfar undefined):

```
[ 1/(a·tan(y/2)),  0,               0,   0 ]
[ 0,               1/tan(y/2),      0,   0 ]
[ 0,               0,              -1,  -1 ]
[ 0,               0,         -2·n,   0 ]
```
where `a` = aspectRatio, `y` = yfov, `n` = znear.

**Finite perspective** (zfar defined):

```
[ 1/(a·tan(y/2)),  0,                  0,   0 ]
[ 0,               1/tan(y/2),         0,   0 ]
[ 0,               0,          -(f+n)/(f-n), -1 ]
[ 0,               0,       -2·f·n/(f-n),    0 ]
```

**Orthographic**:

```
[ 1/r,   0,    0,          0 ]
[ 0,    1/t,   0,          0 ]
[ 0,     0,  -2/(f-n),     0 ]
[ 0,     0, -(f+n)/(f-n),  1 ]
```
where `r` = xmag, `t` = ymag.

When camera aspect ratio doesn't match viewport, do NOT crop or stretch.

---

## Extensions and Extras

Every glTF object supports:

```json
{
  "extensions": {
    "KHR_materials_sheen": {
      "sheenColorFactor": [1.0, 0.33, 0.1],
      "sheenRoughnessFactor": 0.8
    }
  },
  "extras": {
    "myAppData": "custom value"
  }
}
```

Top-level:
```json
{
  "extensionsUsed": ["KHR_materials_sheen", "VENDOR_physics"],
  "extensionsRequired": ["KHR_texture_transform"]
}
```

`extensionsRequired` ⊆ `extensionsUsed`. If a required extension is unknown, the client
SHOULD NOT load the asset.