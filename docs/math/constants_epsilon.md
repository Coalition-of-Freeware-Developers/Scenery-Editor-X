# Math Library – Constants & Epsilon Utilities

Covers numeric constants (pi, degree/radian factors) and floating point comparison helpers (`constants.h`, `epsilon.h`).

## Overview

Provide centralized, constexpr values / utilities to avoid magic numbers and inconsistent tolerance handling across the codebase.

## Core Constants

| Name | Value (float) | Description |
|------|---------------|-------------|
| `PI` | 3.1415926535f | π |
| `TWO_PI` | 6.283185307f | 2π |
| `HALF_PI` | 1.5707963267f | π/2 |
| `DEG2RAD` | PI / 180.f | Degrees → radians |
| `RAD2DEG` | 180.f / PI | Radians → degrees |
| `INV_PI` | 0.318309886f | 1/π |
| `EPSILON_F` | 1e-6f (suggested) | General float epsilon |
| `EPSILON_D` | 1e-12 (suggested) | Double precision epsilon |

> Validate actual numeric definitions; update table if higher precision constants used.

## Epsilon Comparison Helpers

| Function | Purpose | Notes |
|----------|---------|-------|
| `epsilonEqual(a,b,eps)` | Absolute diff compare | Works for non-near-zero magnitudes |
| `epsilonNotEqual(a,b,eps)` | Negation convenience | Maintain symmetry |
| `epsilonRelEqual(a,b,rel,abs)` | Relative + absolute check | Better scaling with magnitude |
| `IsZero(v, eps)` | Absolute near-zero test | Implement as `std::fabs(v) < eps` |

Consider overloading for `Vec2/3/4`, matrices (element-wise all-of), quaternions.

## Suggested Patterns

```cpp
if (!epsilonRelEqual(len, 1.0f, 1e-4f, 1e-6f))
{
    q = Normalize(q);
}
```

Prefer relative + absolute strategy for values spanning wide magnitude range (e.g., distances), and pure absolute for normalized domain values (unit vectors, quaternions).

## Pitfalls

| Issue | Guidance |
|-------|----------|
| Chaining equality on floats | Replace with epsilon compare |
| Using same epsilon everywhere | Tune per domain (position vs angle) |
| Angle wrap-around | Normalize angle before compare |
| Accumulated drift | Periodically re-normalize (unit vectors, quats) |

## Domain-Specific Tolerances (Suggested)

| Domain | Typical Magnitude | Suggested Epsilon |
|--------|-------------------|-------------------|
| Unit length vector | ~1 | 1e-5f |
| Position (world meters) | 1–10^5 | Relative 1e-5f + abs 1e-4f |
| Angle (radians) | 0–π | 1e-5f |
| Scale factors | ~1 | 1e-5f |
| Time seconds | 0–10^4 | 1e-6f or frame delta dependent |

Tune empirically; profile false positives/negatives in assertions.

## Implementation Tips

- Mark constants `constexpr` and `inline` to avoid ODR issues.
- Provide both `float` and `double` forms when needed.
- If performance critical, avoid branching inside tight epsilon vector comparisons—prefer inlined all-of loop.

## Testing Strategy

- Edge cases: comparisons crossing zero boundary.
- Very large vs small magnitude relative comparisons.
- Consistency: `epsilonEqual(a,b) == !epsilonNotEqual(a,b)`.
- Unit tests for composite types (vectors/matrices) verifying element-wise logic.

## TODO / Backlog

- Add angle wrapping helpers (`WrapRadians`, `WrapDegrees`).
- Provide ULP-based comparison for extreme precision cases (optional).
- Integrate with logging macros to dump offending deltas on assertion failure.

---

*Generated documentation file; ensure numbers match source header definitions.*
