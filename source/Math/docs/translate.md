# Math Library – Translation Utilities

Documents free translation helpers in `translate.h` for 2D (homogeneous Mat3) and 3D (Mat4) translation matrix construction and application.

## Scope

Lightweight creation / composition of translation matrices to avoid full transform struct overhead when only offset needed.

## Provided Functions

| Function | Dim | Purpose | Notes |
|----------|-----|---------|-------|
| `Translate(t)` | 3D | Build 4×4 translation matrix | Row-major: components in last column |
| `Translate(m, t)` | 3D | Post-multiply with translation | `m * T` |
| `Translate2D(t)` | 2D | Build 3×3 homogeneous translation | Last column stores offsets (row-major) |
| `Translate2D(m, t)` | 2D | Apply translation to Mat3 | `m * T2D` |

## Matrix Layout

Row‑major convention; translation column index 3 for Mat4, index 2 for Mat3 (homogeneous):

```text
[ 1 0 0 tx ]
[ 0 1 0 ty ]
[ 0 0 1 tz ]
[ 0 0 0  1 ]
```

2D homogeneous:

```text
[ 1 0 tx ]
[ 0 1 ty ]
[ 0 0  1 ]
```

## Usage

### 3D Compose TRS

```cpp
Mat4 m = Translate(position) * RotateAxisAngleRadians(axis, angle) * Scale(scaleVec);
```

### Incremental Accumulation

```cpp
Mat4 model = Mat4::Identity();
for (const Vec3& step : path)
    model = Translate(model, step);
```

### 2D UI Offset

```cpp
Mat3 ui = Translate2D(Vec2(12.0f, 24.0f));
```

## Performance Notes

| Aspect | Note |
|--------|------|
| Branching | None; pure constructor functions |
| Memory | Small stack allocation; relies on aggregate list initialization |
| Composition Cost | One matrix multiply per apply variant (`m * T`) |
| Inline | Marked `inline` → header-only |

## Pitfalls

| Issue | Cause | Mitigation |
|-------|-------|-----------|
| Reversed multiply order | Assuming pre-multiply semantics | Follow documented `m * T` row-major pattern |
| Mixing row/column major | Using external libs (e.g., column-major GLM) | Confirm layout or supply wrappers |
| Accumulating drift | Large step counts with float precision | Periodically re-normalize orthonormal basis if rotation introduced elsewhere |

## Extension Backlog

| Feature | Priority | Rationale |
|---------|----------|-----------|
| `TranslateX/Y/Z` helpers | Low | Micro convenience |
| SIMD batch translate | Low | Bulk sprite transforms |
| Span-based multi-compose | Medium | Reduce multiply count for paths |

## Testing

- **Identity**: `Translate(Vec3(0))` equals identity matrix.
- **Round Trip**: Applying translation then inverse (negate t) returns original point.
- **Composition Order**: `Translate(a) * Translate(b)` equals single translate of `a + b`.

---

*Generated documentation file for translation utilities.*
