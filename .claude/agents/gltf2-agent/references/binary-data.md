# glTF 2.0 — Binary Data Reference

## Data Flow

```
buffer (raw bytes on disk / embedded)
  └── bufferView (contiguous slice of buffer + optional stride)
        └── accessor (typed interpretation of bufferView slice)
              → used by: meshes, skins, animations
```

All binary data is  **little-endian** . Buffer data MUST NOT exceed 2⁵³ bytes (JSON parser
limit). GLB-stored binary chunk limit: 2³²−1 bytes.

---

## Buffers

A buffer is a raw binary blob. It MAY reference an external file or embed data as a
base64 Data URI.

```json
{
  "buffers": [
    {
      "uri": "duck.bin",       // relative path OR "data:application/octet-stream;base64,..."
      "byteLength": 102040,    // REQUIRED: must match or be ≤ actual file size
      "name": "MyBuffer"       // optional
    }
  ]
}
```

**GLB-stored buffer** (first buffer, no URI):

```json
{ "byteLength": 35884 }
```

The actual bytes come from the GLB BIN chunk. This buffer MUST be the first element of
`buffers` and MUST NOT have a `uri`.

Data URI media type MUST be `application/octet-stream` or `application/gltf-buffer`.

---

## Buffer Views

A buffer view is a contiguous window into a buffer.

```json
{
  "bufferViews": [
    {
      "buffer": 0,           // REQUIRED: index into buffers array
      "byteOffset": 0,       // optional, default 0
      "byteLength": 25272,   // REQUIRED: ≥ 1
      "byteStride": 32,      // optional: 4–252 (multiple of 4), only for vertex attributes
      "target": 34963,       // optional hint: 34962=ARRAY_BUFFER, 34963=ELEMENT_ARRAY_BUFFER
      "name": "IndicesView"
    }
  ]
}
```

### Target Values

| Value | Meaning              | Used for          |
| ----- | -------------------- | ----------------- |
| 34962 | ARRAY_BUFFER         | Vertex attributes |
| 34963 | ELEMENT_ARRAY_BUFFER | Vertex indices    |

### Buffer View Rules

* A buffer view used for **vertex indices** MUST NOT also be used for **vertex attributes**
  (no mixing of data kinds in the same view).
* `byteStride` MUST be defined when two or more accessors share the same buffer view.
* `byteStride` is ONLY valid for vertex attribute data; other uses MUST NOT define it
  (unless an extension enables it).
* `byteStride` range: 4–252 (multiples of 4 only).

---

## Accessors

An accessor provides a typed view into a buffer view.

```json
{
  "accessors": [
    {
      "bufferView": 1,         // optional (may be absent for sparse/zero-init)
      "byteOffset": 0,         // optional, default 0 (must be multiple of componentType size)
      "componentType": 5126,   // REQUIRED: see Component Types below
      "normalized": false,     // optional, default false
      "count": 2399,           // REQUIRED: number of elements ≥ 1
      "type": "VEC3",          // REQUIRED: "SCALAR","VEC2","VEC3","VEC4","MAT2","MAT3","MAT4"
      "max": [0.96, 1.64, 0.54],  // optional (required for POSITION and animation input)
      "min": [-0.69, 0.10, -0.61], // optional (required for POSITION and animation input)
      "name": "positions"
    }
  ]
}
```

### Component Types

| Code | Type           | Signed                    | Bits | TypedArray   |
| ---- | -------------- | ------------------------- | ---- | ------------ |
| 5120 | BYTE           | Signed two's complement   | 8    | Int8Array    |
| 5121 | UNSIGNED_BYTE  | Unsigned                  | 8    | Uint8Array   |
| 5122 | SHORT          | Signed two's complement   | 16   | Int16Array   |
| 5123 | UNSIGNED_SHORT | Unsigned                  | 16   | Uint16Array  |
| 5125 | UNSIGNED_INT   | Unsigned                  | 32   | Uint32Array  |
| 5126 | FLOAT          | IEEE-754 single precision | 32   | Float32Array |

⚠️ **Signed 32-bit integer (INT) is NOT supported.**
⚠️ `UNSIGNED_INT` (5125) may ONLY be used for `mesh.primitive.indices`.
⚠️ Float values MUST NOT be NaN, +Infinity, or −Infinity.

### Element Types

| type     | Components |
| -------- | ---------- |
| "SCALAR" | 1          |
| "VEC2"   | 2          |
| "VEC3"   | 3          |
| "VEC4"   | 4          |
| "MAT2"   | 4 (2×2)   |
| "MAT3"   | 9 (3×3)   |
| "MAT4"   | 16 (4×4)  |

**Element byte size** = `sizeof(componentType) × numComponents`

Example: `VEC3` of `FLOAT` = 4 × 3 =  **12 bytes per element** .

### Normalization

When `normalized: true`, integer types are interpreted as floats:

| componentType  | int→float        | float→int      |
| -------------- | ----------------- | --------------- |
| BYTE (signed)  | max(c/127, −1)   | round(f×127)   |
| UNSIGNED_BYTE  | c/255             | round(f×255)   |
| SHORT (signed) | max(c/32767, −1) | round(f×32767) |
| UNSIGNED_SHORT | c/65535           | round(f×65535) |

`normalized` MUST NOT be true for FLOAT or UNSIGNED_INT.

---

## Data Alignment Rules

### Accessor Byte Offset Alignment

```
accessor.byteOffset  MUST be a multiple of sizeof(componentType)
accessor.byteOffset + bufferView.byteOffset  MUST be a multiple of sizeof(componentType)
```

### Vertex Attribute 4-Byte Rule

For vertex attributes specifically:

```
accessor.byteOffset  MUST be a multiple of 4
bufferView.byteStride  MUST be a multiple of 4
```

### Fit Check

```
accessor.byteOffset + effectiveStride × (accessor.count − 1) + elementByteSize
  ≤ bufferView.byteLength
```

where `effectiveStride = byteStride` if defined, else `elementByteSize`.

### Matrix Column Alignment

Matrices store data  **column-major** . Each column start MUST be 4-byte aligned.

Only these three matrix configurations require padding:

| Matrix | Component     | Padding per column                              |
| ------ | ------------- | ----------------------------------------------- |
| MAT2   | BYTE or SHORT | 2 padding bytes after each column of 2 elements |
| MAT3   | BYTE          | 1 padding byte after each column of 3 elements  |
| MAT3   | SHORT         | 2 padding bytes after each column of 3 elements |

### Multiple Accessors Sharing a BufferView

When two or more accessors reference the same buffer view (interleaved vertex data),
`bufferView.byteStride` MUST be defined.

---

## Accessor Bounds (min/max)

* `min` and `max` are arrays of per-component values.
* Length = number of components (1 for SCALAR, 3 for VEC3, etc.).
* Must reflect actual data values in the buffer **after** any sparse substitution.
* `normalized` has NO effect on min/max — they always store raw buffer values.
* **REQUIRED** for: `POSITION` attribute accessors and animation sampler `input` accessors.
* For float components, values SHOULD be rounded to single precision before use.

---

## Sparse Accessors

Sparse encoding replaces only the changed elements, leaving others at their initialization value.

```json
{
  "accessors": [
    {
      "bufferView": 0,         // base data (or omit for zero-initialization)
      "componentType": 5123,
      "count": 12636,
      "type": "VEC3",
      "sparse": {
        "count": 10,           // number of overridden elements
        "indices": {
          "bufferView": 1,
          "byteOffset": 0,
          "componentType": 5123  // UNSIGNED_BYTE, UNSIGNED_SHORT, or UNSIGNED_INT only
        },
        "values": {
          "bufferView": 2,
          "byteOffset": 0
        }
      }
    }
  ]
}
```

### Sparse Rules

* `sparse.count` ≤ accessor's `count`.
* Indices MUST form a **strictly increasing** sequence.
* Indices MUST NOT be ≥ accessor's `count`.
* Sparse indices bufferView MUST NOT define `target` or `byteStride`.
* Sparse values bufferView MUST NOT define `target` or `byteStride`.
* If `bufferView` is absent, the base array is  **zero-initialized** .
* `min`/`max` must reflect the final values after sparse substitution.
* **Useful for** : morph target deltas (only displaced vertices stored).

---

## Accessor Example: Vertex Layout

```json
{
  "bufferViews": [{
    "buffer": 0,
    "byteOffset": 0,
    "byteLength": 192,
    "byteStride": 32,    // 12 (pos) + 12 (nrm) + 8 (uv) = 32 bytes/vertex
    "target": 34962
  }],
  "accessors": [
    { "bufferView": 0, "byteOffset": 0,  "componentType": 5126, "count": 6, "type": "VEC3" },  // POSITION
    { "bufferView": 0, "byteOffset": 12, "componentType": 5126, "count": 6, "type": "VEC3" },  // NORMAL
    { "bufferView": 0, "byteOffset": 24, "componentType": 5126, "count": 6, "type": "VEC2" }   // TEXCOORD_0
  ]
}
```
