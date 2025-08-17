# Math Library – Transforms (TRS)

Covers `transform.h` (or equivalent) providing composition and decomposition utilities for Translation–Rotation–Scale (TRS) using row‑major `Mat4` plus `Quat` + `Vec3` primitives.

## Overview

The transform layer converts intuitive authorable components (position, orientation, scale) into a 4×4 affine matrix consumable by the renderer and GPU pipelines. Focus areas:

- **Deterministic ordering**: Always `Mat4 = T * R * S` (row‑major multiplication left→right). Document clearly to avoid ambiguity.
- **Stable decomposition**: Recovers original TRS within epsilon when no shearing and uniform handedness.
- **Minimal allocations**: Pure value operations; return by value relying on NRVO.

## Composition

```cpp
Mat4 world = Transforms::Compose(
    Vec3{tx, ty, tz},         // Translation
    Quat{qx, qy, qz, qw},     // Rotation quaternion (normalized or will be normalized)
    Vec3{sx, sy, sz}          // Scale (non-zero expected)
);
```

### Algorithm Notes

1. Build scale matrix (diagonal).
2. Convert quaternion to 3×3 rotation.
3. Multiply rotation * scale (or scale * rotation depending on chosen ordering – confirm implementation; documented expectation: **scale-then-rotate** so `R * S`).
4. Embed into 4×4 with translation in the last row (row‑major) or last column (column‑major) – current design: translation occupies the last row's first three components if using row‑vector convention. Clarify actual layout in code comments.

> Ensure doc stays synced with any convention adjustments (e.g., migrating to column-major GPU alignment).

## Decomposition

```cpp
Vec3 translation;
Quat rotation;
Vec3 scale;
bool ok = Transforms::Decompose(worldMatrix, translation, rotation, scale);
```

### Failure Modes

| Condition | Result | Suggested Handling |
|-----------|--------|--------------------|
| Near-zero scale component | `ok = false` | Log + fallback to (1,1,1) |
| Non-orthogonal rotation (shear present) | `ok = true` but rotation may need Gram–Schmidt | Optional orthogonalization |
| Non-invertible matrix | `ok = false` | Abort decomposition / assert |

### Numerical Stability

- Normalize quaternion after extraction.
- Length of basis vectors → scale.
- Divide out scale before forming quaternion.
- Use epsilon guard: if axis length < `EPSILON`, treat as zero.

## Handedness & Coordinate System

Current assumption: Right-handed world, Y-up (confirm in engine root docs). Composition routine should not implicitly flip handedness; any mirror (negative scale) introduces potential quaternion sign ambiguity—document and test.

## API Sketch

```cpp
namespace Transforms
{
    Mat4 Compose(const Vec3& translation, const Quat& rotation, const Vec3& scale);
    bool Decompose(const Mat4& m, Vec3& outTranslation, Quat& outRotation, Vec3& outScale);
}
```

Consider overloads for:

- `Compose(translation, EulerAngles, scale)` (lazy quaternion build)
- `ComposeTR(translation, rotation)` (unit scale)
- `ComposeRS(rotation, scale)` (origin translation)

## Edge Cases

| Case | Recommended Behavior |
|------|----------------------|
| Zero scale component | Return identity rotation; set scale to (0,0,0) and translation as extracted |
| Negative scale | Preserve sign in scale vector; rotation remains proper (determinant +1) – absorb reflection into scale only |
| Denormal quaternion input | Re-normalize, log at debug level |
| Shear (non-orthogonal basis) | Option: attempt orthogonalization; expose flag to caller |

## Interop with Matrices

Round trip expectation:

```cpp
Vec3 t0{...}; Quat r0{...}; Vec3 s0{...};
Mat4 m  = Transforms::Compose(t0, r0, s0);
Vec3 t1; Quat r1; Vec3 s1;
Transforms::Decompose(m, t1, r1, s1);
// epsilonEqual(t0,t1) etc.
```

## Performance Guidelines

| Scenario | Recommendation |
|----------|---------------|
| Per-frame camera | Cache composed matrix until change flag flips |
| Static objects | Precompute once; store matrix only |
| Frequent partial updates (scale only) | Recompose full matrix (cheap) vs attempting in-place patch |
| Many decompositions | Avoid unless required (e.g., importing external baked matrices) |

## Suggested Extensions

| Feature | Priority | Rationale |
|---------|----------|-----------|
| `ComposeAffineFast` (assumes unit scale) | High | Common for rigid bodies |
| Dual quaternion support | Medium | Smoother blending for animation |
| Shear support in decomposition | Medium | Importing DCC content |
| SIMD quaternion->matrix path | Medium | Hot in animation update |
| Batch compose (SoA inputs) | High | ECS friendly, vectorizable |

## Testing Strategy

- **Identity**: Compose(0, identityQuat, 1) == Identity matrix.
- **Round Trip**: Random TRS (bounded) → compose→decompose within epsilon.
- **Negative Scale**: Validate reflection captured only in scale.
- **Degenerate Scale**: scale = (0,0,0) decomposes predictably with flag false.
- **Quaternion Normalization**: Feed in intentionally unnormalized quaternion—output quaternion normalized.

## Logging & Assertions

Use tagged logging `MATH`. Examples:

```cpp
SEDX_CORE_WARN_TAG("MATH", "Decompose: near-zero scale component (len={}), applying fallback", len);
SEDX_CORE_ASSERT(valid, "Compose received NaN component");
```

## TODO / Backlog

- Confirm and lock documented memory layout (row vs column commentary) to remove ambiguity.
- Provide micro-benchmarks for batch compose vs individual calls.
- Add fast path for uniform scale (skip scale matrix multiply).
- Document quaternion handedness & winding with camera extraction.

---

*Generated documentation file; keep synchronized with implementation.*
