# Scenery Editor X Math Library – GitHub Copilot Instructions (2025-08 Refresh)

> Purpose: Guide Copilot (and contributors) in using, extending, and fixing the math subsystem now located under `source/Math` (previously `source/SceneryEditorX/utils/math`). This refresh synchronizes the instruction set with the current implemented code (notably a substantially featured `Mat4`) and clarifies conventions where legacy draft docs drifted from actual implementation.

> IMPORTANT: Some earlier draft docs described a left→right "apply A then B" multiplication semantics for row‑major matrices. The **current shipped `Mat4` implementation (see `mat4.h`) stores translation in the last column and documents that in `lhs * rhs`, the `rhs` transform is applied first. This reflects a column‑vector conceptual order (even though storage is row‑major). Until an intentional refactor unifies wording and storage semantics, **treat multiplication as: `result = A * B` applies B, then A.** Keep docs consistent with code to avoid subtle transform bugs.

## 1. Core Principles (Validated / Updated)

1. Lightweight POD types (vectors, matrices, quaternions) with explicit public members – no hidden storage, no dynamic allocation.
2. Row‑major storage (`Mat2`, `Mat3`, `Mat4`). Current operational semantics: `M = A * B` applies **B first, then A** (column‑vector style). Translation lives in the last column (`m03,m13,m23`). When adding examples, reflect this to avoid confusion. (Open action: optionally introduce explicit helpers like `ComposeTRSRowMajorLeftToRight` or rename docs after any future unification.)
3. Deterministic binary layout (safe for memcpy, serialization, uniform uploads after appropriate packing or transpose if GPU expects column‑major).
4. Prefer free functions over member overloads when ambiguous; keep APIs minimal and explicit.
5. Avoid implicit conversions or silent unit changes (degrees vs radians must be explicit / documented).
6. Use engine constants (`PI`, `TWO_PI`, `DEG_TO_RAD`) instead of hard‑coded numbers.
7. Use `epsilon<T>()`, `epsilonEqual()` for tolerant float comparisons; never raw `==` on floats in algorithms needing robustness.
8. Don’t add GLM includes or mimic GLM naming; this is an internal math layer.

## 2. Type Overview & Layout (Current State)

| Type | File | Notes |
|------|------|-------|
| `Vec2` / `Vec3` / `Vec4` | `vector.h` + `vec*.h` | POD wrappers (`TVector*<float>` aliases). Multiple semantic unions (`x/y/z/w`, `r/g/b/a`, `s/t/p/q`). |
| `Mat2` | `mat2.h` | Row‑major 2×2, fields: `m00,m01,m10,m11`. |
| `Mat3` | `mat3.h` | Row‑major 3×3, fields contiguous `m00..m22`. |
| `Mat4` | `mat4.h` | Row‑major 4×4 (16 floats). Implements: rich constructors, Perspective/Ortho/LookAt, arithmetic, multiply, transpose, inverse (analytical), NearlyEqual, data access. |
| `Quat` | `quat.h` | (x,y,z,w), identity = (0,0,0,1). Rotation representation – avoid gimbal issues. |
| `AABB` | `aabb.h` (skeleton / TODO) | Axis-aligned box with `min`/`max` (Vec3). Invariant: `min <= max` component-wise. Implementation still pending. |
| Colors (`Color*`) | `colors.h` / `colors.cpp` | Utility conversion & linear/sRGB helpers (WIP). |
| Gradient | `gradients.h/.cpp` | Keyed color interpolation; maintain sorted keys. |
| Projection builders | `projection.h` | Perspective / ortho (row‑major). |
| Transform helpers | `transforms.h/.cpp` | `Compose` / `Decompose` TRS using `Mat4`, `Quat`, `Vec3`. |
| Rotation helpers | `rotation.h` | Axis / Euler builder/apply variants (YXZ order for Euler – document!). |
| Translation helpers | `translate.h` | Build / post-multiply translation matrices. |
| Scale helpers | `scale.h` | 2D and 3D scale matrix builders + vector length normalization. |
| Constants | `constants.h` | Implemented: `PI`, `TWO_PI`, `HALF_PI`, `DEG_TO_RAD`, `RAD_TO_DEG`, `INV_PI`, `INV_TWO_PI`, `PI_OVER_4`, `PI_OVER_6`, `EPSILON_F`, etc. |
| Epsilon | `epsilon.h` | `epsilon<T>()`, `epsilonEqual()`. Extend with vector overloads if needed. |
| Dot | `dot.h` | Vec2 / Vec4 implemented; Vec3 still missing (add soon). |

### Path / Include Updates
The canonical include root has shifted to `#include <Math/...>` (e.g. `#include <Math/includes/mat4.h>`). Update legacy include paths in new code accordingly.

### Memory / ABI Rules
* Do not reorder public members – downstream serialization may depend on order.
* Do not add virtual methods or non-trivial destructors; keep trivially copyable.
* When adding operators, mark them `constexpr` and `noexcept` where defensible.

## 3. Naming & Style Enforcement
Follow repository coding style: tabs for indentation, Allman braces, max line length 120, Doxygen comments for public APIs. Right-align pointer stars to variable name (e.g., `float *ptr`). All inline math functions should live in headers with `inline` or `constexpr` to avoid ODR issues.

## 4. Vectors – Implemented vs Missing
`TVector3<T>` currently implements: constructors, semantic unions, arithmetic (+ - * / with scalar), compound assignments, unary minus, equality (exact), indexing, and free scalar*vector overload. **Still missing** common geometric helpers (Length, Normalize, Dot for Vec3, Cross, Min/Max/Clamp, epsilon-aware equality). Avoid re‑implementing per call site; add canonical helpers.

Recommended minimal operator set per `TVectorN<T>`:
```cpp
constexpr TVector3<T> operator+(const TVector3<T>& rhs) const noexcept;
constexpr TVector3<T> operator+(const TVector3<T>& a, const TVector3<T>& b) noexcept; // free
constexpr TVector3<T> operator-(...) noexcept;
constexpr TVector3<T> operator*(const TVector3<T>& v, T s) noexcept;
constexpr TVector3<T> operator/(const TVector3<T>& v, T s) noexcept; // assert s != 0 in debug
constexpr T LengthSq(const TVector3<T>& v) noexcept; // v.x*v.x + ...
inline T Length(const TVector3<T>& v) noexcept { return std::sqrt(LengthSq(v)); }
inline TVector3<T> Normalized(const TVector3<T>& v) noexcept { T len = Length(v); return len > epsilon<T>() ? v / len : TVector3<T>{0}; }
```
* Guard divide-by-zero with epsilon in new division helpers (current raw `/` does not check – callers must ensure non-zero scalar).
* Scalar * vector free overload exists for `TVector3`; replicate for other dimensions when implementing their arithmetic.
* When adding `Length` / `Normalize`, prefer free functions in a dedicated header (or extend `math_utils`) to keep POD vector lean.

## 5. Matrices – Status & Guidance
`Mat4` already offers: identity (diagonal ctor), zero, translation, scale (2D & 3D), Euler (degrees & radians), Z‑axis angle helper, perspective, orthographic (two variants), look-at, arithmetic (+ - * / with scalar & matrix), Multiply static helpers, transpose (in-place and free), inverse (analytical), determinant/adjoint infrastructure, epsilon comparison (`NearlyEqual`), pointer access, and an `fmt` formatter (when available).

Pending for smaller matrices (`Mat2`, `Mat3`) and cross‑type transform helpers:
* Implement `Mat3`/`Mat2` parity (identity, multiply, inverse, transpose) – ensure naming matches `Mat4` style.
* Add `TransformPoint/TransformVector` free functions (with explicit w=1 or 0) once vector-matrix multiply conventions are finalized.
```cpp
inline Vec3 TransformPoint(const Mat4& m, const Vec3& p);
inline Vec3 TransformVector(const Mat4& m, const Vec3& v);
```
* Prefer specialized affine inverse path (rotation+translation+scale) over full analytical inverse in hot code; current implementation uses general path — mark a TODO if profiling flags a hotspot.

## 6. Quaternion Usage (Still Needed)
* Identity default constructed.
* Provide: `Length()`, `LengthSq()`, `Normalize(q)`, `Normalized(q)`, `Conjugate(q)`, `Inverse(q)`, `Dot(q1,q2)`, `Slerp(q1,q2,t)` (shortest path, clamp dot). Mark TODO in code where not yet implemented.
* Multiplication order MUST align with current matrix semantics: with `result = a * b` the **b** rotation occurs first, then **a**. Document clearly when implementing to avoid contradictions.
* Provide conversion: `Mat3 ToMat3(const Quat&)`, `Mat4 ToMat4(const Quat&)` (embedding into upper-left 3×3). Keep row-major orientation consistent.
* Euler conversions must document order (e.g., intrinsic Y (yaw), X (pitch), Z (roll) – YXZ).

## 7. Transform Composition / Decomposition
Target design remains logical TRS composition. Given current column‑vector style application order, building a world matrix as `M = T * R * S` means a point `v` experiences scale, then rotation, then translation (desired). Preserve this when implementing `Transforms::Compose`.
```cpp
Mat4 compose = TranslationMatrix(translation) * RotationMatrix(rotation) * ScaleMatrix(scale);
```
* DO NOT multiply in reverse or silently optimize away order.
* `Decompose` should: extract translation (last column in current layout), derive scale as length of basis vectors, normalize rotation axes, build quaternion. Use epsilon to guard near-zero scale.
* Always verify `Decompose(Compose(t,q,s))` reconstructs components within tolerance.

## 8. Projection Builders (Planned vs Existing)
Current `Mat4` includes static builders `PerspectiveProjection` / `OrthographicProjection` (naming differs from earlier MakeX plan). A unified projection API in `projection.h` is still desirable for consistency & reverse/infinite variants. When adding:
Functions (planned / to complete) in `projection.h`:
```cpp
Mat4 MakePerspective(float fovYRadians, float aspect, float zNear, float zFar);
Mat4 MakePerspectiveInfinite(float fovYRadians, float aspect, float zNear);
Mat4 MakePerspectiveReverseZ(float fovYRadians, float aspect, float zNear, float zFar);
Mat4 MakeOrthographic(float left,float right,float bottom,float top,float zNear,float zFar);
Mat4 MakeOrthographicCentered(float width,float height,float zNear,float zFar);
```
Guidelines:
* Validate inputs: `aspect > 0`, `zNear > 0`, `zFar > zNear` (unless reverse-Z variant).
* Reverse-Z swaps mapping so far plane becomes greater precision region near camera; set depth mapping accordingly (1 at near vs default). Ensure renderer depth compare flips (`GREATER`) and clear depth = 0.
* Keep handedness consistent (right-handed; confirm with engine conventions before finalizing sign of Z terms).

## 9. Translation / Rotation / Scale Helpers
Files: `translate.h`, `rotation.h`, `scale.h`.
Patterns:
* Provide pure constructors `Translate(Vec3 t)`, `RotateZDegrees(float deg)`, `Scale(Vec3 s)` returning new `Mat4` without modifying globals.
* Provide post-multiply variants: `RotateZDegrees(const Mat4& m, float deg)` returns `m * Rz` (explicit row-major doc string).
* Euler: Current `Mat4::RotationDegrees/RotationRadians` implement **Y (yaw), X (pitch), Z (roll)** order (YXZ). Maintain consistency across free helpers and quaternion conversions.
* Scale2D / Translate2D operate on `Mat3` homogeneous coordinates; keep final row `[0 0 1]`.

## 10. AABB Utilities (Implementation Guidance – Still Missing)
Add to `aabb.h/.cpp` (create `.cpp` if needed):
```cpp
struct AABB {
    Vec3 min{ std::numeric_limits<float>::max() };
    Vec3 max{ -std::numeric_limits<float>::max() };
    // This “inverted” default allows incremental expansion.
    AABB() = default;
    AABB(const Vec3& a, const Vec3& b) { min = { std::min(a.x,b.x), ... }; max = { std::max(a.x,b.x), ... }; }
    void Reset(); // restore inverted state
    void Expand(const Vec3& p);
    void Expand(const AABB& other);
    Vec3 Center() const; // 0.5f*(min+max)
    Vec3 Extents() const; // max-min
    bool IsValid() const; // min.x <= max.x etc.
    bool Contains(const Vec3& p) const;
    bool Intersects(const AABB& b) const;
};
```
* Avoid NaN propagation; if any component becomes NaN during expansion, assert in debug.

## 11. Colors & Gradients (WIP)
* Color types should clearly distinguish byte vs float (naming suffix: `RGBA8` vs `RGBAF`).
* Provide `FromBytes`, `ToBytes`, `FromSRGBBytes`, `ToSRGBBytes`. Use standard gamma 2.2 (or 2.4 if specified) – document chosen curve.
* Gradient: always keep keys sorted (`position` 0..1). When adding a key, insert then stable sort or insert in order; clamp evaluation input.
* Implement interpolation with linear blend; for future perceptual correctness consider gamma-correct lerp (document if changed).

## 12. Epsilon & Comparisons
* Use `epsilonEqual(a,b)` for scalar; add vector overloads when needed:
```cpp
inline bool epsilonEqual(const Vec3& a,const Vec3& b,float eps=epsilon<float>()) {
    return epsilonEqual(a.x,b.x,eps) && epsilonEqual(a.y,b.y,eps) && epsilonEqual(a.z,b.z,eps);
}
```
* Avoid using epsilon for values that can legitimately be large magnitude – prefer relative epsilon when comparing scaled results. (Potential future utility: `relativeEqual(a,b,eps)`.)

## 13. Performance / Profiling
* Mark small functions `constexpr` or `inline` to enable inlining.
* Avoid temporaries in matrix multiply; unroll loops explicitly for fixed 4×4 to help compiler.
* Add `SEDX_PROFILE_SCOPE` only in higher-level composition paths (`Transforms::Compose`, projection builders) – not inside trivial per-element operations to prevent overhead pollution.
* When normalizing repeatedly inside loops, cache inverse length if reused.

## 14. Error Handling & Assertions
Use `SEDX_CORE_ASSERT` / `SEDX_ASSERT` for developer errors (e.g., invalid aspect ratio, negative scale if disallowed). Use `epsilon()` guard to avoid division by near-zero. For release resilience, degrade gracefully (return identity on invalid projection parameters after logging an error, for example).

## 15. Example Usage Patterns
### Compose TRS
```cpp
Vec3 pos{10.0f, 2.0f, -5.0f};
Quat rot = /* build from axis-angle */;
Vec3 scl{1.0f, 2.0f, 1.0f};
Mat4 world = Transforms::Compose(pos, rot, scl);
```
### Apply Additional Rotation
```cpp
world = RotateZDegrees(world, 45.0f); // world = world * Rz
```
### Perspective Projection
```cpp
Mat4 proj = MakePerspective(60.0f * DEG_TO_RAD, width / height, 0.1f, 1000.0f);
```
### AABB Expansion
```cpp
AABB bounds; // inverted
for (const auto& v : meshVertices) bounds.Expand(v.position);
if (!bounds.IsValid()) { SEDX_CORE_WARN_TAG("AABB", "Invalid bounds after build"); }
```
### Normalizing a Vector Safely
```cpp
Vec3 dir = playerTarget - playerPos;
float lenSq = dir.x*dir.x + dir.y*dir.y + dir.z*dir.z;
if (lenSq > epsilon<float>()) dir = dir * (1.0f / std::sqrt(lenSq));
```
### Gradient Evaluation
```cpp
Gradient heatmap({ {0.0f, Color::Blue()}, {0.5f, Color::Yellow()}, {1.0f, Color::Red()} });
Color c = heatmap.Evaluate(normalizedValue);
```

## 16. Adding Missing Implementations (Placeholders `{…}`)
When you encounter `{…}` blocks in current headers/cpps:
1. Copy signature into implementation (if header-only desired, keep inline).
2. Follow deterministic row-major ordering for matrix math.
3. Add Doxygen comment before function if externally usable.
4. Add debug assertions for preconditions; release path should still return a defined value.
5. If adding overload sets, group them logically (e.g., vector ops together) and avoid template bloat unless needed.

## 17. Bug Fix Guidance
Common mistakes to avoid / detect:
| Symptom | Typical Cause | Fix Pattern |
|---------|---------------|-------------|
| Incorrect orientation after rotation application | Multiplied in wrong order (used `R * M` instead of `M * R`) | Enforce post-multiply API semantics in docs & code. |
| Skewed or non-uniform decomposed rotation | Not removing scale before quaternion extraction | Normalize basis vectors after extracting scale. |
| Projection depth reversed | Using regular perspective but engine expects reverse-Z | Switch to `MakePerspectiveReverseZ` or adjust depth mapping. |
| NaNs in AABB | Expansion over uninitialized/inverted state misuse | Initialize with inverted sentinel or assert on invalid min/max after expansion. |
| Color banding | Lerp in sRGB space directly | Convert to linear before blending; provide `LerpLinear()` if required. |

## 18. Extending the Library
Before adding a new feature (e.g., plane equations, barycentric helpers):
1. Confirm it is engine-wide (multi-system) need vs module-local utility.
2. Prefer a new focused header (e.g., `plane.h`) rather than overloading existing ones.
3. Maintain naming pattern: noun or verb-first for stateless builder (e.g., `MakePlane` or `PlaneFromPoints`).
4. Add minimal tests (once test harness for math established) verifying round-trip or algebraic identities.

## 19. Documentation & Doxygen
Every public function should include:
* `@brief` one-liner
* Parameter descriptions (units specified – degrees/radians, normalized ranges)
* Return semantics (ownership? invariants?)
* Notes on precision or edge behavior (e.g., zero-length normalization returns zero vector)

## 20. “DO / DO NOT” Quick Reference
**DO**:
* Use `constexpr` where possible.
* Use engine constants instead of literals.
* Use `epsilonEqual` for float comparisons.
* Keep row-major ordering consistent.
* Keep operations pure (no hidden state).

**DO NOT**:
* Introduce GLM headers or mimic GLM namespace.
* Change operand order semantics silently.
* Use exceptions for numeric edge cases (use asserts + graceful fallback).
* Add implicit unit conversions.

## 21. Future TODO Placeholders (Mark Clearly in Code)
When scaffolding yet-to-be-done functions, annotate:
```cpp
// TODO(Math): Implement quaternion slerp (shortest path) – clamp dot, handle near-zero angle using nlerp fallback.
```
This helps Copilot not hallucinate behavior without explicit direction.

## 22. Minimal Test Ideas (For Future Catch2 Suites – Update as Features Land)
| Test | Assertion |
|------|-----------|
| Identity multiply | `M * I == M` within epsilon |
| Compose/Decompose round-trip | Original TRS approx equal |
| Quaternion normalization | `Length(norm(q)) == 1` |
| AABB expansion | Contains all inserted points |
| Perspective mapping | NDC z for `zNear` / `zFar` matches convention |

---
## 23. Immediate Shortlist (High-Value Next Steps)
1. Implement `Dot(Vec3,Vec3)` & `Cross(Vec3,Vec3)` in a new or existing header (e.g. extend `dot.h` or a `vector_ops.h`).
2. Add vector length / normalize helpers with epsilon guards.
3. Provide `projection.h` with reverse & infinite perspective variants plus centered ortho.
4. Introduce `Transforms::Compose` / `Decompose` to remove ad-hoc TRS construction.
5. Scaffold `quat.h` with documented multiplication semantics (YXZ Euler conversions consistent with matrix helpers).
6. Implement AABB (expansion, validity) for forthcoming culling work.

---
**Reminder for Copilot:** Use existing implemented `Mat4` methods where possible; do **not** re‑invent projection or inversion inline. Reflect CURRENT multiplication semantics (rhs first) unless/until code is refactored to match earlier draft wording.
