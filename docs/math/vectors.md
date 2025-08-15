# Math Library – Vectors

Covers `vec2.h`, `vec3.h`, `vec4.h`, and the umbrella `vector.h`.

## Overview

The vector subsystem supplies lightweight POD templates `TVector2<T>`, `TVector3<T>`, `TVector4<T>` (namespace `SceneryEditorX::Utils`) plus engine typedefs:

| Alias | Underlying Template | Type Parameter | Header | Intent |
|-------|---------------------|----------------|--------|--------|
| `Vec2` | `TVector2<float>` | `float` | `vector.h` | 2D positions, UVs, colors (RG) |
| `Vec3` | `TVector3<float>` | `float` | `vector.h` | 3D positions, directions, RGB |
| `Vec4` | `TVector4<float>` | `float` | `vector.h` | Homogeneous coord, RGBA, packed data |

All vector templates provide semantic unions for multiple domains (`x/y/z/w`, `r/g/b/a`, `s/t/p/q`) without extra memory.

## Storage & ABI

- Contiguous scalar members in declared order (no padding in current definitions).
- Trivially copyable & relocatable (safe for binary serialization / memcpy / GPU upload after packing rules).

## Constructors (constexpr)

| Signature | Description |
|-----------|-------------|
| `TVectorN()` | Zero-initialize (`0` for all components) except `Vec4` also zeroes w. |
| `TVectorN(T s)` | Broadcast scalar to all components. |
| `TVector2(T x, T y)` | Component explicit ctor. |
| `TVector3(T x, T y, T z)` | Component explicit ctor. |
| `TVector4(T x, T y, T z, T w)` | Component explicit ctor. |
| `TVector4(const TVector3<T>& v, T w)` | Promote 3D vector to 4D with explicit w. |

## Arithmetic Operators (Declared)

> Bodies omitted in header excerpts (`{…}`); semantics are standard component‑wise operations. All return new vectors and are `constexpr` where possible.

| Operator | Vec2 | Vec3 | Vec4 | Semantics |
|----------|------|------|------|-----------|
| `+` | ✓ | ✓ | ✓ | Component-wise add |
| `-` | ✓ | (likely, TBD final header) | (likely, TBD) | Component-wise subtract |
| `* (scalar)` | ✓ | (likely) | (likely) | Multiply by scalar |
| `/ (scalar)` | ✓ | (likely) | (likely) | Divide by scalar |
| `scalar * vector` | ✓ (free) | ✓ (free template) | (expected) | Commutative scalar multiply |

### Equality / Relational

Not explicitly shown; recommended future additions (exact vs epsilon versions). Use `epsilonEqual(a.x, b.x)` per component until provided.

## Common Patterns

```cpp
Vec3 p{1.0f, 2.0f, 3.0f};
Vec3 d = 2.0f * p + Vec3{0.5f};
```

## Extension Recommendations

| Feature | Rationale | Status |
|---------|-----------|--------|
| Length / LengthSquared | Avoid sqrt when possible | Add to `math_utils` or free funcs |
| Normalize / SafeNormalize | Prevent divide-by-zero | Pending |
| Dot (Vec3) / Cross | 3D geometry ops | Partially: dot Vec2/Vec4 implemented in `dot.h`; extend |
| Min/Max/Clamp | AABB & color ops | Pending |
| Lerp | Animation | Should call `math_utils` once available |

## Interop Notes

- Row-major matrices expect vectors multiplied on the right (when implementing `Mat * Vec` later—document consistently).
- For column-major shader code, upload transposed matrices; vectors remain unchanged.

## Performance Guidelines

- Pass small vectors by value (fits in registers). Avoid heap allocations.
- Avoid normalizing every frame if values unchanged; cache normalized direction.

## Error Handling

Vectors are plain data containers—no internal assertions. Use `SEDX_CORE_ASSERT` in higher-level functions (e.g., when normalizing zero-length vector).

## TODO / Backlog

- Implement missing operators explicitly (list in code comments).
- Provide constexpr conversion utilities (e.g., to/from array span).
- Add hashing (for map/set usage) once stable API finalized.

---
*File generated for documentation; update when implementations are completed.*
