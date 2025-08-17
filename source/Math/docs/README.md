# Scenery Editor X Math Library (2025-08 Update)

> Status: Living documentation synchronized with current `source/Math` implementation. Earlier drafts referenced the legacy path `source/SceneryEditorX/utils/math`; adjust includes to the new root (`<Math/...>`). This update reflects the now feature-rich `Mat4` and clarifies transform multiplication semantics.

The Scenery Editor X Math Library provides a lightweight, engine‑oriented set of vector, matrix, quaternion (planned), color, gradient, transform (planned), and projection utilities used throughout rendering, asset processing, spatial reasoning, and editor tooling. It replaces external monolithic math dependencies with a small, explicit API (deterministic layout, minimal headers, fast inlining, explicit precision & epsilon policy).

## Goals

- Fast, minimal, self‑contained (no heavy template meta‑programming, no global state)
- Explicit layouts (row‑major matrices, component names with semantic aliases)
- Predictable binary compatibility for serialization & GPU uniform packing
- Clear separation of primitive types (Vec*, Mat*, Quat) and higher‑level helpers (Transforms, projection, rotation/scale/translation builders)
- GLM compatibility shims maintained only where needed for transition (clearly marked)

## Directory Layout (Current)

```text
utils/math/
   aabb.h                # Axis-aligned bounding box (placeholder, unimplemented)
  colors.h / .cpp       # Color struct & utility conversions (HSV/RGB, packing, clamping)
  gradients.h / .cpp    # Keyframed color gradient with hashing & evaluation
   constants.h           # Core mathematical constants (PI, DEG↔RAD, INV_PI, EPSILON_F, etc.)
  dot.h                 # Dot product overloads (Vec2 / Vec4) – extended forms TBD
  epsilon.h             # Epsilon helpers (epsilon<T>, epsilonEqual)
  mat2.h                # 2×2 matrix (row-major)
  mat3.h                # 3×3 matrix (row-major)
  mat4.h                # 4×4 matrix (row-major) + formatting support
  math_utils.h / .cpp   # Generic helpers (approx comparison, angle conversions, etc.)
  math.h                # Umbrella include for the full math subsystem
  matrix.h / .cpp       # Aggregate matrix include & shared matrix operations
   projection.h          # (Planned) Consolidated perspective/orthographic builders (currently some live in Mat4)
   quat.h / .cpp         # (Planned) Quaternion type & operations
  rotation.h            # Axis‑angle & matrix rotation helpers (+ GLM-style wrappers)
  scale.h               # Scale matrix constructors
  translate.h           # Translation matrix constructors
   transforms.h / .cpp   # (Planned) Decompose / compose TRS matrices
  vector.h              # Umbrella include for vector templates
  vec2.h / vec3.h / vec4.h  # TVectorN templates with semantic component unions
```

## Primitive Types Overview

| Type | Purpose | Storage | Notes |
|------|---------|---------|-------|
| `Vec2` / `Vec3` / `Vec4` | Cartesian / color / texture coordinate vectors | Plain aggregates via `TVectorN<T>` | Multiple semantic aliases (`x/y/z/w`, `r/g/b/a`, `s/t/p/q`) |
| `Mat2` | 2×2 linear ops (2D transforms, UV space) | Row-major scalars | Identity by default |
| `Mat3` | 3×3 rotation/scale/skew (no translation) | Row-major | Often embedded in `Mat4` upper-left |
| `Mat4` | Full 4×4 affine/projective transforms | Row-major contiguous floats (translation last column) | Rich API: perspective/ortho, look-at, inverse, transpose, arithmetic |
| `Quat` | Rotation representation (x,y,z,w) | (pending) | Will provide stable rotation accumulation |
| `Color` | RGBA or linear color | Likely floats (see `colors.h`) | Utility conversions & interpolation |
| `Gradient` | Time / position mapped colors | Vector of sorted keys | Hashing for cache invalidation |

## Conventions & Semantics

* **Row-major storage**: Matrices store rows contiguously; translation components sit in the last column (`m03, m13, m23`).
* **Multiplication order (current)**: `A * B` applies **B first, then A** (column-vector conceptual order). Match this in TRS compositions until/unless refactored. When composing TRS manually use `M = T * R * S` so a point experiences S→R→T.
* **Right-handed coordinate system** (camera/look-at is right-handed). Y-up unless otherwise documented in higher-level engine docs.
* **Angles**: Core uses radians; degree variants provided where convenient. Convert via `DEG_TO_RAD` / `RAD_TO_DEG` constants (already defined in `constants.h`).
* **Floating comparison**: Use `epsilonEqual` (scalar) and implement vector/matrix overloads before relying on tolerant comparisons outside scalar scope.
* **No implicit normalization**: Vector/quaternion normalization (once quats exist) must be explicit to avoid hidden costs.

## Umbrella Headers

| Header | Includes | Use Case |
|--------|----------|----------|
| `math.h` | All public math components | High-level engine module includes |
| `matrix.h` | `mat2.h`, `mat3.h`, `mat4.h` | When only matrix types needed |
| `vector.h` | `vec2.h`, `vec3.h`, `vec4.h` | Lightweight vector-only usage |

## Transformation Pipeline (Planned Final Form)

Typical object/world transform operations:

1. Build individual components:
   - `T` via translation helpers (`translate.h`)
   - `R` via quaternion (`Quat`) or axis-angle (`rotation.h`)
   - `S` via uniform / non-uniform scale (`scale.h`)
2. Compose: `M = Transforms::Compose(translation, rotationQuat, scaleVec)`
3. For cameras / projections:
   - `P = Mat4::PerspectiveProjection(aspect, fovDeg, near, far)` (current) OR future `MakePerspective()` in `projection.h`.
   - `V` from inverse of camera world transform (utility TBD / camera system)
   - Final: `MVP = P * V * M`
4. Decomposition: `Transforms::Decompose(matrix, outT, outR, outS)` for gizmos, animation, or serialization.

## Key Components

### Vectors (`vec2.h`, `vec3.h`, `vec4.h`)

- Lightweight templates `TVectorN<T>`
- Semantic unions for clarity in different domains (color, texture, position)
Implemented: constructors, arithmetic (+,-,* scalar,/ scalar), compound assigns, unary minus, exact equality, indexing, free scalar*Vec3.
Missing: Length/LengthSq, Normalize/SafeNormalize, Dot(Vec3), Cross(Vec3), epsilon-aware equality, per-component clamp/min/max/abs, lerp.

### Matrices (`mat2.h`, `mat3.h`, `mat4.h`)

`Mat4` implements zero, diagonal (identity), translation, scale (Vec2/Vec3), Euler rotations (degrees & radians YXZ), Z-axis rotation (`Angle`), perspective/orthographic projections, look-at, arithmetic (+,-,*,/ with scalar & matrix), transpose (in-place & free), inverse (analytical), determinant support, `NearlyEqual`, pointer access, `fmt` formatter. `Mat2`/`Mat3` pending parity.

### Quaternions (`quat.h/.cpp`) – Pending

Planned features: axis-angle & Euler (YXZ) constructors, length/normalize, multiply (composition, matching matrix semantics), conjugate/inverse, slerp (shortest path), nlerp fallback, to/from Mat3/Mat4. Document order semantics explicitly when implemented.

### Transforms (`transforms.h/.cpp`) – Pending

Will provide `Compose(Vec3, Quat, Vec3)` building `T * R * S` (consistent with current matrix multiply semantics) and `Decompose` (extract translation from last column, scale from basis lengths, rotation from normalized basis → quaternion).

### Projection (`projection.h`) – Planned

Centralize existing `Mat4::PerspectiveProjection / OrthographicProjection` plus reverse-Z, infinite far, centered ortho, jittered (TAA) variants. Provide consistent validation (fov bounds, aspect>0, near>0, far>near unless reverse/infinite) and depth mapping documentation.

### Rotation / Scale / Translation Helpers

Higher-level free helpers (or thin wrappers) still desirable for explicit intent (`Translate(m, v)`, `RotateEulerDegrees(m, e)`, etc.) even though many operations exist as `Mat4` statics.

### Colors & Gradients (`colors.*`, `gradients.*`)

- Color utilities (e.g., clamping, conversions, interpolation) used by UI, material systems, editor themes.
- `Gradient` supports evaluation at normalized position, dynamic key insertion/removal, hashing for cache keys.

### Constants & Epsilon (`constants.h`, `epsilon.h`, `math_utils.*`)

- Centralized constants prevent magic numbers.
- `epsilon<T>()` and `epsilonEqual` avoid dependency on GLM while preserving familiar semantics.
- `IsEqual(a, b, eps)` is a convenience wrapper.

### AABB (`aabb.h`) – Placeholder

- Placeholder for axis-aligned bounding box type to support culling / spatial queries.
- Planned features: expansion, overlap test, ray intersection, center/extent queries.

## Dependency Graph (Conceptual)

```text
vector → (independent primitives)
mat2/mat3/mat4 → vector, epsilon
quat → vector, math_utils
transforms → mat4, quat, vector, scale/rotation/translate
projection → constants, mat4, vector
colors → math_utils (for clamp/lerp) & possibly vector
gradients → colors
math_utils → constants, vector, quat (selected helpers)
umbrella headers aggregate (no reverse deps)
```

## Usage Examples

### Basic Vector Arithmetic

```cpp
#include <SceneryEditorX/utils/math/vector.h>

using namespace SceneryEditorX;

Vec3 a{1.0f, 2.0f, 3.0f};
Vec3 b{4.0f, 5.0f, 6.0f};
Vec3 sum = a + b;        // (5,7,9)
Vec3 scaled = 2.0f * a;  // (2,4,6)
```

### Compose & Decompose Transform (Future API Sketch)

```cpp
#include <SceneryEditorX/utils/math/math.h>

Vec3 position{10.0f, 2.0f, -5.0f};
Quat rotation; // identity
Vec3 scale{2.0f, 1.5f, 2.0f};

Mat4 world = Transforms::Compose(position, rotation, scale);

Vec3 outT, outS; Quat outR;
bool ok = Transforms::Decompose(world, outT, outR, outS);
```

### Perspective Projection

```cpp
#include <SceneryEditorX/utils/math/projection.h>

float fovDeg = 60.0f;
float aspect = 1920.0f / 1080.0f;
float nearZ = 0.1f;
float farZ  = 500.0f;

Mat4 proj = Mat4::PerspectiveProjection(aspect, fovDeg, nearZ, farZ);
```

### Color Gradient Evaluation

```cpp
#include <SceneryEditorX/utils/math/gradients.h>

Gradient g({ {0.0f, Color::Black()}, {0.5f, Color::Red()}, {1.0f, Color::White()} });
Color mid = g.Evaluate(0.5f);
```

## Error Handling & Assertions

- Internal math routines may use `SEDX_CORE_ASSERT` to validate preconditions (e.g., non‑singular decomposition, valid scale components).
- Always sanitize user/editor input before passing to math builders (e.g., guard against zero or negative near plane in `Perspective`).

## Performance Notes

- All small functions are `constexpr` or `inline` to enable full inlining under optimization.
- Avoid temporary allocations; all math types are trivially copyable.
- Prefer composing transforms once per frame and caching when possible (rather than recompute per draw call).

## Extension Guidelines

When adding new functionality:

1. Keep headers minimal – forward declare where possible.
2. Add new primitive types only if they provide clear engine value.
3. Maintain row-major consistency; if adding GPU upload helpers, document transposition requirements explicitly.
4. Add unit tests (Catch2) for numerical stability & edge cases (NaN handling, zero-length vectors, degenerate matrices).
5. Document new APIs in this directory (`docs/math/`), updating dependency and usage sections.

## Pending / TODO Areas (Priority Roughly Ordered)

1. Vector geometric helpers (Length / Normalize / Dot(Vec3) / Cross).
2. `dot.h` Vec3 overload.
3. Quaternion implementation.
4. `projection.h` (reverse & infinite variants + centered ortho + jitter).
5. Transforms compose/decompose utilities.
6. AABB implementation.
7. Mat2/Mat3 feature parity.
8. Epsilon overloads for vectors/matrices.
9. Color space conversions (sRGB ↔ linear) & gradient interpolation finalization.
10. Affine inverse fast path (if profiling indicates need).

- Complete implementation & docs for `AABB`.
- Add cross / normalize / length utilities in vector templates (if not already implemented in source).
- Provide `Inverse(Mat4)` and `Determinant(Mat4)` with robust handling of affine vs projective.
- SIMD optimization pass (structure remains SoA‑friendly).
- Expand color space conversions (sRGB ↔ linear, HDR tone mapping helpers).

---
*Document version updated 2025-08; synchronize when new primitives or helpers land.*
