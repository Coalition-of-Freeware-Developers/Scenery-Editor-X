# Math Library – Rotation Utilities

Covers free rotation helpers declared in `rotation.h`: axis-specific degree helpers, Euler (YXZ) builders, axis-angle rotation, matrix apply variants, and GLM compatibility shims.

## Scope & Goals

Provide lightweight, explicit constructors for rotation matrices without pulling a full quaternion or transform stack when only a single rotation composition is needed.

## Provided Functions (Summary)

| Function | Angle Units | Purpose | Notes |
|----------|------------|---------|-------|
| `RotateZDegrees(deg)` | Degrees | Build Z-axis rotation | Right-handed CCW looking down +Z |
| `RotateZDegrees(m, deg)` | Degrees | Post-multiply `m` with Z-rot | Returns `m * Rz` (row-major assumption) |
| `RotateEulerRadians(euler)` | Radians | Build Y-X-Z ordered rotation | Order: yaw(Y), pitch(X), roll(Z) |
| `RotateEulerDegrees(eulerDeg)` | Degrees | Degrees overload (build) | Converts to radians internally (expected) |
| `RotateEulerRadians(m, euler)` | Radians | Apply YXZ rotation to matrix | `m * R(euler)` |
| `RotateEulerDegrees(m, eulerDeg)` | Degrees | Degree variant apply | Same order YXZ |
| `RotateAxisAngleRadians(axis, angle)` | Radians | Axis-angle rotation | Assumes normalized axis (caller responsible) |
| `RotateAxisAngleRadians(m, axis, angle)` | Radians | Apply axis-angle to matrix | `m * R(axis,angle)` |
| `glm::rotate(angle, axis)` | Radians | GLM-style free build | Shim for compatibility |
| `glm::rotate(m, angle, axis)` | Radians | GLM-style apply | Shim (post-multiply) |

## Conventions

- Right-handed coordinate system.
- Row-major matrices (rotation resides in upper 3×3 portion, last row `[0 0 0 1]`).
- Degree helpers suffixed with `Degrees`; radian versions use *Radians* or omit explicit suffix in per-axis API (currently only Z axis provided as degree variant in header snippet). Future consistency improvement: consider radian-only core plus thin degree wrappers.
- Euler order: **YXZ** (Yaw → Pitch → Roll) – chosen to reduce gimbal issues for typical camera style movement (yaw first about world Y, then pitch about local X, then roll).

## Detailed Behavior

### Z-Axis Degree Rotation

```cpp
Mat4 rz = RotateZDegrees(90.0f); // +90° CCW around +Z
Vec3 v(1,0,0);
Vec3 v2 = (rz * Vec4(v,1.0f)).xyz(); // Expect (0,1,0)
```text

### Euler YXZ (Radians)

Application order when post-multiplying a row‑major matrix `M`:

```text
M' = M * R_y(yaw) * R_x(pitch) * R_z(roll)
```

This yields global yaw, then local pitch, then local roll.

### Axis-Angle

Standard Rodrigues form:

```
R = I * c + (1-c) * (axis ⊗ axis) + [axis]_x * s
```

where `c = cos(θ)`, `s = sin(θ)`. Axis must be normalized (library does **not** auto-normalize to keep predictable performance; caller should ensure normalization or risk unintended scaling/shear).

## Usage Examples

### Compose TRS Manually (Rotation First)

```cpp
Mat4 r = RotateEulerRadians(Vec3(pitch, yaw, roll)); // YXZ order
Mat4 t = Translate(position);
Mat4 s = Scale(scaleVec);
Mat4 model = t * r * s; // Consistent with transforms doc
```

### Camera Yaw / Pitch Increment (Degrees Input)

```cpp
float yawDeg   = 15.0f;
float pitchDeg = -5.0f;
Mat4 cameraRot = RotateEulerDegrees(Vec3(pitchDeg, yawDeg, 0.0f));
```

### Align Around Arbitrary Axis

```cpp
Vec3 axis = Normalize(Vec3(0.3f, 1.0f, 0.0f));
Mat4 spin = RotateAxisAngleRadians(axis, 2.0f); // ~114.59°
```

## Interop (GLM)

The shim functions replicate GLM signatures allowing drop‑in replacement for limited rotate usage without migrating entire code paths.

| GLM Signature | Provided Equivalent | Remark |
|---------------|---------------------|--------|
| `glm::rotate(mat, angle, axis)` | `glm::rotate(mat, angle, axis)` | Post-multiply axis-angle |
| `glm::rotate(angle, axis)` | `glm::rotate(angle, axis)` | Standalone builder |

If broader GLM removal is planned, ensure remaining dependent call sites route through these shims then gradually refactor to direct `RotateAxisAngleRadians`.

## Numerical & Precision Notes

- For very small angles (< 1e-6), consider early-out returning identity to avoid precision loss (not yet implemented – potential optimization).
- Ensure axis normalization: `length(axis)` close to 1; if not, rotation matrix acquires scaling components.
- Combining many small rotations may accumulate drift; prefer quaternion accumulation then convert to matrix if stability issues arise.

## Performance Considerations

| Aspect | Note |
|--------|------|
| Branching | All builders are branchless except potential axis normalization (caller side). |
| Trig Calls | Euler and axis-angle each invoke multiple `sin/cos`; batch with `sinf/cosf` pair or use `sincosf` where available. |
| Composition | Post-multiplication pattern consistent across API for cache predictability. |
| Normalization Cost | Pushing responsibility to caller avoids hidden sqrt in hot loops. |

## Common Pitfalls

| Issue | Cause | Mitigation |
|-------|-------|-----------|
| Unexpected scaling in rotation matrix | Non-normalized axis | Normalize before call |
| Gimbal lock near ±90° pitch | Euler order limitation | Use quaternions for camera beyond typical pitch ranges |
| Orientation drift over frames | Incremental Euler accumulation | Switch to quaternion integrate → convert |
| Degree / radian mixups | Passing degrees into radian-only API | Use explicit `Degrees` overloads or conversion helpers |

## Extension Backlog

| Feature | Priority | Rationale |
|---------|----------|-----------|
| `RotateXDegrees / RotateYDegrees` | Medium | Completeness & consistency |
| Unified degree wrappers (template tag) | Low | Reduce name proliferation |
| Simd axis-angle builder | Low | Batch bone rotations |
| Quaternion helper integration | Medium | Convert quaternion to Mat3/Mat4 directly |
| Small-angle approximation path | Low | Micro-opt in IK / physics loops |

## Testing Strategy

- **Axis Consistency**: Rotate basis vectors and verify expected permutation signs.
- **Euler Composition**: Compare against quaternion-built rotation for random angles (tolerant within epsilon).
- **Axis-Angle**: For axis = (1,0,0), compare with dedicated X rotation to ensure equivalence.
- **GLM Shim Parity**: If GLM present in test harness, assert matrices are element-wise equal.

## Documentation Notes / Discrepancies

Header snippet shows some doc comments referencing *radians* while the function names / params labeled *Degrees* – confirm and align (either adjust implementation to convert degrees to radians or rename). Track in backlog.

---

*Generated documentation file for rotation utilities; refine after implementation details finalize.*
