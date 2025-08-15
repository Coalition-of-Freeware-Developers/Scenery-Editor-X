# Math Library – Scale Utilities

Documentation for scaling helpers in `scale.h` supporting 3D / 2D scaling matrices and vector length normalization by target magnitude.

## Overview

Provide small stateless constructors for scale matrices (Mat4 for 3D, Mat3 homogeneous for 2D) plus a utility to resize a vector to a desired length without invoking full transform systems.

## Functions Summary

| Function | Purpose | Notes |
|----------|---------|-------|
| `Scale(Vec3 s)` | Build 4×4 non-uniform 3D scale | Row-major diagonal with (s.x,s.y,s.z,1) |
| `Scale(Vec2 s)` | Build 4×4 planar scale | Z scale fixed 1.0 |
| `Scale(m, Vec3 s)` | Apply 3D scale to Mat4 | Post multiply `m * S` |
| `Scale(m, Vec2 s)` | Apply planar scale to Mat4 | Only X/Y scaled |
| `Scale2D(Vec2 s)` | Build 3×3 homogeneous scale | For 2D pipelines |
| `Scale2D(m, Vec2 s)` | Apply 2D scale to Mat3 | `m * S2D` |
| `Scale(Vec3 v, float len)` | Return vector scaled to length | Safe zero vector guard |

## Matrix Layout

Row‑major scale matrix (3D):

```text
[ sx  0  0  0 ]
[  0 sy  0  0 ]
[  0  0 sz  0 ]
[  0  0  0  1 ]
```

2D homogeneous (Mat3):

```text
[ sx  0  0 ]
[  0 sy  0 ]
[  0  0  1 ]
```

## Vector Length Scaling Helper

```cpp
Vec3 v(3,4,0);               // length 5
Vec3 unit = Scale(v, 1.0f);  // (0.6,0.8,0)
Vec3 five = Scale(unit, 5.0f); // back to (3,4,0)
```

| Behavior | Detail |
|----------|--------|
| Zero Guard | If magnitude ~ 0 (epsilonEqual) → returns (0,0,0) |
| Precision | Uses sqrt for magnitude (improvement: fast inverse sqrt path optional) |
| Stability | Relies on `epsilon<T>()` from constants system |

## Usage Examples

### Combine TRS

```cpp
Mat4 model = Translate(pos) * RotateEulerRadians(rot) * Scale(scaleVec);
```

### Apply Additional Planar Scale Non-Uniformly

```cpp
Mat4 sprite = Scale(Vec2(2.0f, 1.0f));
```

### Normalize Then Re-Lengthen

```cpp
Vec3 direction = Scale(rawVector, 1.0f);
Vec3 velocity   = direction * speed;
```

## Performance Notes

| Aspect | Note |
|--------|------|
| Inline | All helpers are `inline` header-only |
| Branching | Only vector length function branches on near-zero magnitude |
| Multiplies | Matrix apply variant incurs full Mat4×Mat4 multiply – fold with other TRS if possible |
| Allocation | None – pure value construction |

## Pitfalls

| Issue | Cause | Mitigation |
|-------|-------|-----------|
| Distorted normals | Non-uniform scale without normal matrix fix | Recompute normal matrix or use inverse transpose |
| Unintended planar scale | Confusing `Scale(Vec2)` vs `Scale(Vec3)` | Prefer explicit `Vec3(sx,sy,1.0f)` for clarity |
| Zero vector normalization | Input length ~0 | Function returns zero; handle upstream if alternate fallback needed |

## Extension Backlog

| Feature | Priority | Rationale |
|---------|----------|-----------|
| Fast inverse sqrt path | Low | Micro-opt for heavy normalization loops |
| SIMD batch normalize | Medium | Physics / particle systems |
| Uniform scale shorthand `Scale(float s)` | Low | Convenience |
| Mat3 2D scale apply overload for Mat4? | Low | Possibly remove to avoid ambiguity |

## Testing Strategy

- **Matrix Diagonal**: Off-diagonals zero, last row `[0 0 0 1]` for Mat4.
- **Planar**: `Scale(Vec2)` leaves Z scale = 1.
- **Length Scaling**: `length(Scale(v,L)) ≈ L` within epsilon.
- **Zero Vector**: Returns zero vector (no NaN).

---

*Generated documentation file for scale utilities.*
