# glTF 2.0 — Materials Reference

## Material System Overview

glTF uses **Physically Based Rendering (PBR)** with a **metallic-roughness** material model.
This ensures consistent rendering across platforms and renderers.

```json
{
  "materials": [
    {
      "name": "Gold",
      "pbrMetallicRoughness": {
        "baseColorFactor": [1.0, 0.766, 0.336, 1.0],
        "metallicFactor":  1.0,
        "roughnessFactor": 0.0
      },
      "normalTexture":    { "index": 2, "scale": 1.0, "texCoord": 0 },
      "occlusionTexture": { "index": 3, "strength": 1.0, "texCoord": 0 },
      "emissiveTexture":  { "index": 4, "texCoord": 0 },
      "emissiveFactor":   [0.0, 0.0, 0.0],
      "alphaMode":        "OPAQUE",
      "alphaCutoff":      0.5,
      "doubleSided":      false
    }
  ]
}
```

### Default Material

When a primitive specifies no material, a default material applies with all default values:

* No emission, no normal texture, no occlusion.
* Black appearance unless there is scene lighting (material emits nothing, diffuse ~0).

---

## PBR Metallic-Roughness Parameters

### pbrMetallicRoughness Object

| Property                     | Type         | Default   | Description                                     |
| ---------------------------- | ------------ | --------- | ----------------------------------------------- |
| `baseColorFactor`          | number[4]    | [1,1,1,1] | RGBA linear multiplier for base color           |
| `baseColorTexture`         | textureInfo  | —        | **sRGB**encoded RGB(A) texture            |
| `metallicFactor`           | number [0,1] | 1.0       | Linear multiplier for metalness                 |
| `roughnessFactor`          | number [0,1] | 1.0       | Linear multiplier for roughness                 |
| `metallicRoughnessTexture` | textureInfo  | —        | Packed: G=roughness, B=metalness ( **linear** ) |

### Material Interpretation

**Base Color:**

* Non-metal: reflected diffuse color.
* Metal: F0 specular reflectance value at normal incidence.

**Metalness (0→1):**

* 0.0 = dielectric (plastic, stone, wood).
* 1.0 = metal (gold, iron, copper).
* Intermediate values represent mixed regions (anti-aliasing, layering).

**Roughness (0→1):**

* 0.0 = perfectly smooth mirror.
* 1.0 = fully rough/diffuse.
* α = roughness² is used in microfacet distribution (perceptually linear).

### BRDF Model Summary

```
material = mix(dielectric_brdf, metal_brdf, metallic)

metal_brdf:
  conductor_fresnel(f0=baseColor, bsdf=specular_brdf(α=roughness²))

dielectric_brdf:
  fresnel_mix(ior=1.5, base=diffuse_brdf(color=baseColor), layer=specular_brdf(α=roughness²))
```

Microfacet model: **Trowbridge-Reitz/GGX** distribution with **Smith** masking-shadowing.

### Texture Combination Rule

When both factor AND texture are present:

```
final_value = factor × texture_sample
```

If texture only → factor defaults to 1.0 (neutral).
If factor only → texture is sampled as 1.0.

### COLOR_0 Attribute

When a primitive has `COLOR_0`, it is an **additional linear multiplier** on the base color:

```
final_base_color = baseColorFactor × texture_sample × COLOR_0
```

---

## Additional Textures

### Normal Texture

```json
"normalTexture": {
  "index": 2,
  "texCoord": 0,
  "scale": 1.0     // scales X and Y components of the sampled normal
}
```

* **Encoding** : linear RGB. Texels encode XYZ normal in tangent space.
* **Mapping** : `sampledValue × 2.0 − 1.0` → maps [0,1] to [−1,+1].
* Convention: +X=right, +Y=up, +Z=toward viewer.
* Blue channel values ≤ 0.5 are **invalid** (Z must be positive in tangent space).
* No alpha channel used (ignore if present).
* Scale formula: `scaledNormal = normalize(sampled_mapped × vec3(scale, scale, 1.0))`
* Normals MUST be re-normalized before use in lighting.

### Occlusion Texture

```json
"occlusionTexture": {
  "index": 3,
  "texCoord": 0,
  "strength": 1.0   // [0,1] reduces occlusion effect
}
```

* Only the **R channel** (red) is used. GBA channels MUST be ignored.
* 0.0 = fully occluded (no indirect lighting). 1.0 = no occlusion.
* Affects **indirect lighting only** (not direct).
* Strength formula: `occlusion = 1.0 + strength × (sampled_R − 1.0)`
* `strength=0.0` → no occlusion effect. `strength=1.0` → full effect.

### Emissive Texture + Factor

```json
"emissiveTexture": { "index": 4, "texCoord": 0 },
"emissiveFactor": [0.2, 0.1, 0.0]
```

* Texture: **sRGB** encoded. Must decode to linear before computation.
* Factor: linear RGB multiplier, each component in [0,1].
* Units when using physical light: candela per square meter (cd/m²).
* When undefined, emissive texture sampled as [1,1,1] RGB; factor default is [0,0,0].
* Alpha channel (if present) MUST be ignored.

### Texture Priority for Resource-Constrained Implementations

Drop bottom → top:

| Priority   | Texture   | Visual Impact                  |
| ---------- | --------- | ------------------------------ |
| Drop last  | Normal    | Less surface detail            |
| Drop mid   | Occlusion | Brighter in dark areas         |
| Drop first | Emissive  | Emissive light sources go dark |

---

## Alpha Modes

| alphaMode             | Behavior                                                         |
| --------------------- | ---------------------------------------------------------------- |
| `"OPAQUE"`(default) | Fully opaque; alpha value ignored                                |
| `"MASK"`            | Fully opaque if alpha ≥ alphaCutoff, else discard (transparent) |
| `"BLEND"`           | Alpha composite using Porter-Duff "over" operator                |

```json
"alphaMode": "MASK",
"alphaCutoff": 0.5   // only used with MASK; default 0.5; must be ≥ 0
```

Alpha value comes from the **4th component of base color** (baseColorFactor.a × texture.a).

### Rasterizer Notes

* **OPAQUE** : write depth for every pixel. No sorting needed.
* **MASK** : discard pixels below threshold; write depth for others. No sorting needed.
* **BLEND** : No perfect universal solution. Implementations may discard near-zero alpha
  pixels, may or may not write depth, may or may not sort.

---

## Double Sided

```json
"doubleSided": true
```

* `false` (default): back-face culling enabled; only front faces rendered.
* `true`: culling disabled; both faces rendered with  **reversed normals on back face** .

---

## Texture Infrastructure

### Texture Object

```json
{
  "textures": [
    {
      "source": 2,    // optional: index into images array
      "sampler": 0,   // optional: index into samplers array
      "name": "DiffuseMap"
    }
  ]
}
```

When `sampler` is undefined: use  **repeat wrapping + auto filtering** .
When `source` is undefined: extension must supply the texture.

### TextureInfo Object

Used whenever a material references a texture:

```json
{
  "index": 0,       // REQUIRED: texture index
  "texCoord": 0     // optional, default 0: which TEXCOORD_n to use
}
```

`texCoord: n` maps to vertex attribute `TEXCOORD_n`.

---

## Images

```json
{
  "images": [
    { "uri": "diffuse.png" },
    { "bufferView": 14, "mimeType": "image/jpeg" },
    { "uri": "data:image/png;base64,iVBORw0KGgo..." }
  ]
}
```

* External file: `uri` (relative path or data URI).
* Embedded in buffer: `bufferView` + `mimeType` (REQUIRED when using bufferView).
* `uri` and `bufferView` are  **mutually exclusive** .
* Supported formats: **PNG** (`image/png`) and **JPEG** (`image/jpeg`).
* Any colorspace info (ICC profiles, gamma) from PNG/JPEG MUST be **ignored** — glTF
  defines color space semantics through the material object referencing the image.

### Image Byte Signatures

| Media Type | Pattern                                           |
| ---------- | ------------------------------------------------- |
| image/png  | 0x89 0x50 0x4E 0x47 0x0D 0x0A 0x1A 0x0A (8 bytes) |
| image/jpeg | 0xFF 0xD8 0xFF (3 bytes)                          |

### Texture Coordinate Origin

```
(0,0) = upper-left corner
(1,1) = lower-right corner
```

---

## Samplers

```json
{
  "samplers": [
    {
      "magFilter": 9729,   // optional: LINEAR
      "minFilter": 9987,   // optional: LINEAR_MIPMAP_LINEAR
      "wrapS":    10497,   // optional, default 10497: REPEAT
      "wrapT":    10497,   // optional, default 10497: REPEAT
      "name": "MainSampler"
    }
  ]
}
```

### Magnification Filter

| Value | Name    | Description      |
| ----- | ------- | ---------------- |
| 9728  | NEAREST | Nearest neighbor |
| 9729  | LINEAR  | Bilinear         |

### Minification Filter

| Value | Name                             |
| ----- | -------------------------------- |
| 9728  | NEAREST                          |
| 9729  | LINEAR                           |
| 9984  | NEAREST_MIPMAP_NEAREST           |
| 9985  | LINEAR_MIPMAP_NEAREST            |
| 9986  | NEAREST_MIPMAP_LINEAR            |
| 9987  | LINEAR_MIPMAP_LINEAR (trilinear) |

### Wrapping Modes

| Value | Name             | Behavior                       |
| ----- | ---------------- | ------------------------------ |
| 33071 | CLAMP_TO_EDGE    | Clamp to edge pixel            |
| 33648 | MIRRORED_REPEAT  | Mirror and repeat              |
| 10497 | REPEAT (default) | Tile: fractional part of coord |

 **REPEAT** : 2.2 → 0.2; −0.4 → 0.6
 **MIRRORED_REPEAT** : 2.2 → 0.2; −0.4 → 0.4

### Mipmap Fallback

If runtime mipmap generation is not possible:

| Mipmap Mode            | Fallback |
| ---------------------- | -------- |
| NEAREST_MIPMAP_NEAREST | NEAREST  |
| NEAREST_MIPMAP_LINEAR  | LINEAR   |
| LINEAR_MIPMAP_NEAREST  | LINEAR   |
| LINEAR_MIPMAP_LINEAR   | LINEAR   |

### Non-Power-of-Two Textures

Platforms with limited NPOT support should resize when:

* Wrapping mode is REPEAT or MIRRORED_REPEAT, OR
* MinFilter uses mipmapping.

---

## Color Space Summary

| Texture                    | Transfer Function                            |
| -------------------------- | -------------------------------------------- |
| Base color texture         | **sRGB**→ decode to linear before use |
| Emissive texture           | **sRGB**→ decode to linear before use |
| Normal texture             | **Linear**                             |
| Occlusion texture          | **Linear**                             |
| Metallic-roughness texture | **Linear**                             |

**Always decode sRGB before filtering** for correct bilinear interpolation.

---

## Point and Line Primitive Materials

No formal size/style defined, but RECOMMENDED:

* Width: 1px in viewport space.
* With NORMAL + TANGENT → standard lighting including normal maps.
* With NORMAL but no TANGENT → standard lighting, ignore normal maps.
* Without NORMAL → unlit: `baseColor × COLOR_0 + emissive`.
