# glTF 2.0 — Animations Reference

## Animation Overview

glTF animations drive **node TRS transforms** and **morph target weights** via keyframe
data stored in buffers accessed through accessors.

glTF defines only **storage** — runtime behavior (looping, blending, ordering, auto-play)
is implementation-defined.

---

## Animation Object Structure

```json
{
  "animations": [
    {
      "name": "Walk",
      "channels": [ ... ],   // REQUIRED: animation channel array (≥1)
      "samplers": [ ... ]    // REQUIRED: animation sampler array (≥1)
    }
  ]
}
```

---

## Animation Channels

A channel connects a sampler to a target node property.

```json
{
  "channels": [
    {
      "sampler": 0,          // REQUIRED: index into this animation's samplers array
      "target": {
        "node": 1,           // optional: node index (if undefined, channel SHOULD be ignored)
        "path": "rotation"   // REQUIRED: property to animate
      }
    }
  ]
}
```

### Valid Path Values

| path              | Accessor Type | Component Types                                         | Description          |
| ----------------- | ------------- | ------------------------------------------------------- | -------------------- |
| `"translation"` | VEC3          | float                                                   | XYZ translation      |
| `"rotation"`    | VEC4          | float, SBYTE_norm, UBYTE_norm, SSHORT_norm, USHORT_norm | XYZW quaternion      |
| `"scale"`       | VEC3          | float                                                   | XYZ scale factors    |
| `"weights"`     | SCALAR        | float, SBYTE_norm, UBYTE_norm, SSHORT_norm, USHORT_norm | Morph target weights |

### Channel Rules

* Within one animation, each  **target (node + path) MUST NOT be used more than once** .
* Nodes without morph targets MUST NOT use `"weights"` path.
* Non-animated properties keep their original values during animation.

---

## Animation Samplers

An animation sampler defines a keyframe curve.

```json
{
  "samplers": [
    {
      "input": 4,              // REQUIRED: accessor index (timestamps, float SCALAR)
      "output": 5,             // REQUIRED: accessor index (output values)
      "interpolation": "LINEAR" // optional, default "LINEAR"
    }
  ]
}
```

### Input Accessor (Timestamps)

* Type: SCALAR, float.
* Values:  **time in seconds** .
* `time[0] >= 0.0`
* Strictly increasing: `time[n+1] > time[n]`
* **`min` and `max` REQUIRED** on this accessor.
* Times are relative to t=0 (start of this animation entry).

### Output Accessor

Depends on the channel path:

| path          | Type                           | Notes                                     |
| ------------- | ------------------------------ | ----------------------------------------- |
| "translation" | VEC3 float                     | XYZ in meters                             |
| "rotation"    | VEC4 float (or normalized int) | XYZW quaternion                           |
| "scale"       | VEC3 float                     | XYZ scale                                 |
| "weights"     | SCALAR (repeated)              | Count per frame = number of morph targets |

For `CUBICSPLINE`: output count = input count × 3 (in-tangent, value, out-tangent per key).

### Clamping Outside Range

Before and after the input time range, output values MUST be clamped to the nearest
endpoint. Example: if first keyframe is at t=10, clients must hold the first output value
for t < 10.

### Multiple Samplers Per Animation

Samplers within one animation MAY have different input (timestamp) accessors.

---

## Interpolation Modes

### LINEAR

```
f(t) = vₖ + (t - tₖ)/(tₖ₊₁ - tₖ) × (vₖ₊₁ - vₖ)
```

* output count = input count.
* For rotation (quaternions): use  **Spherical Linear Interpolation (SLERP)** .

### STEP

```
f(t) = vₖ   (hold until next keyframe)
```

* output count = input count.

### CUBICSPLINE

Uses Hermite cubic splines. For segment from keyframe k to k+1:

Let `t` = normalized time in [0,1], `Δt` = tₖ₊₁ − tₖ.

```
f(t) = (2t³ - 3t² + 1)·vₖ + (t³ - 2t² + t)·Δt·bₖ + (-2t³ + 3t²)·vₖ₊₁ + (t³ - t²)·Δt·aₖ₊₁
```

Where:

* `vₖ` = spline vertex at keyframe k
* `bₖ` = out-tangent at keyframe k
* `aₖ₊₁` = in-tangent at keyframe k+1

Output accessor layout per keyframe: `[in-tangent, value, out-tangent]`
So output accessor count =  **3 × input count** .

**At least 2 keyframes required** for CUBICSPLINE.

First in-tangent (`a₀`) and last out-tangent (`bₙ`) SHOULD be zero (not used).

For  **rotation with CUBICSPLINE** : the interpolated quaternion MUST be **normalized**
before applying to the node.

**Morph weight CUBICSPLINE layout** (n = morph target count per frame):

```
[a₁,a₂,…aₙ, v₁,v₂,…vₙ, b₁,b₂,…bₙ]  (per keyframe)
```

---

## SLERP for Rotation Quaternions

Used when `interpolation = "LINEAR"` on rotation path.

```
θ  = arccos(|q₀ · q₁|)
sign = sgn(q₀ · q₁)   // ensures short-path interpolation

slerp(t) = sin((1-t)θ)/sin(θ) × q₀  +  sin(t·θ)/sin(θ) × sign×q₁
```

When θ ≈ 0: degenerate → fall back to regular linear interpolation.

---

## Morph Target Weight Animation

For n morph targets, each animation frame has n scalar output values.
The output accessor size = frames × n scalars (packed end-to-end).

Example: 3 targets, 2 keyframes →

```
output[0..2]  = weights at frame 0
output[3..5]  = weights at frame 1
```

---

## Skinned Animation

Achieved by animating the **joint nodes** (translation/rotation/scale) referenced in the
skin's `joints` array. No special animation path — just TRS animation on the joint nodes.

---

## Animation Examples

### Animate one node's rotation

```json
{
  "animations": [{
    "name": "Spin",
    "channels": [{
      "sampler": 0,
      "target": { "node": 1, "path": "rotation" }
    }],
    "samplers": [{
      "input": 4,
      "interpolation": "LINEAR",
      "output": 5
    }]
  }]
}
```

### Animate morph targets + rotation on same node

```json
{
  "channels": [
    { "sampler": 0, "target": { "node": 1, "path": "rotation" } },
    { "sampler": 1, "target": { "node": 1, "path": "weights" } }
  ],
  "samplers": [
    { "input": 4, "interpolation": "LINEAR", "output": 5 },
    { "input": 4, "interpolation": "LINEAR", "output": 6 }
  ]
}
```

### Same sampler targeting two different nodes

```json
{
  "channels": [
    { "sampler": 0, "target": { "node": 0, "path": "rotation" } },
    { "sampler": 0, "target": { "node": 1, "path": "rotation" } }
  ],
  "samplers": [{ "input": 0, "interpolation": "LINEAR", "output": 1 }]
}
```

---

## Normalized Integer Decoding for Animation Output

| componentType  | int→float        | float→int      |
| -------------- | ----------------- | --------------- |
| BYTE (signed)  | max(c/127, −1)   | round(f×127)   |
| UNSIGNED_BYTE  | c/255             | round(f×255)   |
| SHORT (signed) | max(c/32767, −1) | round(f×32767) |
| UNSIGNED_SHORT | c/65535           | round(f×65535) |

---

## Implementation Notes

* Non-linear time inputs (e.g., 3ds Max/Maya time warps) are NOT representable — exporters
  SHOULD resample into linear time.
* Sparse accessors are recommended for morph target animation output (memory efficient).
* Best practice: each animation should be self-contained as an action (e.g., "Walk", "Run").
