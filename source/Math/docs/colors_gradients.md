# Math Library – Colors & Gradients

Covers color utility types (`color.h`) and gradient interpolation (`gradient.h`).

## Overview

Color utilities supply lightweight RGBA container(s) plus conversions; gradient system provides keyed color ramps for UI themes, heatmaps, and visualization overlays.

## Color Representation

| Type | Components | Range | Notes |
|------|------------|-------|------|
| `ColorRGBA8` | 4 × uint8 | 0–255 | Packed 8-bit per channel |
| `ColorRGBF` | 3 × float | [0,1] | Linear color (no alpha) |
| `ColorRGBAF` | 4 × float | [0,1] | Linear color + alpha |
| `HSV` | h,s,v floats | h: [0,360) | Useful for procedural palettes |

*Confirm actual typedefs / struct names with implementation; adjust table accordingly.*

## Constructors / Helpers

| Function | Purpose |
|----------|---------|
| `ColorRGBAF::FromBytes(r,g,b,a)` | Convert 0–255 to normalized |
| `ColorRGBAF::FromSRGBBytes(r,g,b,a)` | sRGB→linear conversion |
| `ToSRGBBytes(color)` | Linear→sRGB gamma encode |
| `Lerp(a,b,t)` | Linear interpolate (no gamma correction) |
| `LerpSRGB(a,b,t)` | sRGB correct blend (optional) |
| `Premultiply(color)` | Multiply RGB by alpha |

## Conversions

| Source → Dest | Notes |
|---------------|-------|
| RGBA8 → RGBAF | Divide by 255.0f |
| RGBAF → RGBA8 | Clamp & round |
| sRGB ↔ Linear | Use fast approximate or exact pow path |
| HSV ↔ RGB | Handle hue wrap & achromatic cases |

## Gamma & Color Space

- Internal linear space recommended for lighting.
- Only convert to sRGB at IO boundaries (texture load/save, UI rendering if required).
- Provide fast path approximate gamma (e.g., `x^(1/2.2)`) and accurate path (IEC spec) if precision matters.

## Utility Functions (Planned / Existing)

| Function | Status | Notes |
|----------|--------|-------|
| `Clamp(color)` | TODO | Clamp each channel to [0,1] |
| `IsFinite(color)` | TODO | Check for NaN / Inf |
| `ApproxEqual(a,b,eps)` | TODO | Per-channel epsilon compare |
| `Luminance(color)` | TODO | Weighted dot (Rec.709) |
| `ContrastRatio(a,b)` | TODO | Accessibility metrics |

## Gradients

A gradient defined by sorted keyframes `(t ∈ [0,1], Color)`; evaluation interpolates between adjacent keys.

### Data Sketch

```cpp
struct GradientKey { float t; ColorRGBAF color; };
struct Gradient
{
    std::vector<GradientKey> keys; // sorted ascending by t
    ColorRGBAF Evaluate(float t) const;
};
```

### Evaluation Algorithm

1. Clamp `t` to [0,1].
2. Binary search (or linear if few keys) for enclosing segment.
3. Normalized local `u = (t - t0)/(t1 - t0)`.
4. `return Lerp(color0, color1, u)`.

### Edge Cases

| Case | Behavior |
|------|----------|
| No keys | Return opaque black (or assert) |
| Single key | Return that key's color |
| Duplicate t | Prefer last inserted or average; document policy |
| Unsorted insertion | Sort after mutation; defer until evaluation if batching |

## Suggested Extensions

| Feature | Priority | Rationale |
|---------|----------|-----------|
| Caching LUT (N samples) | Medium | Fast repeated evaluation in shaders / UI |
| sRGB-aware interpolation | Medium | Better perceptual gradients |
| Alpha mode selection (premultiplied) | Low | Compositing chains |
| Gradient inversion | Low | Data viz toggles |
| Procedural generators (heat, terrain) | Medium | Built-in presets |

## Performance Notes

| Scenario | Guidance |
|----------|----------|
| Many evaluations per frame | Precompute LUT |
| Dynamic key edits | Invalidate sorted flag; resort lazily |
| Small key counts (<=8) | Linear scan cheaper than binary search |

## Testing Strategy

- **Gradient**: Known 2-key endpoints; test mid t=0.5.
- **HSV Round Trip**: Random colors → HSV → RGB within epsilon.
- **Gamma Conversion**: Compare fast vs precise path error bounds.
- **LerpSRGB**: Compare result gamma-correct vs linear difference.

## Logging & Assertions

- Assert keyframe `t` within [0,1].
- Log warning if keys unsorted when evaluating (debug only).

## TODO / Backlog

- Finalize canonical color struct names and update docs.
- Add SIMD pack/unpack for RGBA8 <-> float conversions.
- Provide WCAG contrast utilities.
- Implement optional sRGB interpolation path.

---

*Generated documentation file; sync with future color system expansions.*
