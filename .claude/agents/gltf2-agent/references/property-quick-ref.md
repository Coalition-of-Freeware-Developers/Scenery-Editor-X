# glTF 2.0 — Property Quick Reference

A lookup table for all enum codes, magic numbers, and default values. Use this when you
need a specific integer value or want to validate a field quickly.

---

## Accessor Component Types

| Code | Name           | TypedArray   | Bytes | Notes                      |
| ---- | -------------- | ------------ | ----- | -------------------------- |
| 5120 | BYTE           | Int8Array    | 1     | Signed                     |
| 5121 | UNSIGNED_BYTE  | Uint8Array   | 1     |                            |
| 5122 | SHORT          | Int16Array   | 2     | Signed                     |
| 5123 | UNSIGNED_SHORT | Uint16Array  | 2     | Most common for indices    |
| 5125 | UNSIGNED_INT   | Uint32Array  | 4     | Indices only               |
| 5126 | FLOAT          | Float32Array | 4     | Most common for attributes |

**No signed 32-bit INT. No 64-bit types.**

---

## Accessor Types

| String   | Components | Typical use                                    |
| -------- | ---------- | ---------------------------------------------- |
| "SCALAR" | 1          | Indices, timestamps, morph weights             |
| "VEC2"   | 2          | Texture coordinates                            |
| "VEC3"   | 3          | Positions, normals, translation, scale         |
| "VEC4"   | 4          | Tangents, colors, quaternions, joints, weights |
| "MAT2"   | 4          | Rarely used                                    |
| "MAT3"   | 9          | Rarely used                                    |
| "MAT4"   | 16         | Inverse bind matrices, node matrix             |

---

## Mesh Primitive Modes

| Code        | Name                | Min verts   | Divisor     |
| ----------- | ------------------- | ----------- | ----------- |
| 0           | POINTS              | 1           | —          |
| 1           | LINES               | 2           | 2           |
| 2           | LINE_LOOP           | 2           | —          |
| 3           | LINE_STRIP          | 2           | —          |
| **4** | **TRIANGLES** | **3** | **3** |
| 5           | TRIANGLE_STRIP      | 3           | —          |
| 6           | TRIANGLE_FAN        | 3           | —          |

**Default: 4 (TRIANGLES)**

---

## Buffer View Targets

| Code  | Name                 | Used for          |
| ----- | -------------------- | ----------------- |
| 34962 | ARRAY_BUFFER         | Vertex attributes |
| 34963 | ELEMENT_ARRAY_BUFFER | Vertex indices    |

---

## Texture Sampler — Magnification Filter

| Code | Name    |
| ---- | ------- |
| 9728 | NEAREST |
| 9729 | LINEAR  |

---

## Texture Sampler — Minification Filter

| Code | Name                             |
| ---- | -------------------------------- |
| 9728 | NEAREST                          |
| 9729 | LINEAR                           |
| 9984 | NEAREST_MIPMAP_NEAREST           |
| 9985 | LINEAR_MIPMAP_NEAREST            |
| 9986 | NEAREST_MIPMAP_LINEAR            |
| 9987 | LINEAR_MIPMAP_LINEAR (trilinear) |

---

## Texture Sampler — Wrapping Modes

| Code  | Name            | Behavior       |
| ----- | --------------- | -------------- |
| 10497 | REPEAT          | Tile (default) |
| 33071 | CLAMP_TO_EDGE   | Edge clamp     |
| 33648 | MIRRORED_REPEAT | Mirror tile    |

**Default wrapS and wrapT: 10497 (REPEAT)**

---

## Animation Interpolation Modes

| String        | Output count       | Notes                                  |
| ------------- | ------------------ | -------------------------------------- |
| "LINEAR"      | = input count      | SLERP for rotation                     |
| "STEP"        | = input count      | Hold value                             |
| "CUBICSPLINE" | = 3 × input count | In-tangent, value, out-tangent per key |

**Default: "LINEAR"**

---

## Animation Channel Target Paths

| String        | Accessor type            | Description          |
| ------------- | ------------------------ | -------------------- |
| "translation" | VEC3 float               | XYZ in meters        |
| "rotation"    | VEC4 float (or norm int) | XYZW quaternion      |
| "scale"       | VEC3 float               | XYZ scale            |
| "weights"     | SCALAR (repeated)        | Morph target weights |

---

## Camera Types

| String         | Required properties                                                        |
| -------------- | -------------------------------------------------------------------------- |
| "perspective"  | `yfov`(radians),`znear`(>0); optional:`aspectRatio`,`zfar`         |
| "orthographic" | `xmag`,`ymag`,`zfar`,`znear`(all required, ≠0 for xmag/ymag/zfar) |

---

## Material Alpha Modes

| String   | Behavior                                        |
| -------- | ----------------------------------------------- |
| "OPAQUE" | Ignore alpha (default)                          |
| "MASK"   | Cutoff threshold (`alphaCutoff`, default 0.5) |
| "BLEND"  | Porter-Duff over compositing                    |

---

## GLB Magic Numbers

| Field              | Value      | Notes                                |
| ------------------ | ---------- | ------------------------------------ |
| Header magic       | 0x46546C67 | "glTF" ASCII bytes: 67 6C 54 46      |
| GLB version        | 2          | Container version (not glTF version) |
| JSON chunk type    | 0x4E4F534A | "JSON" in ASCII                      |
| BIN chunk type     | 0x004E4942 | "BIN\0" in ASCII                     |
| JSON chunk padding | 0x20       | Space character                      |
| BIN chunk padding  | 0x00       | Null byte                            |

---

## Default Property Values

| Object.property                               | Default        |
| --------------------------------------------- | -------------- |
| accessor.byteOffset                           | 0              |
| accessor.normalized                           | false          |
| animation.sampler.interpolation               | "LINEAR"       |
| bufferView.byteOffset                         | 0              |
| material.alphaMode                            | "OPAQUE"       |
| material.alphaCutoff                          | 0.5            |
| material.doubleSided                          | false          |
| material.emissiveFactor                       | [0, 0, 0]      |
| material.pbrMetallicRoughness.baseColorFactor | [1, 1, 1, 1]   |
| material.pbrMetallicRoughness.metallicFactor  | 1.0            |
| material.pbrMetallicRoughness.roughnessFactor | 1.0            |
| material.normalTextureInfo.scale              | 1.0            |
| material.occlusionTextureInfo.strength        | 1.0            |
| mesh.primitive.mode                           | 4 (TRIANGLES)  |
| node.matrix                                   | Identity 4×4  |
| node.rotation                                 | [0, 0, 0, 1]   |
| node.scale                                    | [1, 1, 1]      |
| node.translation                              | [0, 0, 0]      |
| sampler.wrapS                                 | 10497 (REPEAT) |
| sampler.wrapT                                 | 10497 (REPEAT) |
| textureInfo.texCoord                          | 0              |
| material.normalTextureInfo.texCoord           | 0              |
| material.occlusionTextureInfo.texCoord        | 0              |

---

## Required Fields Summary

| Object                        | Required fields                         |
| ----------------------------- | --------------------------------------- |
| glTF root                     | `asset`                               |
| asset                         | `version`                             |
| accessor                      | `componentType`,`count`,`type`    |
| animation                     | `channels`,`samplers`               |
| animation.channel             | `sampler`,`target`                  |
| animation.channel.target      | `path`                                |
| animation.sampler             | `input`,`output`                    |
| animation.sparse              | `count`,`indices`,`values`        |
| animation.sparse.indices      | `bufferView`,`componentType`        |
| animation.sparse.values       | `bufferView`                          |
| buffer                        | `byteLength`                          |
| bufferView                    | `buffer`,`byteLength`               |
| camera                        | `type`                                |
| camera.orthographic           | `xmag`,`ymag`,`zfar`,`znear`    |
| camera.perspective            | `yfov`,`znear`                      |
| image                         | `uri`OR `bufferView`(+`mimeType`) |
| mesh                          | `primitives`                          |
| mesh.primitive                | `attributes`                          |
| skin                          | `joints`                              |
| textureInfo                   | `index`                               |
| material.normalTextureInfo    | `index`                               |
| material.occlusionTextureInfo | `index`                               |

---

## Vertex Attribute Semantic Summary

| Semantic   | Type      | ComponentType                  | Notes             |
| ---------- | --------- | ------------------------------ | ----------------- |
| POSITION   | VEC3      | float                          | min/max REQUIRED  |
| NORMAL     | VEC3      | float                          |                   |
| TANGENT    | VEC4      | float                          | W must be ±1     |
| TEXCOORD_n | VEC2      | float, UBYTE_norm, USHORT_norm |                   |
| COLOR_n    | VEC3/VEC4 | float, UBYTE_norm, USHORT_norm | Clamped [0,1]     |
| JOINTS_n   | VEC4      | UBYTE, USHORT                  |                   |
| WEIGHTS_n  | VEC4      | float, UBYTE_norm, USHORT_norm | Sum ≈ 1          |
| _CUSTOM    | any       | any except UINT                | Underscore prefix |

---

## Image Byte Signatures

| Format | Bytes                   |
| ------ | ----------------------- |
| PNG    | 89 50 4E 47 0D 0A 1A 0A |
| JPEG   | FF D8 FF                |

---

## URI Types Supported

| Type              | Example                                        |
| ----------------- | ---------------------------------------------- |
| Relative path     | `"duck.bin"`,`"textures/diffuse.png"`      |
| Data URI (buffer) | `"data:application/octet-stream;base64,..."` |
| Data URI (image)  | `"data:image/png;base64,..."`                |

Clients MAY optionally support: absolute paths, `http://`, `file://`, query params.
Assets using these are less portable.
