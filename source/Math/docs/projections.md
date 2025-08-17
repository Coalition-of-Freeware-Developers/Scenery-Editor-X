# Math Library – Projection Matrices

Covers perspective and orthographic matrix builders used for camera and shadow mapping. Intended headers: `projection.h` (or split `perspective.h`, `orthographic.h`).

## Overview

Projection matrices convert view-space coordinates into clip-space. The engine uses row‑major `Mat4` while GPU shaders may expect column-major; transpose on upload if required by API conventions.

## Supported Builders

| Function | Purpose | Notes |
|----------|---------|-------|
| `MakePerspective(fovYRadians, aspect, zNear, zFar)` | Standard perspective | Finite depth range |
| `MakePerspectiveInfinite(fovYRadians, aspect, zNear)` | Infinite far plane | Useful for sky / large worlds |
| `MakePerspectiveReverseZ(fovYRadians, aspect, zNear, zFar)` | Reverse-Z mapping | Improves depth precision (zFar <-> zNear swap logic) |
| `MakeOrthographic(left, right, bottom, top, zNear, zFar)` | Axis-aligned box | UI, shadow cascades |
| `MakeOrthographicCentered(width, height, zNear, zFar)` | Center at origin | Convenience wrapper |
| `ExtractFrustumPlanes(matrix)` | Frustum plane extraction | Culling (normalize planes) |
| `ReconstructPosition(depth, ndc, inverseProj)` | View-space position | G-buffer / SSAO |

## Parameter Contracts

| Parameter | Constraints | Rationale |
|-----------|-------------|-----------|
| `fovYRadians` | (0, π) exclusive | Avoid degeneracy / tan undefined |
| `aspect` | > 0 | Division safety |
| `zNear` | > 0 (non-zero) | Projection singularity otherwise |
| `zFar` | > `zNear` (finite variants) | Ensures positive depth range |

## Numerical Stability

- Clamp extremely small `zNear` to minimum (e.g. `1e-4f`) to limit floating point cancellation.
- Reverse-Z variant stores depth with greater precision for distant geometry (requires clearing depth to 0 and using `GREATER` test).
- Infinite projection sets matrix terms to remove `zFar` usage; confirm GPU side depth comparison logic matches.

## Perspective Matrix (Row-Major Sketch)

```cpp
Mat4 MakePerspective(float fovY, float aspect, float zNear, float zFar)
{
    float f = 1.0f / std::tan(fovY * 0.5f);
    Mat4 m = Mat4::Identity();
    m.m00 = f / aspect;
    m.m11 = f;
    m.m22 = zFar / (zFar - zNear);          // row-major depth term (adjust if convention differs)
    m.m23 = 1.0f;                           // position of perspective term
    m.m32 = (-zNear * zFar) / (zFar - zNear);
    m.m33 = 0.0f;
    return m;
}
```

> Validate against actual implementation; adjust row/column accessors as needed. Keep doc consistent.

## Reverse-Z Perspective

Replace depth terms (example form):

```cpp
m.m22 = zNear / (zNear - zFar);
m.m32 = (zFar * zNear) / (zNear - zFar); // Signs inverted relative to standard
```

Ensure depth buffer configured for reverse mapping: clear depth to 0, comparison `GREATER` or `GREATER_EQUAL`.

## Infinite Perspective

Let `zFar → ∞`, limit of standard matrix yields:

```cpp
m.m22 = 1.0f;
m.m32 = -zNear;
```

Used for vast scenes where explicit far plane is undesirable. Combine with fog or depth-based fading to avoid artifacts.

## Orthographic Matrix (Row-Major Sketch)

```cpp
Mat4 MakeOrthographic(float l, float r, float b, float t, float n, float f)
{
    Mat4 m = Mat4::Identity();
    m.m00 = 2.0f / (r - l);
    m.m11 = 2.0f / (t - b);
    m.m22 = 1.0f / (f - n);      // or 2/(f-n) depending on clip range convention
    m.m30 = -(r + l) / (r - l);
    m.m31 = -(t + b) / (t - b);
    m.m32 = -n / (f - n);        // adjust for reverse-Z or OpenGL-style [-1,1]
    return m;
}
```

## Clip Space Conventions

| API | NDC Z Range | Typical Depth Clear | Reverse-Z Friendly |
|-----|-------------|---------------------|--------------------|
| Vulkan / D3D | [0, 1] | 1.0 (forward) / 0.0 (reverse) | Yes |
| OpenGL (core) | [-1, 1] | 1.0 (forward) | Requires adjustments |

Document which space engine standardizes on (likely [0,1]). Conversions needed if interoperating with imported matrices from tools assuming OpenGL.

## Frustum Plane Extraction

Normalize each plane after extraction to ensure consistent distance tests.

Plane form: `ax + by + cz + d = 0`; store `(a,b,c,d)` in struct with helper `Distance(point)`.

## Reconstructing View-Space Position

Deferred shading / SSAO often reconstruct view position from depth:

```cpp
Vec3 Reconstruct(float depth, Vec2 ndc, const Mat4& invProj)
{
    Vec4 h = invProj * Vec4(ndc.x, ndc.y, depth, 1.0f);
    return (h.xyz / h.w);
}
```

Ensure depth linearization matches chosen projection variant (reverse, infinite, etc.).

## Error Handling & Assertions

- Assert parameter bounds (`fovY > 0 && fovY < PI`).
- Verify non-zero denominators before division.
- Log (`MATH` tag) on suspicious aspect ratios (< ~0.01 or > ~100).

## Performance Considerations

| Concern | Guidance |
|---------|----------|
| Rebuilding projection each frame | Avoid; only rebuild on parameter change |
| Frustum extraction frequency | Cache planes until matrix changes |
| Branches for variants | Prefer separate functions over runtime flags for inlining |

## Suggested Extensions

| Feature | Priority | Notes |
|---------|----------|-------|
| Jittered perspective (TAA) | High | Per-frame sub-pixel offsets |
| Cascaded projection helpers | High | Shadow cascade splits & matrices |
| Oblique near-plane clip | Medium | Reflections / planar clipping |
| Reversed + infinite combo | Medium | Precision + large scenes |
| Frustum vs AABB test util | High | Culling integration |

## Testing Strategy

- **Parameter Boundaries**: Very small & large FOV values.
- **Depth Precision**: Compare forward vs reverse-Z precision at far distances.
- **Frustum Culling**: Known boxes inside/outside/straddling frustum.
- **Round Trip**: Project + inverse project sample points (epsilon close).

## TODO / Backlog

- Confirm final row-major element indices in implementation and sync examples.
- Add SIMD plane extraction if profiling justifies.
- Provide depth linearization helper functions.

---

*Generated documentation file; synchronize with renderer conventions.*
