# Math Library – Axis-Aligned Bounding Boxes (AABB)

Covers planned / existing `aabb.h` utilities: construction, expansion, intersection tests, and support functions for culling & spatial queries.

## Overview

AABB represented by min/max corners (Vec3). Invariant: `min <= max` component-wise. Used for broad-phase culling, spatial partitioning, frustum checks, ray casts, and scene import bounds.

## Data Structure

```cpp
struct AABB
{
    Vec3 min; // inclusive
    Vec3 max; // inclusive
};
```

Ensure constructors enforce ordering (swap if user passes inverted corners) or assert in debug builds.

## Core Operations (Planned / Existing)

| Operation | Status | Notes |
|-----------|--------|-------|
| Default constructor (empty or zero) | TODO | Decide: empty sentinel vs (0,0,0) box |
| Construct(min,max) | TODO | Validate ordering |
| Reset() | TODO | Set to inverted bounds for expansion pattern |
| Expand(point) | TODO | Grow to include point |
| Expand(aabb) | TODO | Merge two boxes |
| Center() | TODO | 0.5f*(min+max) |
| Extents() | TODO | (max-min) |
| HalfExtents() | TODO | 0.5f*(max-min) |
| Volume() | TODO | Zero if invalid |
| Contains(point) | TODO | Inclusive check |
| Contains(aabb) | TODO | Full containment |
| Intersects(aabb) | TODO | Overlap test (≥ semantics) |
| RayIntersect(ray, tMin, tMax) | TODO | Slab method |
| Transform(mat4) | TODO | 8 corner transform then rebuild |
| IsValid() | TODO | min <= max per component |

## Usage Pattern – Expansion

```cpp
AABB box;
box.Reset();          // sets min = +INF, max = -INF
for (const Vec3& p : points)
    box.Expand(p);
```

## Frustum Culling Helper (Planned)

```cpp
bool IntersectsFrustum(const AABB& box, const Frustum& frustum);
```

Select the most positive vertex against plane normal for early-out (standard AABB vs plane test).

## Ray Intersection (Slab Method Sketch)

```cpp
bool RayIntersect(const AABB& b, const Ray& r, float& tMinOut, float& tMaxOut)
{
    float tmin = 0.0f;
    float tmax = r.tMax; // or large number
    for (int axis=0; axis<3; ++axis)
    {
        float invD = 1.0f / r.dir[axis];
        float t0 = (b.min[axis] - r.origin[axis]) * invD;
        float t1 = (b.max[axis] - r.origin[axis]) * invD;
        if (invD < 0.0f) std::swap(t0, t1);
        tmin = t0 > tmin ? t0 : tmin;
        tmax = t1 < tmax ? t1 : tmax;
        if (tmax < tmin) return false;
    }
    tMinOut = tmin; tMaxOut = tmax; return true;
}
```

## Transforming AABBs

Transform with full `Mat4` (rotation creates OBB). To remain axis-aligned:

1. Generate 8 corners.
2. Multiply by matrix.
3. Compute new min/max.

Optimization: If matrix is only TRS without rotation (or uniform scale + translation), transform min and max directly.

## Numerical Stability

- Avoid NaN by validating input points.
- For expansion after Reset, initialize min with `+FLT_MAX`, max with `-FLT_MAX`.
- Ray epsilon: clamp division by near-zero direction components; treat as large/small t interval.

## Suggested Extensions

| Feature | Priority | Rationale |
|---------|----------|-----------|
| SIMD merge/expand | Medium | Batch updates in spatial trees |
| Morton code helper | Medium | Spatial indexing |
| Frustum packet test | Medium | Vectorized culling |
| OBB support | Low | Precise culling where needed |
| Serialization helpers | Low | Scene cache persistence |

## Performance Notes

- Branchless intersect can help in hot loops; profile before adopting.
- Store precomputed center & half extents if repeatedly needed (trade memory for speed).

## Testing Strategy

- **Containment**: Points on faces/edges/corners included.
- **Intersection**: Overlapping, touching (face contact), disjoint.
- **Ray**: Orthogonal, diagonal, miss, starting inside.
- **Transform**: Pure translation vs rotation correctness.

## Logging & Assertions

- Assert on invalid boxes when used in culling.
- Debug log expansion anomalies (e.g., huge volume growth in one frame).

## TODO / Backlog

- Finalize default ctor semantics (empty vs zero box).
- Add unit tests for inverse transform assumptions.
- Integrate with frustum + projection utilities once frustum type solidified.

---

*Generated documentation file; update as AABB implementation evolves.*
