# Math Library TODO Checklist

Centralized checklist for implementing and hardening the in‑progress math subsystem (`source/SceneryEditorX/utils/math`). Mark items as completed (`[x]`) when fully implemented **and** documented/tests added where applicable.

---
## Legend
 
- ✅ = Completed
- 🚧 = In progress (partial implementation or missing tests/docs)
- ❌ = Not started

Use standard markdown checkboxes: `[ ]` unchecked, `[x]` checked.

---
## 1. Global / Cross-Cutting

- [x] Add missing constants: `RAD_TO_DEG`, `INV_PI`, `EPSILON_F` (present in `constants.h`) (`EPSILON_D` still TBD)
- [ ] Ensure consistent naming: choose `DEG_TO_RAD` vs `DEG2RAD` (current is `DEG_TO_RAD`; remove legacy mentions)
- [ ] Doxygen comments for all public math APIs (Mat4 heavily documented; others pending)
- [ ] Add vector & matrix overloads for `epsilonEqual` (Vec2/3/4, Quat, Mat2/3/4)
- [ ] Add relative epsilon compare helper (optional)
- [ ] Introduce basic Catch2 test suite for math (identity, compose/decompose, etc.)
- [ ] Add fmt / stream formatters for core types (Vec*, Mat*, Quat, AABB, Colors) (Mat4 done)
- [x] Clarify global coordinate & handedness conventions in docs (README updated)
- [x] Explicitly document Euler order (YXZ) in rotation docs / Mat4 comments (quat pending)

## 2. Vectors (`vec2.h`, `vec3.h`, `vec4.h`, `vector.h`)

- [x] Arithmetic operators (+, -, unary -, *, /, compound assignments) (implemented for Vec3; verify Vec2/Vec4 parity)
- [ ] Scalar multiply/divide free operators both orders (`scalar * v`, `v * scalar`) (Vec3 scalar * v present; others pending)
- [ ] Equality / inequality (epsilon-aware helpers) (exact only now)
- [ ] `LengthSq`, `Length`, `Normalize`, `Normalized`, safe zero guard
- [ ] Dot product (Vec3 currently missing in `dot.h`)
- [ ] Cross product for Vec3
- [ ] Per-component Min / Max / Clamp / Lerp helpers
- [ ] Hadamard (element-wise) multiply helper
- [ ] Pointer access / iterators if needed by renderer
- [ ] Per-component floor/ceil/abs helpers (optional)

## 3. `dot.h`

- [ ] Add `Dot(const Vec3&, const Vec3&)`
- [ ] Consider templated generic TVectorN fallback (optional)

## 4. Matrices

### `mat2.h`
 
- [ ] Define full class (currently placeholder/friend token)
- [ ] Constructors: default identity, elements, diagonal scalar
- [ ] Static `Identity()`
- [ ] Matrix multiply & vector multiply
- [ ] Determinant / Inverse / Transpose
- [ ] FromRotation(angle) (2D)
- [ ] Epsilon-based equality
- [ ] Stream/fmt formatter

### `mat3.h`
 
- [ ] Complete class definition (members present, methods missing)
- [ ] Constructors & `Identity()`
- [ ] Mat3 × Mat3 / Mat3 × Vec3 multiply
- [ ] Determinant / Cofactor / Adjugate / Inverse / Transpose
- [ ] FromQuaternion
- [ ] Euler (YXZ) build & extract
- [ ] Normal matrix builder (from Mat4)
- [ ] Epsilon equality
- [ ] Stream/fmt formatter

### `mat4.h`

- [x] Full struct definition (present with rich API)
- [x] Constructors (zero, diagonal identity, rows, initializer list) (Mat3-from ctor pending)
- [x] `Identity()`
- [x] Mat4 × Mat4 multiply (implemented with static `Multiply`)
- [ ] `TransformPoint` / `TransformVector` helpers
- [x] Determinant & inverse (analytical) (opt affine inverse TODO)
- [x] Transpose (in-place & free)
- [ ] FromTRS (pending; to live in `Transforms` or helper)
- [ ] ToTRS (decompose; planned in `Transforms`)
- [x] fmt formatter specialization implementation
- [x] Epsilon equality (`NearlyEqual`)

## 5. Quaternions (`quat.h/.cpp`)

- [ ] Define visible `Quat` struct (x,y,z,w) with identity default
- [ ] Axis-angle constructor & `ToAxisAngle`
- [ ] Euler YXZ constructor & `ToEulerYXZ`
- [ ] From/To rotation matrix (Mat3 / Mat4)
- [ ] Length / LengthSq / Normalize / Normalized
- [ ] Conjugate / Inverse
- [ ] Multiplication (document order semantics; must match matrix semantics: rhs first)
- [ ] RotateVector(Vec3)
- [ ] Dot(Quat,Quat)
- [ ] Slerp (shortest path, clamp dot)
- [ ] Nlerp fallback for small angle
- [ ] Epsilon equality
- [ ] fmt / stream formatting

## 6. Transforms (`transforms.h/.cpp`)
 
- [ ] Implement `Compose(translation, rotation, scale)` (T*R*S row-major)
- [ ] Implement `Decompose(transform, outT, outR, outS)` with scale extraction and normalized rotation basis
- [ ] Handle negative scale / handedness sign
- [ ] Return failure on degenerate scale (epsilon threshold)
- [ ] Overload without scale (defaults 1,1,1) (optional)
- [ ] Tests for round-trip accuracy

## 7. Rotation Helpers (`rotation.h`)
 
- [ ] `RotateZDegrees` / `RotateZRadians`
- [ ] Euler YXZ builders (radians & degrees)
- [ ] Post-multiply variants (matrix * rotation)
- [ ] Generic axis-angle builder `RotateAxisAngle(axis, angle)`
- [ ] Remove or implement GLM shim placeholders
- [ ] Unit tests verifying orientation against quaternion path

## 8. Translation Helpers (`translate.h`)
 
- [ ] `Translate(Vec3)` → Mat4
- [ ] `Translate(const Mat4&, Vec3)` post-multiply
- [ ] `Translate2D(Vec2)` → Mat3
- [ ] `Translate2D(const Mat3&, Vec2)`

## 9. Scale Helpers (`scale.h`)
 
- [ ] `Scale(Vec3)` → Mat4
- [ ] `Scale(const Mat4&, Vec3)` post-multiply
- [ ] Planar `Scale(Vec2)` (z=1)
- [ ] 2D: `Scale2D(Vec2)` → Mat3
- [ ] `Scale2D(const Mat3&, Vec2)`
- [ ] Vector length set / normalization helper (`SetLength` or `Scale(vec,len)`) with zero guard

## 10. Projection Builders (`projection.h`)

- [ ] `MakePerspective` (current stopgap: `Mat4::PerspectiveProjection`)
- [ ] `MakePerspectiveInfinite`
- [ ] `MakePerspectiveReverseZ`
- [ ] `MakeOrthographic` (current stopgap: `Mat4::OrthographicProjection`)
- [ ] `MakeOrthographicCentered`
- [ ] Parameter validation & assertions (aspect, near/far ordering) (partially in Mat4 static funcs)
- [ ] Reverse-Z depth mapping verification
- [ ] Optional left-handed variants (future)
- [ ] Tests (NDC mapping of near/far)

## 11. AABB (`aabb.h` + new `aabb.cpp`)
 
- [ ] Struct definition (`min`, `max`)
- [ ] Inverted default constructor
- [ ] Corner constructor with min/max ordering
- [ ] `Reset()` to inverted state
- [ ] `Expand(const Vec3&)`
- [ ] `Expand(const AABB&)`
- [ ] `Center()`
- [ ] `Extents()` / `HalfExtents()`
- [ ] `IsValid()`
- [ ] `Contains(point)` / `Contains(aabb)`
- [ ] `Intersects(aabb)`
- [ ] `SurfaceArea()` & `Volume()`
- [ ] Free `Merge(a,b)` helper
- [ ] Ray intersection (optional future)
- [ ] fmt / stream formatter
- [ ] Tests (expansion, intersection, validity)

## 12. Constants (`constants.h`)

- [x] Add/verify: `RAD_TO_DEG`, `INV_PI`, `EPSILON_F`
- [ ] Add `EPSILON_D` or decide policy (use `std::numeric_limits<double>::epsilon()`)
- [ ] Consistency audit: naming + duplication avoidance (DEG_TO_RAD vs legacy aliases)
- [ ] Documentation alignment with `constants_epsilon.md` (pending update applied in this sweep)

## 13. Epsilon (`epsilon.h`)
 
- [ ] Vector overloads (Vec2/3/4)
- [ ] Quaternion overload
- [ ] Matrix element-wise compare
- [ ] Relative epsilon function (optional) `relativeEqual(a,b,rel,abs)`

## 14. Math Utilities (`math_utils.h/.cpp`)
 
- [ ] `Clamp`, `Saturate`, `Lerp`, `SmoothStep`
- [ ] Angle helpers: `Radians(deg)`, `Degrees(rad)`, `WrapAngleRadians`, `WrapAngleDegrees`
- [ ] `NearlyZero(value, eps)` (+ vector variant)
- [ ] `Orthogonal(Vec3)` (generate any perpendicular)
- [ ] `SafeNormalize(Vec3, fallback)`
- [ ] Affine matrix inverse helper if not in `mat4`
- [ ] Unit tests for utility correctness

## 15. Matrix Implementation (`matrix.cpp`)

- [x] Mat4 multiply (implemented static in header; verify cpp implementation parity)
- [x] Transpose & inverse (general analytical path)
- [ ] Affine fast path inverse (optimization)
- [ ] Determinant specialized affine path (if beneficial)
- [ ] Optional SIMD optimization (future)
- [ ] Benchmark harness (future)

## 16. Colors (`colors.h/.cpp`)
 
- [ ] Define color structs: `ColorRGBA8`, `ColorRGBF`, `ColorRGBAF`, `HSV`
- [ ] Byte ↔ float linear conversions
- [ ] sRGB ↔ linear conversions
- [ ] HSV ↔ RGB conversions
- [ ] Predefined color constants
- [ ] Lerp (linear) & optional gamma-correct variant
- [ ] Pack/unpack 32-bit RGBA
- [ ] Tests for round-trip conversions

## 17. Gradients (`gradients.h/.cpp`)
 
- [ ] Implement `Evaluate(position)` interpolation
- [ ] `AddKey` (sorted insert) & `Clear`
- [ ] Optional `RemoveKey`
- [ ] Clamp evaluation input
- [ ] Support different interpolation modes (future)
- [ ] Convenience preset gradients (heatmap, terrain, etc.)
- [ ] Tests (key ordering, endpoint interpolation)

## 18. Reverse-Z & Depth Precision
 
- [ ] Validate reverse-Z projection correctness in renderer path
- [ ] Document when to use reverse vs standard perspective
- [ ] Add tests comparing depth distribution (optional)

## 19. Formatters / Logging Integration
 
- [ ] Implement fmt::formatter for Vec2/3/4
- [ ] Implement fmt::formatter for Mat2/3/4 (compact & multiline option?)
- [ ] Implement fmt::formatter for Quat
- [ ] Implement fmt::formatter for AABB
- [ ] Implement fmt::formatter for Color types

## 20. Performance / SIMD (Future Enhancements)
 
- [ ] Optional SSE/AVX/NEON path for Mat4 multiply
- [ ] Optional SIMD quaternion normalization
- [ ] Micro-benchmark harness (timing macro or Tracy zones)

## 21. Documentation Enhancements
 
- [ ] Fill out all `docs/math/*.md` with final API signatures
- [ ] Add diagrams for matrix storage & transform composition
- [ ] Add section on numeric stability & epsilon policy
- [ ] Add guidelines for choosing projection variant

## 22. Testing Roadmap (Catch2)
 
- [ ] Identity & associativity basic checks
- [ ] Compose/Decompose round-trip TRS
- [ ] Quaternion slerp unit arc cases (0°, 180°)
- [ ] Projection near/far mapping validation
- [ ] AABB containment & intersection matrix
- [ ] Color conversion round-trip error margin
- [ ] Gradient interpolation correctness

## 23. Nice-to-Have / Deferred
 
- [ ] Plane, Frustum, Ray types
- [ ] Ray vs AABB, Ray vs Plane intersection utilities
- [ ] Barycentric coordinate helpers
- [ ] Bezier / spline interpolation utilities
- [ ] Dual quaternions for skeletal animation (future)
- [ ] SIMD lane-friendly SoA variants for batch transforms

---
### Progress Tracking Notes
Add brief notes below as items are implemented (commit hash, date, test refs):

| Date | Item(s) Completed | Commit | Notes |
|------|-------------------|--------|-------|
|      |                   |        |       |

---
**Reminder:** Mark a task complete only after: code + documentation + (where applicable) tests are implemented and pass.
