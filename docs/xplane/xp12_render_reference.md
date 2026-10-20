# X-Plane 12 rendering: reference for the SEDX emulation

What Scenery Editor X's viewport has to reproduce to look like X-Plane 12, condensed from the research in
[`reports/X Plane 12 graphics pipeline.md`](../../reports/X%20Plane%2012%20graphics%20pipeline.md) (which cites every
source) and its notes in `research_notes/X Plane 12 graphics pipeline/`. The implementation plan is
[`plan/feature-xp12-environment-rendering-1.md`](../../plan/feature-xp12-environment-rendering-1.md).

Reference build: X-Plane **12.4.4-b4** (executable and shader archives compiled Oct 6 2026). The target is the
post-12.2 look (AgX, exposure fusion, recalibrated sky and clouds); anything tuned against 12.0 / 12.1 is stale.

**How this was learned, and the rule that follows.** Facts come from Laminar's public statements and specs, from
X-Plane's rendered output, from the names and formats visible in the shipped shader archives and executable, and from
the shipped art. X-Plane's shaders, binaries and art are Laminar Research's property: SEDX implements every pass from
published techniques and its own code, never by copying or translating Laminar's shaders, and loads X-Plane art only at
runtime from the user's install. This page describes behaviour in prose only.

Values marked *initial* are starting points that the parity harness (`tools/xp12_reference_probe`,
`tools/image_compare`) replaces with values measured from X-Plane.

---

## Frame

A Vulkan-only, always-HDR, deferred renderer. Inferred pass order (from executable labels and resource flow, to be
confirmed by a frame capture):

| # | Stage | What happens |
|---|---|---|
| 0 | Probes (when due) | Cube probes rendered and GGX / cosine filtered |
| 1 | Weather data | Cloud coverage and property volumes, a 3D signed distance field, cloud shadows |
| 2 | Atmosphere LUTs | Transmittance, multi-scattering, sky view, sky ambient; in-scatter and transmittance 3D volumes |
| 3 | Sun shadows | Depth-only cascades |
| 4 | G-buffer | Terrain, objects, water meta data; ocean FFT |
| 5 | Depth helpers | Min/max depth pyramid |
| 6 | Weather apply | One compute pass edits albedo, material and normal in place |
| 7 | Screen space | CACAO ambient occlusion; tiled light binning; screen-space reflections |
| 8 | Clouds and fog | Ray-marched clouds at reduced resolution, temporal upscale; froxel volumetric fog |
| 9 | Lighting | Deferred lighting, then overflow ("spilled") light volumes; water shading |
| 10 | Sky and composite | Sky dome, sun / moon / stars, volumetric apply; forward particles, light billboards |
| 11 | Post | HDR clamp, luminance histogram, exposure fusion, bloom, depth of field, tone map + FXAA, FSR 1 |

There are **no motion vectors and no temporal anti-aliasing**: AA is MSAA (optional) plus FXAA, upscaling is FSR 1.
A faithful emulation adds no temporal techniques, film grain, vignette, chromatic aberration or LUT grading (X-Plane has
none of these).

## G-buffer

| Target | Content |
|---|---|
| albedo | sRGB colour × coverage, alpha = coverage |
| material | R: a per-material lighting factor (meaning not settled); G: gloss; B: metalness / F0 value; A: coverage |
| normal | View-space normal (equal-area encoded) |
| emissive | Emissive luminance × the current exposure scale (pre-exposed) |
| weather | Rain / damp, puddle and snow receptivity (snow signed) |
| depth | Reverse-Z |

Every target is premultiplied by coverage × LOD fade so decals blend into the g-buffer.

## Materials

- Normal map channels: RG normal XY (Z rebuilt), **A gloss** (1 = smooth), **B = F0 / metalness** when the material has
  `NORMAL_METALNESS`; without it B is ignored. Translucency mode (`NORMAL_TRANSLUCENCY`, vegetation): B = translucency.
- One channel carries both: values 0-0.08 act as metalness *and* F0; above 0.08 it is metal with dielectric F0 0.08.
- Perceptual roughness = **1 − 0.96875 · gloss** (the smoothest surface is 1/32, not 0); GGX α = roughness².
- F0 without `NORMAL_METALNESS`: 0.06 for aircraft glass, 0.0 for translucency-mode normal maps, 0.04 for other normal
  maps or any gloss > 0, 0.0 for fully rough surfaces.
- Gloss is multiplied by `GLOBAL_specular` / `ATTR_shiny_rat` (`GLOBAL_specular 1.0` with a normal map is the norm).
- BRDF: GGX, **height-correlated Smith** visibility, Schlick Fresnel. The split-sum table is 256 × 256 RG32F indexed by
  (N·V, perceptual roughness); X-Plane's `Resources/brdf_lookup` matches this model.
- Albedo is sRGB. Emissive (`TEXTURE_LIT`) is in nits: `GLOBAL_luminance <nits>` (clamped at 65530), or
  `ATTR_light_level ... <nits>`; terrain uses `LUMINANCE` and `LIT_FADE_PARAMS`. Lights are **never dimmed by
  daylight**: exposure hides them.
- Undocumented but widely shipped: `SUPER_ROUGHNESS` (0.5 on draped scenery, 1.0 on forests; treated as a roughness
  floor until measured), `WEATHER <mask>`, `WEATHER_TRANSPARENT`, `SNOW_ALBEDO_LUMA a b c d`, `BLEND_HASH`,
  `SEASONAL <texture>`. Their exact meaning is calibrated with the SEDX_Calibration pack.

## Image-based lighting, reflections, AO, shadows

- Up to **8 cube probes** (cube array, 7 mips): specular LOD = roughness · 5 + bias, diffuse irradiance from mip 6
  (cosine filtered). Refreshed when the sun moves or the camera travels.
- Screen-space reflections: hierarchical-Z march into the previous frame's lit image, matrix reprojection, pixels below
  a minimum gloss skipped, probes as fallback; water traces too.
- Ambient occlusion: AMD FidelityFX CACAO. Presets: off; quality 2 half res; quality 3 half res (default); quality 4
  half res; quality 4 full res.
- Sun shadows: cascaded shadow maps (2D array, 1-4 cascades, practical split λ, soft PCF), resolved in deferred
  lighting; beyond the far limit, baked shadow from `TEXTURE_CONTROL` alpha. Cloud shadows: world-space, temporally
  filtered, up to 9 cascades. No screen-space contact shadows.

Quality levels (from X-Plane's `settings.txt`):

| Level | Exterior cascades | Far limit | Map size | Fade ratio |
|---|---|---|---|---|
| 0 | 1 (scenery shadows off) | 500 m | 2048 | 0.999 |
| 1 | 2 | 120 m | 1024 | 0.7 |
| 2 (default) | 3 | 250 m | 2048 | 0.7 |
| 3 | 3 | 500 m | 4096 | 0.7 |
| 4 | 4 | 1000 m | 4096 | 0.5 |

## Exposure and tone

- Units: luminance in nits, sun and moon illuminance in lux (`lighting/E_sun_lx`, initial 115,000 lx), lights in candela
  (`lights.txt` `375cd` syntax). HDR targets store radiance × the current exposure scale; a guard pass replaces
  infinities before post.
- Exposure: an EV100 camera (K = 12.5), metered by a 256-bin log-luminance histogram with spot weighting, clamped to
  **EV100 5-15** (the floor at 5 is why moonlight barely registers). An art-directed bias table (initial: +3.5 at EV 5,
  +3.0 at EV 8, 0 at EV 11-12, −0.3 at 12.33) and per-sky-condition tone ratios shape it.
- **Exposure fusion** (12.2): luminance-only local tone mapping from synthetic exposures blended through Gaussian /
  Laplacian pyramids (Mertens 2007 as adapted by Wronski 2022); the result rescales colour in the tone map pass.
- Tone curve: **AgX** (12.2, replacing an ACES / Uncharted2 blend), then an ASC-CDL grade (slope, offset, power,
  saturation; identity by default), output gain and gamma, sRGB swapchain.
- Bloom: thresholded, blurred over a mip range, deliberately subtle (reduced in 12.2).

## Atmosphere, haze, clouds

- Hillaire 2020 structure: transmittance (integrated over 40 steps), multi-scattering, sky-view and sky-ambient LUTs,
  and two aerial-perspective volumes (in-scatter, transmittance). Departures from the paper: **RGB** transmittance, a
  fourth **ground-fog** layer, a tabulated Mie phase, cloud shadows baked into the sky LUTs.
- Initial parameters: Rayleigh scattering (5.802, 13.558, 33.1)·10⁻⁶ m⁻¹, scale height 8 km; Mie scattering
  3.996·10⁻⁶, extinction 4.4·10⁻⁶, scale height 1.2 km, Cornette-Shanks g 0.8; ozone absorption
  (0.65, 1.881, 0.085)·10⁻⁶ m⁻¹ in a tent centred at 25 km, 30 km wide; ground albedo 0.1.
- Visibility is calibrated so a runway monolith is just visible at the set distance (contrast threshold, Koschmieder
  β = 3.912 / V) and, since 12.4.4, held in a surface layer that fades to clear air over an altitude band.
- 11 sky-condition classes (clean, hazy, foggy, sockd, ocast, strat, snowy, mount, hialt, orbit, final), each scaling
  ambient, direct, tone, Rayleigh, Mie and shadow.
- Clouds: three layers turned into 3D coverage / property volumes, accelerated by a signed distance field, ray marched at
  20-50 % of screen resolution (half at the default preset) and temporally upscaled. Froxel volumetric fog near the
  camera.

## Water

JONSWAP/TMA FFT ocean (four cascades) on a GPU projected grid, a slope-variance volume against distant sparkle, Fresnel
with F0 0.02, SSR plus probes, foam. Colour and clarity per 1° tile from `Resources/bitmaps/world/water/<tile>.png`
(fallback `any.png`, RGBA 13, 32, 51, 191; alpha is clarity). DSF water carries a fetch ratio (1 ocean, 0 pond) and a
depth ratio against the `sea_level` raster. Users set only wave height and direction.

## Night lighting

Tiled lights: 16 × 16 pixel tiles, 2 depth slices, up to **512 lights per tile**; overflow lights are drawn as additive
volumes (interior and exterior lists). Billboards are forward drawn, one per light, tinted like the spill, fogged by
aerial perspective. Airport light intensities follow FAA specification sheets.

## Weather surfaces

One compute pass after the g-buffer: **rain** darkens porous albedo and raises gloss; **puddles** flatten the normal
toward up with animated ripples and water-like reflectance; **snow** combines a world-projected coverage map, a slope
test, noise and per-material receptivity, and draws snow albedo / normal / decal textures; **ice** draws ice textures.
State comes from `sim/weather/region/runway_friction` (0 dry; 1-3 wet; 4-6 puddly; 7-9 snowy; 10-12 icy; 13-15 snowy
and icy), `snow_cover` and `rain_percent`. There is no persistent wetness map. No snow inside hangars (12.3), weather on
main roads (12.2).

## Seasons

Three layers: library exports per season (`EXPORT_SEASON`, `EXPORT_RATIO_SEASON`, `EXPORT_EXTEND_SEASON`,
`EXPORT_EXCLUDE_SEASON`, codes spr / sum / fal / win); DSF rasters of day-of-year ranges per season, interpolated;
`SEASONAL <texture>` naming the next season's albedo, blended by the season ratio. Winter textures carry no painted
snow (snow is the weather pass's job). There is no season dataref.

## Environment inputs

Date and time, latitude / longitude, visibility, three cloud layers (base, tops, coverage, type), rain, temperature,
runway friction, snow cover, wind, wave height and direction, season. X-Plane updates regional weather every 60 s unless
`update_immediately` is set.

## Art controls

X-Plane exposes roughly 1,000 rendering-related private art controls (`sim/private/controls/<group>/<name>`); their names
are listed in [`xp12_art_controls.txt`](xp12_art_controls.txt). Their defaults are not stored as text and are read from
a running X-Plane by the reference probe; SEDX keeps each matching tunable under the same relative name.
