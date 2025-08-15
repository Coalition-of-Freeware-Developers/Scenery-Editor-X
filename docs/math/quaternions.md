# Math Library – Quaternions

Covers `quat.h` providing quaternion representation for 3D rotations plus helpers for conversion and interpolation.

## Overview

Quaternion (x, y, z, w) stored as four contiguous floats; identity = (0,0,0,1). Used for stable rotation accumulation, interpolation, and conversion to rotation matrices.

| Feature | Status | Notes |
|---------|--------|-------|
| Identity construction | ✓ | Default ctor sets identity |
| Normalization | TODO | Provide `Normalize(q)` / `Normalized(q)` |
| Length / LengthSq | TODO | Needed for defensive checks |
| Conjugate / Inverse | TODO | Used for vector rotation & relative rot |
| Multiply (composition) | TODO | Define consistent operand order |
| Slerp / Nlerp | TODO | Animation blending |
| FromAxisAngle / ToAxisAngle | TODO | Ensure axis normalization |
| FromEuler / ToEuler | TODO | Document intrinsic order (e.g., XYZ) |
| ToMat3 / ToMat4 | Planned | Row‑major matrix extraction |

## Memory & Formatting

Trivial POD style; supply `{fmt}` formatter + stream operator for logging.

```cpp
Quat q; // identity
SEDX_CORE_TRACE_TAG("MATH", "Quat: {}", q);
```

## Construction Patterns

```cpp
Quat identity;                      // (0,0,0,1)
Quat axisAngle = Quat::FromAxisAngle(Vec3{0,1,0}, Radians(90.f));
Quat euler    = Quat::FromEulerXYZ(pitch, yaw, roll); // Confirm order naming
```

## Rotation Application

(Planned) Free function for rotating a vector:

```cpp
Vec3 Rotate(const Quat& q, const Vec3& v);
```

Implementation sketch:

1. Convert to 3×3 OR
2. Use q * (0,v) * q^(-1) expansion (fewer temporaries if optimized)

## Interpolation

| Method | Use Case | Pros | Cons |
|--------|----------|------|------|
| Nlerp  | Camera orientation, cheap blends | Fast, constant speed approx for small angles | Not constant angular velocity |
| Slerp  | Large-angle animation, spin | True constant angular velocity | Slightly more expensive |

Edge: If dot < 0, negate one operand to pick shortest path.

## Normalization Rules

- Auto-normalize on composition? (Prefer explicit: do not hide cost.)
- Provide `IsNormalized(q, eps)` utility.
- After accumulation loops (e.g., integrating angular velocity) renormalize occasionally.

## Euler Angle Conventions

Specify and lock intrinsic order (e.g., XYZ). Provide explicit variant functions if multiple orders needed. Document degrees vs radians—library core uses radians.

## Conversion to Matrix

```cpp
Mat3 r3 = ToMat3(q);
Mat4 r4 = ToMat4(q); // Upper-left 3×3, last row/column identity as appropriate
```

Consider caching path if repeated conversions appear in hot code (e.g., animation system).

## Relative Rotations

```cpp
Quat qRelative = qB * Conjugate(qA); // Apply A⁻¹ then B
```

Define clearly so caller expectations match engine math (update docs if operand order changes in implementation).

## Suggested API Surface

```cpp
struct Quat
{
    float x, y, z, w;
    static Quat Identity();
    static Quat FromAxisAngle(const Vec3& axis, float angleRad);
    static Quat FromEulerXYZ(float pitch, float yaw, float roll);
};

float Length(const Quat& q);
float LengthSq(const Quat& q);
Quat  Normalize(const Quat& q);
Quat  Conjugate(const Quat& q);
Quat  Inverse(const Quat& q);  // Conjugate / |q|^2
Quat  Multiply(const Quat& a, const Quat& b); // Rotation composition
Quat  Slerp(const Quat& a, const Quat& b, float t);
Quat  Nlerp(const Quat& a, const Quat& b, float t);
```

## Testing Strategy

- **Identity**: Multiplying identity preserves operand.
- **Axis-Angle Round Trip**: FromAxisAngle→ToAxisAngle within epsilon.
- **Normalization**: Random quaternion → Normalize → Length ≈ 1.
- **Slerp Endpoints**: t=0 returns a; t=1 returns b.
- **Shortest Path**: Opposite quats produce expected midpoint.

## Performance Notes

- Favor nlerp for per-frame camera smoothing; slerp for animation key interpolation.
- Consider approximate slerp for small angles (fallback to nlerp).

## Logging & Assertions

Use `SEDX_CORE_ASSERT(!IsNaN(q), "Invalid quaternion")` style checks in debug-only paths. Log anomalies:

```cpp
SEDX_CORE_WARN_TAG("MATH", "Slerp inputs nearly antiparallel (dot={})", dot);
```

## TODO / Backlog

- Complete base operations (normalize, conjugate, multiply).
- Implement slerp with robust dot thresholding.
- Provide quaternion <-> direction vector (look rotation) helper.
- Add dual quaternion extension notes.

---

*Generated documentation file; align with code updates.*
