# Scenery Editor X Math Library

> Status: Draft (initial authoring based on current `source/SceneryEditorX/utils/math` layout – fill in implementation details as functions are completed)

The Scenery Editor X Math Library provides a lightweight, engine‑oriented set of vector, matrix, quaternion, color, gradient, transform, and projection utilities used throughout rendering, asset processing, spatial reasoning, and editor tooling. It replaces external monolithic math dependencies with a small, explicit API tailored to engine needs (deterministic layout, minimal headers, fast inlining, and clear ownership of precision policy).

## Goals

- Fast, minimal, self‑contained (no heavy template meta‑programming, no global state)
- Explicit layouts (row‑major matrices, component names with semantic aliases)
- Predictable binary compatibility for serialization & GPU uniform packing
- Clear separation of primitive types (Vec*, Mat*, Quat) and higher‑level helpers (Transforms, projection, rotation/scale/translation builders)
- GLM compatibility shims maintained only where needed for transition (clearly marked)

## Directory Layout

```text
utils/math/
  aabb.h                # Axis-aligned bounding box (planned / placeholder)
  colors.h / .cpp       # Color struct & utility conversions (HSV/RGB, packing, clamping)
  gradients.h / .cpp    # Keyframed color gradient with hashing & evaluation
  constants.h           # Core mathematical constants (PI, DEG↔RAD, etc.)
  dot.h                 # Dot product overloads (Vec2 / Vec4) – extended forms TBD
  epsilon.h             # Epsilon helpers (epsilon<T>, epsilonEqual)
  mat2.h                # 2×2 matrix (row-major)
  mat3.h                # 3×3 matrix (row-major)
  mat4.h                # 4×4 matrix (row-major) + formatting support
  math_utils.h / .cpp   # Generic helpers (approx comparison, angle conversions, etc.)
  math.h                # Umbrella include for the full math subsystem
  matrix.h / .cpp       # Aggregate matrix include & shared matrix operations
  projection.h          # Perspective & orthographic projection builders (+ shims)
  quat.h / .cpp         # Quaternion type & operations
  rotation.h            # Axis‑angle & matrix rotation helpers (+ GLM-style wrappers)
  scale.h               # Scale matrix constructors
  translate.h           # Translation matrix constructors
  transforms.h / .cpp   # Decompose / compose TRS matrices
  vector.h              # Umbrella include for vector templates
  vec2.h / vec3.h / vec4.h  # TVectorN templates with semantic component unions
```

## Primitive Types Overview

| Type | Purpose | Storage | Notes |
|------|---------|---------|-------|
| `Vec2` / `Vec3` / `Vec4` | Cartesian / color / texture coordinate vectors | Plain aggregates via `TVectorN<T>` | Multiple semantic aliases (`x/y/z/w`, `r/g/b/a`, `s/t/p/q`) |
| `Mat2` | 2×2 linear ops (2D transforms, UV space) | Row-major scalars | Identity by default |
| `Mat3` | 3×3 rotation/scale/skew (no translation) | Row-major | Often embedded in `Mat4` upper-left |
| `Mat4` | Full 4×4 affine/projective transforms | Row-major contiguous floats | API designed for TRS composition, projection, camera matrices |
| `Quat` | Rotation representation (x,y,z,w) | Scalar members | Right-handed, unit-length expected for rotation |
| `Color` | RGBA or linear color | Likely floats (see `colors.h`) | Utility conversions & interpolation |
| `Gradient` | Time / position mapped colors | Vector of sorted keys | Hashing for cache invalidation |

## Conventions

- **Row-major matrices**: Indexing documented in each header. When interfacing with APIs expecting column-major (e.g., some shading languages), explicit transpose or upload paths handle conversion.
- **Right-handed coordinate system** unless noted.
- **Angles**: Radians internally; degree helpers provided (`DEG_TO_RAD`, `RAD_TO_DEG`, `ToRadians`, `ToDegrees`).
- **Epsilon comparisons**: Use `epsilonEqual` or `IsEqual` from `math_utils` for float stability.
- **No hidden allocations**: All math types are POD‑like and trivially movable.

## Umbrella Headers

| Header | Includes | Use Case |
|--------|----------|----------|
| `math.h` | All public math components | High-level engine module includes |
| `matrix.h` | `mat2.h`, `mat3.h`, `mat4.h` | When only matrix types needed |
| `vector.h` | `vec2.h`, `vec3.h`, `vec4.h` | Lightweight vector-only usage |

## Transformation Pipeline

Typical object/world transform operations:

1. Build individual components:
   - `T` via translation helpers (`translate.h`)
   - `R` via quaternion (`Quat`) or axis-angle (`rotation.h`)
   - `S` via uniform / non-uniform scale (`scale.h`)
2. Compose: `M = Transforms::Compose(translation, rotationQuat, scaleVec)`
3. For cameras / projections:
   - `P = Perspective(fovY, aspect, near, far)` or `Orthographic(left,right,bottom,top,near,far)` (in `projection.h`)
   - `V` from inverse of camera world transform (utility TBD / camera system)
   - Final: `MVP = P * V * M`
4. Decomposition: `Transforms::Decompose(matrix, outT, outR, outS)` for gizmos, animation, or serialization.

## Key Components

### Vectors (`vec2.h`, `vec3.h`, `vec4.h`)

- Lightweight templates `TVectorN<T>`
- Semantic unions for clarity in different domains (color, texture, position)
- Basic arithmetic operators (+, -, *, /) return new vectors (inlined)
- Free scalar * vector overload (e.g., `2.0f * v`)

### Matrices (`mat2.h`, `mat3.h`, `mat4.h`)

- Identity by default constructor.
- Stream (`operator<<`) and `fmt` formatting (for `Mat4`) for logging & debugging.
- Intended future ops: multiplication, determinant, inverse, transpose (some may be in `.cpp` or pending — fill as implemented).

### Quaternions (`quat.h/.cpp`)

- Stored as `(x, y, z, w)` with identity = (0,0,0,1)
- Provide construction from axis‑angle and conversion to rotation matrices.
- Normalize after composite rotations to maintain numerical stability.

### Transforms (`transforms.h/.cpp`)

- `Compose(translation, rotation, scale)` → `Mat4`
- `Decompose(mat, outT, outR, outS)` → extracts TRS; asserts validity.
- Uses epsilon tolerances to counter floating‑point drift.

### Projection (`projection.h`)

- Perspective and orthographic matrix constructors.
- Maintains GLM‑style compatibility wrappers (namespaced shim) for gradual migration.

### Rotation / Scale / Translation Helpers

- Each domain broken into dedicated headers (`rotation.h`, `scale.h`, `translate.h`) for minimal inclusion and compile performance.
- Provide `Mat4` builders rather than modifying existing matrices in-place (favor explicit construction).

### Colors & Gradients (`colors.*`, `gradients.*`)

- Color utilities (e.g., clamping, conversions, interpolation) used by UI, material systems, editor themes.
- `Gradient` supports evaluation at normalized position, dynamic key insertion/removal, hashing for cache keys.

### Constants & Epsilon (`constants.h`, `epsilon.h`, `math_utils.*`)

- Centralized constants prevent magic numbers.
- `epsilon<T>()` and `epsilonEqual` avoid dependency on GLM while preserving familiar semantics.
- `IsEqual(a, b, eps)` is a convenience wrapper.

### AABB (`aabb.h`)

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

### Compose & Decompose Transform

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

Mat4 proj = Perspective(ToRadians(fovDeg), aspect, nearZ, farZ);
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

## Pending / TODO Areas

- Complete implementation & docs for `AABB`.
- Add cross / normalize / length utilities in vector templates (if not already implemented in source).
- Provide `Inverse(Mat4)` and `Determinant(Mat4)` with robust handling of affine vs projective.
- SIMD optimization pass (structure remains SoA‑friendly).
- Expand color space conversions (sRGB ↔ linear, HDR tone mapping helpers).

---
*Generated documentation scaffolding – refine as implementation details are finalized.*
