# Math Library – Matrices

Covers `mat2.h`, `mat3.h`, `mat4.h`, and aggregate header `matrix.h`.

## Overview

Row‑major matrices for deterministic layout and intuitive memory inspection. Each matrix class is a lightweight aggregate exposing public float members; no hidden storage or dynamic allocation.

| Type | Dimensions | Purpose | Typical Uses |
|------|------------|---------|--------------|
| `Mat2` | 2×2 | 2D linear algebra | UV transforms, 2D rotation/scale, minor Jacobians |
| `Mat3` | 3×3 | 3D linear (rotation/scale/skew) | Normal matrices, local orientations |
| `Mat4` | 4×4 | 3D affine & projective | World, view, projection, TRS composition |

## Memory Layout (Row-Major)

`Mat2`: `[ m00 m01 ; m10 m11 ]`

`Mat3`: `[ m00 m01 m02 ; m10 m11 m12 ; m20 m21 m22 ]`

`Mat4`: 16 contiguous floats (naming documented in header – ensure consistent use when adding math ops).

## Key Design Points

- **No implicit transposition**: What you see in memory matches the declaration.
- **Formatting**: `Mat4` integrates with `{fmt}` for logging; all matrices have `operator<<` for debug streams.
- **Construction**: Defaults to identity (confirmed for `Mat2`; design intent for others – ensure additions follow pattern).

## Common Operations (Planned / Existing)

| Operation | `Mat2` | `Mat3` | `Mat4` | Notes |
|-----------|--------|--------|--------|-------|
| Identity ctor | ✓ | (expected) | (expected) | Default ctor sets identity |
| Copy / Move | ✓ | ✓ | ✓ | Trivial |
| Equality (exact) | TBD | TBD | TBD | Prefer epsilon compare for floats |
| Multiply (matrix * matrix) | TODO | TODO | TODO | Introduce carefully (perf) |
| Multiply (matrix * vector) | TODO | TODO | TODO | Document vector orientation |
| Determinant | TODO | TODO | TODO | Required for inverse |
| Inverse | TODO | TODO | TODO | Optimize `Mat4` affine path |
| Transpose | TODO | TODO | TODO | Provide free or member function |
| From Quaternion | — | ✓ (extract 3×3) | ✓ (upper-left) | Implement in `quat` utilities |
| Compose TRS | — | — | ✓ | Provided via `Transforms::Compose` |
| Decompose TRS | — | — | ✓ | `Transforms::Decompose` |

> Mark operations implemented as you extend the code; table acts as quick status view.

## Mat4 & Transform Stack

Typical creation path:

```cpp
Mat4 world = Transforms::Compose(translation, rotationQuat, scaleVec);
Mat4 mvp   = projection * view * world; // Maintain consistent order across engine
```

Maintain documentation alignment with any future change to ordering conventions (e.g., switching to column-major). If GPU expects column-major, transpose on upload—not in core math operations.

## Formatting & Logging

```cpp
Mat4 m = /* ... */;
SEDX_CORE_TRACE_TAG("MATH", "World Matrix: {}", m); // Uses fmt formatter specialization
```

## Projection / Camera Interop

Projection builders return `Mat4` in row‑major form. When writing shaders, transpose if the shading language expects column-major (Vulkan + GLSL default is column-major for uniform layout but accepts row-major with explicit layout qualifiers).

## Performance Guidance

| Scenario | Guidance |
|----------|----------|
| Per-object composition | Cache `Mat4` after TRS changes; avoid recomputation per draw call. |
| Inversion of rigid transforms | Implement a specialized fast path (transpose rotation, inverse translate) instead of general 4×4 inverse. |
| Normal matrix | Use upper-left 3×3 inverse-transpose only if non-uniform scale present; otherwise rotation matrix transpose suffices. |

## Epsilon & Stability

Use `epsilonEqual` across elements for matrix comparison. Provide helper `MatrixApproxEqual(a,b,eps)` once patterns repeat.

## Suggested Additions

| Feature | Priority | Rationale |
|---------|----------|-----------|
| `Mat4::InverseAffine()` | High | Fast path for rigid body transforms |
| `Mat4 MakeScale(Vec3)` | High | Express intent vs generic compose |
| `Mat4 MakeTranslation(Vec3)` | High | Shortcut for translation-only |
| `Mat4 MakeRotation(Quat)` | High | Cached conversion |
| SIMD (SSE/NEON) path | Medium | Hot loops (multiplies) |
| Determinant/Inverse unit tests | High | Numerical safety |

## Example: Manual 4×4 Multiplication (Future Implementation Sketch)

```cpp
Mat4 Mul(const Mat4& A, const Mat4& B)
{
    Mat4 R; // fill all 16 terms explicitly to aid compiler vectorization
    // R.m00 = A.m00 * B.m00 + A.m01 * B.m10 + A.m02 * B.m20 + A.m03 * B.m30; // etc.
    return R;
}
```

## Error Handling

Matrix functions assert on invalid inputs where meaningful (e.g., decomposition failure). Provide logs with tag `MATH` for numeric anomalies.

## Testing Strategy

- **Determinant / Inverse**: Randomized matrices with known properties (orthonormal, scaled, singular).
- **TRS Round Trip**: Compose then decompose → recompose; compare within epsilon.
- **Projection**: Validate frustum corner reconstruction.

## TODO / Backlog

- Fill in unimplemented math ops with high-performance versions.
- Add constexpr constructors where feasible (Mat3/Mat4 identity).
- Provide serialization helpers (flat array accessor for asset formats).
- Add hashing for use in render state caches.

---

*Generated documentation file; synchronize with code evolution.*
