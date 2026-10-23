# X-Plane 12 authoring-side material, lighting, season and weather rules (what a renderer must honor)

Scope note for the report writer: the public Laminar specs are badly out of date for scenery formats.
The .ter (2018), .pol (2015), .fac (2018), .lin (2019) and .str (2014) specs predate X-Plane 12, and
none of them document the XP12 material directives that the shipped 12.4 default scenery uses heavily
(`SUPER_ROUGHNESS`, `SNOW_ALBEDO_LUMA`, `WEATHER`, `WEATHER_TRANSPARENT`, `SEASONAL`, `NORMAL_TRANSLUCENCY`,
`BLEND_HASH`, `LUMINANCE`, `LIT_FADE_PARAMS`, `RUNWAY_TILE`, ...). Where the public docs are silent, these notes
use the locally installed X-Plane 12.4 content (`K:/SteamLibrary/steamapps/common/X-Plane 12`, DataRefs.txt
stamped Oct 6 2026) as primary evidence. Those findings are labeled "Local" and describe observed usage, not
documented semantics. The page index of current specs, with update dates, is at
[Current File Formats](https://developer.x-plane.com/docs/specs/).

OBJ version numbers map to sim versions: 1100 = XP 11.0, 1130 = 11.30, 1200 = XP 12.0, 1210 = 12.1, 1220 = 12.2
([OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/)).

## Q1. OBJ8 in X-Plane 12: what the material/light directives mean and what changed from XP11

### Takeaway
OBJ8 keeps the XP11 texture set (albedo `TEXTURE`, emissive `TEXTURE_LIT`, packed `TEXTURE_NORMAL` where RG = normal,
B = F0/metalness if `NORMAL_METALNESS`, A = gloss), and XP12 adds photometric emissive (`GLOBAL_luminance` in nits, a nits
`brightness` argument on `ATTR_light_level`), `TEXTURE_MAP` split maps, glass/rain/HUD directives (1200), and decals,
normal decals, modulators and OBJ `LOAD_CENTER` (1210). The normal-map channel meanings did not change from XP11.
What changed is how F0 is turned into metalness, and translucency is new.

### Cited Findings
**Textures and material**
- `TEXTURE <path>`: daytime albedo, mandatory; a blank name uses a gray proxy texture — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/)
- `TEXTURE_LIT <path>`: optional emissive overlay — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). Lit alpha is "allowed but not recommended" — [Standard Shading Options](https://developer.x-plane.com/article/standard-shading-options-for-x-plane-10-scenery/). If the emissive map has alpha, it is added to albedo alpha — [XP12 material model](https://developer.x-plane.com/article/the-x-plane-12-material-model)
- `TEXTURE_NORMAL <path>` (OBJ 940): normal map whose alpha holds "specular strength"; the OBJ form takes no scale/ratio argument in the OBJ8 spec — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). Shipped OBJs confirm it: `TEXTURE_NORMAL ../../textures/hangars_2_NML.png` (no ratio) — Local: default scenery .obj files. A ratio form `TEXTURE_NORMAL [ratio] [file]` exists only in the standard-shader formats (.ter/.pol/.net/.lin/.fac/.for/autogen); there the ratio is the map's resolution relative to albedo (4.0 = repeats 4x per albedo tile). That form accepts RGB (normals), RGBA (normals + specular level in A) or grayscale (specular only) — [Standard Shading Options](https://developer.x-plane.com/article/standard-shading-options-for-x-plane-10-scenery/)
- `NORMAL_METALNESS` (OBJ 1100), placed after `TEXTURE_NORMAL` or `TEXTURE_DRAPED_NORMAL`: the blue channel becomes base reflectance (reflection at normal incidence) — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). It takes no parameter, and the blue channel "must be ignored in legacy models" because tangent-space maps have strong blue values — [XP11 material model](https://developer.x-plane.com/article/x-plane-11-material-model/)
- Normal-map alpha is gloss: 1.0 is the glossiest surface and 0.0 the roughest. Every channel X-Plane reads is 8-bit (staff comment) — [XP11 material model](https://developer.x-plane.com/article/x-plane-11-material-model/)
- `TEXTURE_MAP <usage> <file>` (OBJ 1200; doc entry 11/09/22): usages `normal`, `material_gloss`, `gloss`. Cannot be combined with `TEXTURE_NORMAL`. The published channel table is garbled: `normal` = R Normal X, G Normal Y; `material_gloss` R = "Material-dependant"; the `gloss` row reads "gloss, gloss, gloss, Ignored" — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). Shipped use: `TEXTURE_MAP normal Chocks_NRM.png` + `TEXTURE_MAP material_gloss Chocks_MAT.png` (12 occurrences in default OBJs) — Local
- "GLOSS" is not an OBJ directive. It exists only as a `TEXTURE_MAP` usage — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/)
- `ATTR_shiny_rat <0..1>` (base OBJ8): specular ratio, exponent 128. The normal-map alpha is multiplied by `ATTR_shiny_rat` or by `GLOBAL_specular` (OBJ 1000), which defaults to 0.0. Use `GLOBAL_specular 1.0` with a normal map — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). Shipped default OBJs mostly use `GLOBAL_specular 1` (5,384 of 6,105 occurrences), plus some 0.2 and 0.05 — Local
- `BLEND_GLASS` (OBJ 1100): makes the diffuse part of glass translucent while keeping specular, "for reflections on clear glass" — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/)
- Deprecated: `ATTR_emission_rgb` (use lit textures), `ATTR_shade_flat` (bake face normals), `LIGHTS`/`VLIGHT` and the special colors 9.9/9.8/9.7 — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/)

**Emissive photometry (XP12)**
- `GLOBAL_luminance <nits>` (OBJ 1200; the spec spells it both "luminance" and "luminence"): baseline luminance of the LIT texture in nits, clamped at 65530 — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). Shipped values: 1000 (192x), 60 (141x), 100, 150 — Local
- `ATTR_light_level <v1> <v2> <dataref> [<brightness>]` (OBJ 930; brightness added for XP12): the dataref is mapped from [v1,v2] and clamped, and it overrides the sim's lighting decision. If brightness (nits) is given, photometric features are enabled and it is the LIT texture's maximum brightness. `ATTR_light_level_reset` restores the default — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). Shipped example: `ATTR_light_level 0 1 sim/graphics/VR/backlight_level` (no brightness arg) — Local
- The 1200 change-history line reads "v12 GLOBAL_luminance and photometric directives". The 11/03/21 revision added "new X-Plane 12 attributes, including rain and lighting" — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/)

**Draping, blending, shadows**
- `ATTR_draped`/`ATTR_no_draped` (OBJ 1000) bracket draped geometry, which is drawn separately from the normal OBJ. `ATTR_LOD_draped <max>` (1000) sets the draped draw distance — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). Draped geometry has its own `TEXTURE_DRAPED` / `TEXTURE_DRAPED_NORMAL` set; `NORMAL_METALNESS` applies to `TEXTURE_DRAPED_NORMAL` too — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/); usage: 1,310 `TEXTURE_DRAPED`, 1,046 `TEXTURE_DRAPED_NORMAL`, 665 `ATTR_layer_group_draped` — Local
- `ATTR_no_blend <cutoff>` (cutoff since 850, default 0.5) alpha-tests; `ATTR_blend` restores blending — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/)
- `ATTR_shadow_blend <ratio>` / `GLOBAL_shadow_blend <ratio>` (1000): blend the main pass but alpha-test the shadow pass at the cutoff. `ATTR_shadow`/`ATTR_no_shadow` (1010) toggle shadow casting per triangle range (default on). `GLOBAL_no_shadow` (1010) removes the whole object from shadows and cannot appear with `ATTR_shadow` in the same file — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/)

**Cockpit**
- `ATTR_cockpit` (8.x): panel texture + panel-click; flat lit unless `GLOBAL_cockpit_lit` (1010). Since 1100 the sim behaves as if `GLOBAL_cockpit_lit` is always set. `ATTR_cockpit_region <n>` (900): up to 4 panel regions, real 3D lighting, opaque. `ATTR_cockpit_lit_only` (1110): replaces only the emissive (screens). `ATTR_cockpit_device <name> <bus> <lighting channel> <auto_adjust>` (1100): replaces the lit texture with an avionics screen and keeps albedo + normal. `auto_adjust` 1 = brighten in daylight. `ATTR_manip_device` (12.1.0) adds touch-screen manipulators. `ATTR_hud_glass`/`ATTR_hud_reset` (1200) mark HUD glass — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/)
- Rain/thermal/wiper (aircraft): `RAIN_scale` (1200), `RAIN_friction` (12.1), `THERMAL_texture`, `THERMAL_source` (1200, updated 1210), `THERMAL_source2` (12.1), `WIPER_texture` (RGBA = one wiper per channel), `WIPER_param` (1200) — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/); [Windshield Rain Effects](https://developer.x-plane.com/article/windshield-rain-effects/)

**Decals / detail (OBJ 1210 = 12.1.0)**
- The 1210 change history adds "access to detailing extensions including decals", referring to the Standard Shading Options document — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). Decals and normal-map decals in .obj files are 12.1.0+ — [Standard Shading Options](https://developer.x-plane.com/article/standard-shading-options-for-x-plane-10-scenery/). The full decal system, including normal-map decals, works in OBJs, including aircraft OBJs (Ben Supnik, Feb 24 2024) — [A Few Notes on 12.1.0](https://developer.x-plane.com/2024/02/a-few-notes-on-12-1-0-for-developers/)
- Aircraft "Detail Textures" (12.1): up to two slots, each with an albedo DT and/or a normal DT. The modulator is an R/G mask (R = DT1, G = DT2). Normal DTs use only R and G. Albedo DTs may use alpha. The blend mode is fixed by the shader — [Detail Textures for Aircraft](https://developer.x-plane.com/article/detail-textures-for-aircraft/)
- Directive syntax (shared standard-shader set) is in Q4. Shipped OBJs use `DECAL_LIB` (2,044), `TEXTURE_MODULATOR` (901), `TEXTURE_CONTROL`/`TEXTURE_TERRAIN` (28 each), `DECAL_PARAMS` (1) — Local

**Particles**
- `PARTICLE_SYSTEM <pss>` (OBJ 1130, one per OBJ) + `EMITTER <name> <x y z> <psi the phi> [index]` — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). In 12.1.0 the XP8 smoke-puff directive and the old particle system were removed. Particle systems from DSF-placed objects were allowed, conditional on passing testing — [A Few Notes on 12.1.0](https://developer.x-plane.com/2024/02/a-few-notes-on-12-1-0-for-developers/)

**Lights in OBJ**
- `LIGHT_NAMED <name> <x y z>` (850): preset light. `LIGHT_PARAM <name> <x y z> [params]` (940): extra parameters defined per light. `LIGHT_CUSTOM <x y z r g b a s s1 t1 s2 t2 dataref>` (850): textured billboard from the OBJ texture. `LIGHT_SPILL_CUSTOM <x y z r g b a s dx dy dz semi dref>` (1000): s = full-attenuation distance in m, (0,0,0) direction = omni, semi = cos(half cone), dref = 9-float dataref or "none" — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/)
- For parameterized lights, XYZ always comes first. The count and meaning of the extra parameters depend on the light name — [Airplane Parameterized Light Guide](https://developer.x-plane.com/article/airplane-parameterized-light-guide/); see Q3 for the XP12 photometric parameter lists

**Other XP12 additions**
- `ATTR_landing_gear` (1220, 03/22/25), `LOAD_CENTER` for OBJs (1210) — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). OBJ `LOAD_CENTER` makes texture resolution scale with distance, aimed at one-off landmark OBJs — [A Few Notes on 12.1.0](https://developer.x-plane.com/2024/02/a-few-notes-on-12-1-0-for-developers/)
- Load-time conditionals: `GLOBAL_LIGHTING` (true when HDR on), `GLOBAL_SHADOWS`, `SCENERY_SHADOWS`. They do not use datarefs, and the file reloads when render settings change — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/)
- Shipped 12.4 default OBJs also use directives the OBJ8 spec does not document: `WEATHER <mask>` (234), `WEATHER_TRANSPARENT` (296), `SNOW_ALBEDO_LUMA` (109), `SUPER_ROUGHNESS` (33), `NORMAL_TRANSLUCENCY` (12), `SPECULAR` (1,067), `DITHER_ALPHA` (84), `NO_BLEND` (370), `TILTED` (1,724) — Local: grep of `Resources/default scenery/**/*.obj`

**What changed from XP11**
- XP12 renders intermediate metals (F0 about 0.25 to 0.75) like Substance Painter 2 / Blender. XP11 added "a lot of extra specularity", so XP11-tuned intermediate metals look brighter and more washed out in XP12 and may need albedo changes or re-bakes. XP12 also adds photometric calibration of emissive and an optional translucency mode, which was effectively always off in XP11 — [XP12 material model](https://developer.x-plane.com/article/the-x-plane-12-material-model)

### Inferences
- A loader should keep three normal-map interpretations apart: legacy (B ignored, rebuild Z), `NORMAL_METALNESS` (B = F0/metalness), and the `TEXTURE_MAP` split. Alpha is gloss (smoothness) in all three, so a roughness-based BRDF needs a conversion.
- The OBJ `TEXTURE_NORMAL` has no ratio. Scenery-format parsers must not reuse the OBJ parser's argument count for `TEXTURE_NORMAL`.
- XP12 OBJs are effectively a superset of the scenery standard shader: weather, snow and roughness directives appear in OBJ headers too. The parser should accept them even though the OBJ spec omits them.

### Gaps
- The exact channel layout for `TEXTURE_MAP material_gloss` and `gloss` is ambiguous on the live page; it needs a Laminar exporter or an XPlane2Blender source read.
- No public document defines `SUPER_ROUGHNESS`, `SNOW_ALBEDO_LUMA`, `WEATHER`, `WEATHER_TRANSPARENT`, `NORMAL_TRANSLUCENCY` or `TILTED` for OBJs (an extended web search found nothing).
- The `TEXTURE_LIT` alpha usage and how `GLOBAL_luminance` interacts with `ATTR_light_level` brightness when both are present are not specified.

## Q2. X-Plane 12's PBR model as documented

### Takeaway
XP12 uses a Disney/UE4-style metalness workflow with one combined F0/metalness channel, untinted dielectric Fresnel reflection,
albedo-tinted metal reflection, gloss (not roughness) authored in normal-map alpha, sRGB albedo, an HDR 16-bit float
pipeline with auto-exposure and a film-like tone mapper, and emissive calibrated in nits. Laminar publishes no BRDF equations
and no gloss-to-roughness formula.

### Cited Findings
- The model is an industry-standard PBR metalness workflow matching the Disney/UE4 model, Substance Painter 2 and Blender's Principled BSDF. It covers all OBJs, aircraft, all scenery elements and plugin instanced OBJs — [XP12 material model](https://developer.x-plane.com/article/the-x-plane-12-material-model)
- Metalness and F0 share one channel "for performance reasons". F0 in [0, 0.08]: used as both metalness and F0. F0 > 0.08: metalness = F0 and the dielectric F0 is clamped to 0.08. Near F0 0.9 or higher the dielectric part is very low. Dielectric reflection is untinted Fresnel from F0, with the rest diffuse tinted by albedo. Metal reflection is albedo-tinted (an approximation) — [XP12 material model](https://developer.x-plane.com/article/the-x-plane-12-material-model)
- Default F0 without `NORMAL_METALNESS`, in priority order: (1) `BLEND_GLASS` material on an aircraft in a glass lighting mode (interior glass, exterior glass, exterior rain glass, interior reflective glass) = 0.06; (2) translucency-mode normal map = 0.0; (3) any other normal map = 0.04; (4) no normal map but gloss > 0 = 0.04; (5) no normal map at default maximum roughness = 0.0 — [XP12 material model](https://developer.x-plane.com/article/the-x-plane-12-material-model)
- XP11 equivalent: without metalness, base reflectance 0.04 is applied to the whole material, "a good approximation for most dielectrics" — [XP11 material model](https://developer.x-plane.com/article/x-plane-11-material-model/)
- Roughness/gloss comes from a texture map (recommended) or a hard-coded value in the art asset file — [XP12 material model](https://developer.x-plane.com/article/the-x-plane-12-material-model). Alpha 1.0 = glossiest, 0.0 = roughest. Total reflected energy stays the same as roughness changes. The glossiest XP11 value was set to match the most reflective XP10 material — [XP11 material model](https://developer.x-plane.com/article/x-plane-11-material-model/)
- Fresnel: surfaces get more reflective at glancing angles, the authored value is the head-on minimum, and diffuse drops as reflectivity rises. The XP11 article itself flags the view-angle diffuse reduction as "probably a bug" — [XP11 material model](https://developer.x-plane.com/article/x-plane-11-material-model/)
- Optional single map channel (at most one per material): base F0 (unused with `NORMAL_METALNESS`), translucency, or separate normal alpha ("normal alpha defaults to albedo alpha"), which lets normal-map decals affect terrain without full materials. Translucency (new in XP12) brightens back-lit diffuse and limits front diffuse, meant for vegetation, default 0 — [XP12 material model](https://developer.x-plane.com/article/the-x-plane-12-material-model)
- Baked AO exists only for certain specialized assets and is not available on OBJs (including aircraft/plugin OBJs) — [XP12 material model](https://developer.x-plane.com/article/the-x-plane-12-material-model)
- Color space: albedos are authored in sRGB and the working gamut stays sRGB — [Photometric Lighting (Supnik, Nov 3 2021)](https://developer.x-plane.com/2021/11/photometric-lighting-what-is-it-and-why-do-we-need-it/)
- The pipeline is HDR "from start to finish" with 16-bit float encoding. A camera model with exposure value (EV) maps to the display, art staff set EV by time of day and weather, and auto-exposure adapts to view direction. A tone mapper tuned for a film-camera look compresses highlights, with an internal control for highlight desaturation. Luminance is in cd/m² (nits). Reference levels: night building floodlights 2 nt, LDR monitor 80 nt, clear sky 7,000 nt, clouds 10,000 nt, sunset sun 600,000 nt, noon sun 1.6e9 nt — [Photometric Lighting](https://developer.x-plane.com/2021/11/photometric-lighting-what-is-it-and-why-do-we-need-it/)
- Emissive is "calibrated to photometric units using a directive in the art asset text file" (new in XP12) — [XP12 material model](https://developer.x-plane.com/article/the-x-plane-12-material-model). The OBJ directive is `GLOBAL_luminance <nits>`, and `ATTR_light_level ... <brightness nits>` sets the LIT maximum — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). Terrain uses `LUMINANCE 600` alongside `TEXTURE_LIT` (49 .ter files), undocumented — Local
- Standard-shader specular rules: `SPECULAR [0..1]` multiplies the normal map's specular channel. Defaults: 1.0 with a normal map, 0.0 (matte) with no normal map. With no normal map but `SPECULAR` present, specular is uniform. `BUMP_LEVEL [0..1]` scales bump height (default 1.0) — [Standard Shading Options](https://developer.x-plane.com/article/standard-shading-options-for-x-plane-10-scenery/)
- In v12 no light source is ever dimmed by the sun; "the camera itself change[s]". `light_attenuation_2d` still exists for 2D panels — Local: [DataRefs.txt `sim/graphics/misc/light_attenuation`](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/plugins/DataRefs.txt)

### Inferences
- To match XP12: decode albedo as sRGB to linear, read gloss from alpha and convert it to roughness with a monotone mapping (XP's curve is unpublished; 1 - gloss, then squared to alpha as in UE4, is the closest documented-compatible assumption). Decode F0 per the 0.08 split rule (metalness = F0, dielectric F0 = min(F0, 0.08)). Do not apply the XP11 boosted specular for 0.25-0.75 metals.
- Emissive should be written to the HDR target as `lit_rgb (linearized) * nits`, scaled by `ATTR_light_level` (and clamped to 65530 nits), then auto-exposed. Do not apply a day/night dimming factor; exposure handles it.

### Gaps
- No published BRDF (NDF, geometry term, diffuse model) or gloss-to-roughness mapping.
- Whether `TEXTURE_LIT` is sampled as sRGB and how nits multiply it (peak = white texel?) is not stated. `GLOBAL_luminance` defaults when absent are unknown.
- The `SUPER_ROUGHNESS` semantics (shipped as 0.5 or 1.0 on most scenery) are undocumented. It may be a roughness floor or scale, but this is unconfirmed.
- The exposure/tone-map curve parameters are not public.

## Q3. Lights: photometric units, lights.txt, spill vs billboard, intensity, day/night

### Takeaway
XP12 lights are photometric. In the shipped lights.txt, the billboard "size" column became intensity in candela (values written like
`375cd`, `20000cd`, `95000cd`), and new `_pm` (photometric) parameterized lights take an `INTENSITY` parameter. Spill and billboard
remain separate records. Scene luminance is in nits, and the sun never dims lights; camera exposure does.

### Cited Findings
- lights.txt is an internal file tied to the X-Plane code. Laminar warns that modifying it may crash the sim and breaks across versions — Local: [lights.txt header](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/world/lites/lights.txt); it is "absolutely not meant for editing by the user" — [lights.txt File Format (Jun 10 2020)](https://developer.x-plane.com/article/lights-txt-file-format/)
- Format: header `A` / `850` / `LIGHT_SPECS` / `TEXTURE` lines; records are `LIGHT_PARAM_DEF <name> <count> <params...>` (optional, must precede overloads) and one or more OVERLOAD records. Column orders: `BILLBOARD_HW` R G B A SIZE CELL_SIZE X_CELL Y_CELL DX DY DZ WIDTH FREQ PHASE AMP DAY; `BILLBOARD_SW` ... WIDTH DREF; `SPILL_HW_DIR` R G B A SIZE DX DY DZ WIDTH DAY; `SPILL_HW_FLA` R G B A SIZE FREQ PHASE AMP DAY; `SPILL_SW` R G B A SIZE DX DY DZ WIDTH DREF; `SPILL_GND`/`SPILL_GND_REV` SIZE CELL... (v8 runway ground quads). Overload args may be numbers, a dataref/NOOP, or a parameter name from `LIGHT_PARAM_DEF`. Non-parameterizable columns are listed per type (e.g., `BILLBOARD_HW` A/CELL/AMP/DAY) — [lights.txt File Format](https://developer.x-plane.com/article/lights-txt-file-format/)
- Semantics: RGBA is a tint, and since light is additive A is a master dimmer. Spill SIZE is in meters; billboard SIZE is a tuned constant (pre-XP12 doc). Default direction is +Z (south). WIDTH is 0..1 omni-ness for billboards and cos(half view angle) for spills. Negative billboard size means axial. DAY 1 = on day and night, 0 = night only. FREQ is in Hz, PHASE in seconds, AMP < 1000 gives a triangle or strobe, AMP > 1000 a square wave with duty (amp - 1000)/1000 — [lights.txt File Format](https://developer.x-plane.com/article/lights-txt-file-format/); same notes in the Local lights.txt header
- XP12 photometric change, from the shipped file: "Intensity: Intensity for V12 lights expressed in Candela (cd)" and "FROM V12 THERE IS ONLY ONE BILLBOARD X LIGHT, TINTED WITH SAME RBG VALUES OF THE SPILL, AND SIZE BECOME INTENSITY EXPRESSED IN CANDELA (cd)". Sections are headed "V12 AIRPLANE LIGHTS ... all photometric" and "Sergio's REAL airport lights ... NOW UPGRADED TO V12 WITH INTENSITY IN CANDELA" — Local: [lights.txt](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/world/lites/lights.txt)
- Example intensities: car taillight `BILLBOARD_HW`/`SPILL_HW_DIR` 375 cd; car headlight billboard 3,000 to 20,000 cd with a 20,000 cd directional spill (WIDTH 0.965); airport white axial beacon 95,000 cd — Local: lights.txt
- XP12 airplane parameterized lights (`_bb` billboard, `_pm` photometric spill): `airplane_{landing,taxi,spot,generic,nav,strobe,beacon}_{bb,pm}` each take `R G B INDEX INTENSITY DX DY DZ WIDTH` (9 params) and are driven by `sim/graphics/animation/lights/airplane_*_light_spill`. Legacy `_sp` spills take `R G B INDEX SIZE WIDTH`. `airplane_generic_static` takes `R G B INTENSITY DX DY DZ WIDTH` — Local: lights.txt `LIGHT_PARAM_DEF` (97 defs)
- Scenery photometric parameterized lights: `spot_params_sp_pm` (R G B A INTENSITY DX DY DZ WIDTH), `spot_params_bb_pm` (R G B INTENSITY DX DY DZ WIDTH), `*_day_pm` variants, `area_lt_param_bb_pm`/`_sp_pm` (DX DY DZ INTENSITY), `flood_merc_XYZTSB_pm` / `flood_LPS_XYZTSB_pm` (DX DY DZ A INTENSITY), `LPS_param_sp_pm` (DX DY DZ A INTENSITY WIDTH), `helipad_flood_sp_pm` (INTENSITY DX DY DZ WIDTH), `apron_light_billboard` (DX DY DZ WIDTH INTENSITY) — Local: lights.txt
- Record counts in 12.4 lights.txt: 366 `BILLBOARD_HW`, 115 `BILLBOARD_SW`, 240 `SPILL_HW_DIR`, 80 `SPILL_HW_FLA`, 34 `SPILL_SW`, 24 `SPILL_GND`, 13 `SPILL_GND_REV`; billboard atlas `1000_lights_close.dds`, 16x8 cells — Local
- Billboard vs spill: X-Plane always provides separate billboard and spill parameterized lights, and you use one of each. A spill may stand alone for recessed bulbs. Spill cost scales with covered area, so prefer one large spill. Interior lights should be spill-only. Brightness follows switch and brightness datarefs indexed by INDEX (e.g., `sim/cockpit2/switches/generic_lights_switch[i]`, `sim/flightmodel2/lights/generic_lights_brightness_ratio[i]`) — [Airplane Parameterized Light Guide (Jan 2018, pre-XP12)](https://developer.x-plane.com/article/airplane-parameterized-light-guide/)
- Units of the scene: luminance in cd/m² (nits). Light sources are specified in real-world units, so aircraft authors can use manufacturer specs — [Photometric Lighting](https://developer.x-plane.com/2021/11/photometric-lighting-what-is-it-and-why-do-we-need-it/)
- Day/night: in v12 no light source is dimmed by the sun; the camera adapts. `light_attenuation` remains only for compatibility semantics — Local: DataRefs.txt. `sim/graphics/scenery/percent_lights_on` (city lights as night falls) and `airport_light_level` / `airport_lights_on` exist as datarefs — Local: DataRefs.txt
- The user manual still states spill lights "only function if HDR is enabled", tied to the top two Visual Effects settings — Local: [X-Plane Manual.html](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Instructions/X-Plane%20Manual.html). This may be stale XP11 text: Supnik describes XP12 as HDR throughout — [Photometric Lighting](https://developer.x-plane.com/2021/11/photometric-lighting-what-is-it-and-why-do-we-need-it/)
- Legacy billboard size/decay constants (`LIGHT_PARAMS 0.69 0.08 0.005 1.0`: decay exponent, scale, glare-fade distance, linear glare scale) are commented out in the 12.4 file — Local: lights.txt

### Inferences
- A renderer should treat a billboard's candela value as luminous intensity: screen brightness of the sprite = I / d² folded into the exposure. Treat a spill's candela plus cone (WIDTH = cos half-angle) as a spot/point light with physical inverse-square falloff. A pre-XP12 numeric SIZE in the same column (no `cd` suffix) still means meters for spills.
- `_pm` most likely means "photometric" (it pairs with `_bb` and replaces `_sp`). This is inferred from naming and the V12 section header, not stated.
- The OBJ `LIGHT_SPILL_CUSTOM` / `LIGHT_CUSTOM` signatures are unchanged in the spec, so their brightness must be calibrated by the renderer (no documented candela field).

### Gaps
- No current (XP12) public lights.txt or parameterized-light doc. The 2020 format doc and 2018 guide do not mention candela, `_pm`, or `INTENSITY`.
- No documented XP12 falloff law, billboard sprite-size rule, or glare model for candela lights.
- Day/night switching of scenery lights (photocell thresholds) is not documented beyond the DAY flag and the datarefs above.

## Q4. Scenery formats (.ter, .pol, .net, .fac, .for, .agp, .lin, .str): material directives and XP12 additions

### Takeaway
All draped/scenery formats share the "Standard Shading Options" command set (albedo/lit/normal with a ratio, `SPECULAR`, `BUMP_LEVEL`,
up to two decals, 12.1 normal decals, control/terrain/modulator detailing, tiling, dither/no-blend, `NO_SHADOW`). XP12 content adds a
layer of undocumented directives for roughness, snow, weather masks, seasons, translucency, runway tiling and lit fading.
`.str` has no material directives; it references OBJs.

### Cited Findings
**Shared standard-shader commands** — [Standard Shading Options (updated Oct 10 2024)](https://developer.x-plane.com/article/standard-shading-options-for-x-plane-10-scenery/)
- Texturing: `TEXTURE`, `TEXTURE_NOWRAP` (.ter/.pol only), `TEXTURE_LIT`, `TEXTURE_LIT_NOWRAP`; `TEXTURE_NORMAL [ratio] [file]`, `TEXTURE_NORMAL_NOWRAP`; `SPECULAR`, `BUMP_LEVEL` (see Q2)
- Decals (max two per shader; a .dcl may contain two, using the full budget): `DECAL [scale] [file]` (alpha-only grayscale, takes albedo tint); `DECAL_RGBA [scale] [file]`; `DECAL_KEYED [scale] [r g b a] [alpha] [file]` (albedo-color keyed); `DECAL_PARAMS [scale] [dither] [RGBA m k] [RGBA m k] [file]` (first 6 = RGB decal, second 6 = alpha decal; m = modulator, k = constant); `DECAL_PARAMS_PROJ [scale_x] [scale_y] ...` (repeats every N meters instead of following albedo UVs); `DECAL_LIB [path]` (library .dcl)
- Normal decals (12.1.0+): `NORMAL_DECAL [scale] [file] [gloss]`, `NORMAL_DECAL_KEYED [scale] [r g b a] [file] [gloss]`, `NORMAL_DECAL_PARAMS [scale] [r g b a] [m] [k] [file] [gloss]`, `NORMAL_DECAL_PARAMS_PROJ [sx] [sy] [r g b a] [m] [k] [file] [gloss]`. The "gloss" argument is not defined
- Alpha: `NO_ALPHA`; `DITHER_ALPHA [softness 0..1 exclusive] [bleed]`; `NO_BLEND [cutoff]` (avoids sky halos on vegetation). Shadows: `NO_SHADOW`. `TWO_SIDED [1/0]` (not in .for; forest billboards are always two-sided)
- Tiling: `TEXTURE_TILE [x_tiles] [y_tiles] [x_pages] [y_pages] [map]` (R/G of the map pick tile offsets; noise used if no map; 256x256 pages recommended); available in .pol, .net, .ter, .bch
- Detailing: `TEXTURE_DETAIL [x_res] [y_res] [file]`, `TEXTURE_NORMAL_DETAIL`; `TEXTURE_CONTROL [file]` (R = RGB decal + 1st normal decal (12.1+); G = terrain layer shown under translucent albedo; B = alpha decal + 2nd normal decal (12.1+); A = baked shadow, 0 = most shadowed, used only beyond dynamic shadow range); `TEXTURE_TERRAIN [file]` (tiling layer under the albedo); `TEXTURE_MODULATOR [file]` (12.1.0+, replaces the control map's terrain + baked-shadow channels to save VRAM; its R maps to control R and its G to control B). Decal strength = m x control R (or B); with a second decal the roles swap. Detailing is available in autogen, .pol, .net, .ter and OBJ8
- The page's availability table says facades and .lin get "none of the extensions". The shipped 12.4 content contradicts this: .fac uses `DECAL_LIB` (773), `TEXTURE_MODULATOR` (571), `NORMAL_DECAL_PARAMS`; .lin uses `DECAL_LIB` (436), `TEXTURE_MODULATOR` (58), `NORMAL_DECAL_PARAMS` (41), `TEXTURE_CONTROL`/`TEXTURE_TERRAIN` — Local: grep of default scenery
- Decals are overlays that modulate albedo for high-frequency detail. Grayscale `DECAL` blends like Photoshop "hard light". Far away, a decal averages out so the result matches the albedo ("no effect"). Scale is a ratio to the base texture. Results degrade if adjacent scale layers differ by more than roughly 100 to 150x — [Using Decals (May 2014)](https://developer.x-plane.com/article/using-decals-to-add-detail-to-scenery/)
- .dcl file layout (observed): `A` / `1000` / `DECAL`, then `DECAL_PARAMS[_PROJ]` / `NORMAL_DECAL_PARAMS[_PROJ]` lines and an optional `DITHER_ALPHA`. Example: `DECAL_PARAMS_PROJ 4 4 -0.5 0 0 0 0 1.0 0 0 0 0 0 0.7 0 textures/grass_and_asphalt_DTL.png` — Local: `Resources/default scenery/1000 decals/AGB_grass_and_asphalt_1.dcl`

**.ter** — [Terrain Type spec (Jun 28 2018)](https://developer.x-plane.com/article/terrain-type-ter-file-format-specification/)
- `BASE_TEX`, `LIT_TEX` (must be explicit; no `_LIT` inference), `BORDER_TEX`, `COMPOSITE_TEX` (deprecated as of XP9), `NO_ALPHA`, `PROJECTED <x_m> <y_m>`, `BUMPY` (802), `WET` (802, physics-only water), `COMPOSITE_BORDERS` (820, dithered fades), `*_NOWRAP` variants (820), `PROJECT_ANGLE <x y z a>` / `PROJECT_OFFSET` (820), `SURFACE` (850), `AUTO_VARY` (900), `LOAD_CENTER <lat> <lon> <size_m> <px>` (920)
- XP12 water via .ter: `WATER_COLOR_MASK` — [DSF Usage](https://developer.x-plane.com/article/dsf-usage-in-x-plane/) (see Q8)
- Shipped 12.4 .ter usage (9,848 files): `DECAL_LIB` (7,442), `SUPER_ROUGHNESS` (6,960, value 0.5), `COMPOSITE_TEX`/`COMPOSITE_PARAMS`/`COMPOSITE_NOISE`/`COMPOSITE_PROJECTED` (about 2,000; still used in XP12 despite the old deprecation, with 6 or 7 floats), `AUTO_SLOPE*`/`AUTO_HEADING`, `NORMAL_TEX <ratio> <file>`, `TEXTURE_NORMAL <ratio> <file>`, `NORMAL_METALNESS`, `BUMP_LEVEL`, `TEXTURE_LIT` + `LIT_FADE_PARAMS 8192 7000 15000` + `LUMINANCE 600`, `FAR <near> <far> <ratio> <tex>` (63), `AUTO_VARY` (263), `TEXTURE_TILE` (730), `SNOW_ALBEDO_LUMA` — Local; example `terrain12/fal/urban_tmp_wet10.ter`

**.pol** — [Draped Polygon spec (Mar 2 2015)](https://developer.x-plane.com/article/draped-polygon-polfac-file-format-specification/)
- `TEXTURE`, `TEXTURE_NOWRAP` (860), `TEXTURE_LIT`, `SCALE <h_m> <v_m>` (ignored if the DSF supplies ST), `LAYER_GROUP <group> <offset>`, `SURFACE`, `NO_ALPHA` (860), `LOAD_CENTER` (920). The page does not mention XP12
- Shipped 12.4 .pol extras: `DECAL_LIB` (7,724), `SUPER_ROUGHNESS` (6,995), `WEATHER <mask>` (165), `WEATHER_TRANSPARENT` (20), `SNOW_ALBEDO_LUMA` (56), `RUNWAY_TILE <x> <y> <px> <py> <noise.png>` (e.g., `4 8 256 512 ../maps/noise_4x4_V4-bars_1.png`), `RUNWAY_NOISE_PARAMS` (4 or 5 floats), `RUNWAY_MARKINGS <4 floats> <png>`, `SKIP_WATER` (3), `NORMAL_DECAL_PARAMS[_PROJ]`, `DITHER_ALPHA`, `SPECULAR` — Local

**.net** — [Vector Network spec (Jan 30 2024)](https://developer.x-plane.com/article/vector-network-net-file-format-specification/): documents geometry (road types, `SEGMENT_DRAPED`, `ROAD_DRAPED`, etc.) and `TEXTURE_LIT` ("X-Plane will not automatically find _LIT bitmaps without this command"). No XP12 shading directives, lights, wet/snow or shadows. Shipped 12.4 .net files add `TEXTURE_NORMAL`, `NORMAL_METALNESS`, `DECAL_LIB`, `DECAL_PARAMS_PROJ`, `TEXTURE_CONTROL`/`TERRAIN`, `WEATHER` (37), `SPECULAR`, `NO_BLEND`, `NO_ALPHA`, `DITHER_ALPHA`, `TEXTURE_TILE` — Local

**.fac** — The public spec is the "X-Plane 10 Facade" page (Feb 5 2018) — [Current File Formats](https://developer.x-plane.com/docs/specs/). Shipped 12.4 .fac files use `SHADER_WALL` / `SHADER_ROOF` / `SHADER_FLOOR` / `SHADER_CEILING` to scope materials, plus `TEXTURE_NORMAL`, `NORMAL_METALNESS`, `TEXTURE_MODULATOR`, `DECAL_LIB`, `TEXTURE_LIT`, `WEATHER` (105), `SNOW_ALBEDO_LUMA` (45), `NO_BLEND`, `TWO_SIDED`, `SPECULAR` — Local

**.for** — [Forest spec (updated 11/08/22 for 1200)](https://developer.x-plane.com/article/forest-for-file-format-specification/)
- XP12: `TREE2` (adds nominal height and per-tree far LOD), `MESH_3D` (up to 3 meshes / 4 LOD steps, last step must be a 2D billboard), `MESH <name> <near> <far> <vcount> <idxcount> <bend_scale> <max_disp> <disp_speed> [NO_SHADOW]` (wind bending; `VERTEX` carries 3 weights: displacement, branch attenuation, phase), `SHADER_2D`/`SHADER_3D` select the material being edited (2D default = double-sided, alpha test 0.5, the XP11 behavior). `LOD` = min(file, user setting). `NO_SHADOW` (1005). The page says XP12 material commands are documented elsewhere and does not list them
- Shipped 12.4 forests: per shader `TEXTURE`, `SEASONAL <next-season albedo>`, `TEXTURE_NORMAL 1 <nml>`, `WEATHER <..._SM.png>`, `BLEND_HASH 0.5`, `SUPER_ROUGHNESS 1.0`, `NORMAL_TRANSLUCENCY` — Local: `1200 forests/{spr,sum,fal,win}/AG_temp_1.for`

**.agp / .lin / .str** — Shipped .agp files use `TEXTURE`, `TEXTURE_TERRAIN`, `TEXTURE_CONTROL`, `DECAL_LIB`, `DITHER_ALPHA`, `SNOW_ALBEDO_LUMA` (4). Shipped .lin files use `TEXTURE_NORMAL` (385), `NORMAL_METALNESS`, `SNOW_ALBEDO_LUMA` (209), `SPECULAR`, `SUPER_ROUGHNESS`, `WEATHER`, `WEATHER_TRANSPARENT` (99), `DITHER_ALPHA`, `TEXTURE_MODULATOR`, `TEXTURE_TERRAIN`/`CONTROL`. Shipped .str files contain only `OBJECT`/`OBJECT_STRING`/`OFFSET`/`EXPORT` — Local

**Lit fading / far textures**
- `LIT_FADE_PARAMS 4096|3072|8192 7000 15000` and `FAR 5000 20000 0.0625 <tex>` appear in urban .ter files — Local. Undocumented

### Inferences
- `SUPER_ROUGHNESS` is near-universal on draped scenery (0.5) and forests (1.0). It likely overrides or caps gloss for materials without per-pixel gloss. Without it, a renderer would make terrain too shiny. Treat the value as a roughness floor until confirmed.
- `NORMAL_TRANSLUCENCY` probably switches the normal-map channel used for F0 (blue) to translucency, matching the XP12 model's "translucency mode normal map" (F0 = 0.0). Implement it as "B = translucency, F0 = 0" for forests.
- `BLEND_HASH <cutoff>` is most plausibly hashed (stochastic) alpha testing, analogous to `DITHER_ALPHA`.
- `RUNWAY_TILE` / `RUNWAY_NOISE_PARAMS` / `RUNWAY_MARKINGS` are XP12 runway-shader controls (a texture-tile shuffle like `TEXTURE_TILE`, plus a marking atlas).

### Gaps
- No public spec for `SUPER_ROUGHNESS`, `SNOW_ALBEDO_LUMA`, `WEATHER`, `WEATHER_TRANSPARENT`, `SEASONAL`, `NORMAL_TRANSLUCENCY`, `BLEND_HASH`, `LUMINANCE`, `LIT_FADE_PARAMS`, `FAR`, `RUNWAY_*`, `SKIP_WATER`, the 6/7-float `COMPOSITE_PARAMS`, `COMPOSITE_NOISE`, or the `AUTO_SLOPE*` family. The meaning of the normal-decal "gloss" argument is not defined.
- No XP12 .fac, .lin, .agp or .str spec updates were found. The XP12 facade spec (WALL_RULE etc.) is outside the Standard Shading doc.

## Q5. Seasons: how authors provide seasonal variation

### Takeaway
Seasons are an X-Plane 12.0 (1200) feature with three authoring hooks: (1) library.txt seasonal exports (`EXPORT_SEASON` etc.
with `spr,sum,fal,win` tokens), (2) DSF season rasters that give each location's season date ranges, with seasonal art
interpolated between ranges, and (3) a per-material `SEASONAL <texture>` that names the next season's albedo for blending.
No season dataref is published, so `REGION_DREF` cannot target seasons directly.

### Cited Findings
- `EXPORT_SEASON <seasons> <vpath> <rpath>`, `EXPORT_RATIO_SEASON <seasons> <qty> <vpath> <rpath>`, `EXPORT_EXTEND_SEASON`, `EXPORT_EXCLUDE_SEASON` (X-Plane 1200; revision 2023-01-25). `<seasons>` is a whitespace-free comma list from `spr sum fal win`. "Sufficient seasonal export commands must be given to cover all four seasons", and seasonal and legacy exports are mutually exclusive for any one virtual path. Each otherwise behaves like its legacy twin. No seasonal `EXPORT_BACKUP` is listed — [library.txt spec](https://developer.x-plane.com/article/library-library-txt-file-format-specification/)
- `REGION_DREF <dataref> <op> <value>` (10.40): ops `< <= == != > >=`; all lines must be true; plugin or sim datarefs; assets load only when true — [library.txt spec](https://developer.x-plane.com/article/library-library-txt-file-format-specification/). Laminar's 2015 RFC pitched it for seasonal/winter add-ons without a restart — [RFC: dataref-driven library regions](https://developer.x-plane.com/2015/03/rfc-dataref-driven-library-regions/)
- No season dataref exists in the 12.4 DataRefs.txt (searched "season"). Related: `sim/time/local_date_days`, `sim/weather/region/snow_cover` — Local: DataRefs.txt
- DSF (X-Plane 12): eight rasters `spr1/spr2`, `sum1/sum2`, `fal1/fal2`, `win1/win2` hold day-of-year start/end values. Ranges may wrap the year, and "seasonal art is interpolated between ranges". There is no snow raster — [DSF Usage In X-Plane](https://developer.x-plane.com/article/dsf-usage-in-x-plane/)
- Shipped usage: 1,804 `EXPORT_SEASON` and 9,980 `EXPORT_EXCLUDE_SEASON` lines in default libraries. Examples: `EXPORT_SEASON spr,sum lib/g10/autogen/natural.ags EU/sub_Resid01.ags`; `EXPORT_EXCLUDE_SEASON fal,win lib/g10/terrain10/sp_cld_dry_hill.ter terrain10_VLC/fal/sp_cld_dry_hill.ter`; one .for per season (`EXPORT_SEASON spr lib/vegetation/AG_temp_1.for spr/AG_temp_1.for`, and likewise sum/fal/win); `.agp` swaps (`..._wet.agp` vs `..._wet_fa.agp`). Seasonal .lin/.pol runway exports exist but are commented out — Local: `Resources/default scenery/*/library.txt`
- Seasonal assets swap whole files: terrain lives in `terrain12/fal/...` and `textures12/apt/fal/...` folders (e.g., `terrain12/fal/urban_tmp_wet10.ter` uses `textures12/apt/fal/apt_loveg_01.png`) — Local
- `SEASONAL` chain in forests: spr uses `TEXTURE ..._ALB_sp` + `SEASONAL ..._ALB` (summer); sum uses `_ALB` + `SEASONAL _ALB_fa`; fal uses `_ALB_fa` + `SEASONAL _ALB_wi`; win uses `_ALB_wi` + `SEASONAL _ALB_sp`. The winter variant also switches the weather mask to `trees_bb1_SM_wi.png` — Local: `1200 forests/{spr,sum,fal,win}/AG_temp_1.for`
- Pre-release statements (secondary): a July 2021 livestream said seasons would change shaders, not only textures — [Threshold: snow generation preview](https://thresholdx.net/xp12sn/) (summary seen only in search results; not fetched)

### Inferences
- Runtime model: per tile, compute the current season and a 0..1 progress toward the next one from the DSF season rasters and the sim date. Select the library asset whose season list contains the current season. Inside that asset, lerp albedo from `TEXTURE` to `SEASONAL` by progress. This matches "interpolated between ranges" and the observed SEASONAL = next-season pattern.
- An editor preview should expose a season selector (spr/sum/fal/win plus a blend slider) and resolve `EXPORT_*_SEASON` accordingly. Legacy `EXPORT` paths are season-invariant.

### Gaps
- The algorithm X-Plane uses to pick the current season (DSF raster + date, possible user override in the weather UI) is not documented beyond the raster description. The progress-blend math is not documented.
- Which 12.x build first shipped visible seasons is not confirmed by an official release note found here; the docs tag everything as 1200 (spec revision Jan 2023).
- How `EXPORT_*_SEASON` interacts with `REGION`/`REGION_DREF` precedence is not specified.

## Q6. Snow, ice and wet surfaces: what authors control and how X-Plane decides

### Takeaway
Wetness, puddles and snow are engine-driven from weather (snow cover ratio, a runway friction enum). Authors control them per material
through XP12 directives that appear throughout shipped content but are not publicly documented: a `WEATHER <mask.png>` weather mask
(RGB, suffixes `_WM`/`_SM`), a `WEATHER_TRANSPARENT` flag, and `SNOW_ALBEDO_LUMA <4 floats>`. No documented rule says which normals or
heights get snow.

### Cited Findings
- Shipped directive usage: `SNOW_ALBEDO_LUMA` appears in .ter (1), .pol (56), .lin (209), .fac (45), .agp (4) and .obj (109). Common values: `0.4 0.2 0.0 0.0`, `1.0 0.5 0.0 0.0`, `1.5 2.0 0.0 0.0`, `1.2 1.5 0.0 0.0`, `0.2 0.2 0.0 0.0`. `WEATHER <file>` appears in .pol (165), .lin (38), .fac (105), .for (934), .net (37) and .obj (234), with textures like `apt_roads_1_WM.png`, `concrete_2b_wm.png`, `buildings_1_WM.png`, `trees_bb1_SM.png`, `trees_bb1_SM_wi.png`. `WEATHER_TRANSPARENT` (no args) appears in .pol (20), .lin (99) and .obj (296) — Local: grep of `Resources/default scenery`
- The weather mask PNGs are 8-bit RGB (PNG color type 2), e.g., `underpass_WM.png` 512², `buildings_1_WM.png` 1024², `EU_industr1_GND_SM.png` 2048² — Local: PNG header read
- Laminar's "Airport Pavement and FX" (XP12, updated Nov 3 2022) covers pavement/FX layer order but not weather. A reader's Jan 2023 question on how to create a weather texture got no answer — [Airport Pavement and FX](https://developer.x-plane.com/?p=40649)
- Weather state datarefs: `sim/weather/region/snow_cover` (0..1 snow cover on the ground, writable), `sim/weather/region/runway_friction` (enum: dry 0, wet 1-3, puddly 4-6, snowy 7-9, icy 10-12, snowy/icy 13-15), `sim/weather/region/rain_percent`, `sim/weather/view/snow_ratio`, `sim/weather/aircraft/snow_on_aircraft_ratio` (windshield) — Local: DataRefs.txt
- 12.3.0 (Nov 5 2025): "Snow will no longer accumulate in hangar interiors", plus "New weather effects applied to parking lots and hangars" — [SDK News 12.3.0](https://developer.x-plane.com/2025/11/12-3-0-radar-love/)
- 12.1.0 shipped windshield ice/rain visuals for aircraft (separate articles) — [X-Plane 12.1 archive](https://developer.x-plane.com/version/x-plane-12-1); [Windshield Rain Effects](https://developer.x-plane.com/article/windshield-rain-effects/)
- Development history (secondary summaries of Laminar posts): May 2022, "both rain and snow shaders for airports and global scenery is complete" and new rain/snow accumulation — [FSElite, May 19 2022](https://fselite.net/content/laminar-research-provides-x-plane-12-development-update/). July 2023, Laminar was building "an all-new weather surface shader to more accurately display water puddles", with the look changing once water forms a puddle — [Threshold, Jul 15 2023](https://thresholdx.net/lrx12u)
- Physics-side surface flags: OBJ `ATTR_hard snow` (850 surface names: water, concrete, asphalt, grass, dirt, gravel, lakebed, snow, shoulder, blastpad) — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). .ter `WET` = behaves like water for physics only — [Terrain Type spec](https://developer.x-plane.com/article/terrain-type-ter-file-format-specification/)
- `REQUIRE_WET` / `REQUIRE_DRY` (OBJ 1000) choose wet or dry library objects based on the underlying terrain — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). Terrain names like `urban_tmp_wet10.ter` / `apt_tmp_wet.ter` carry the same climate wet/dry axis — Local

### Inferences
- "Wet/dry" in terrain names and `REQUIRE_WET` is a climate (moisture) classification, not rain wetness. Do not drive rain shading from it.
- `WEATHER <mask>` is the authored "weather mask" for where puddles/wetness and snow may form on a material. The `_SM` suffix on vegetation masks plausibly means "snow mask", and winter forests get a dedicated `_SM_wi`. `WEATHER_TRANSPARENT` probably lets weather effects show on alpha-blended/translucent parts (or marks the material as transparent to the weather pass). `SNOW_ALBEDO_LUMA`'s four floats likely tune snow albedo/brightness blending (first two vary widely, last two are always 0 in shipped data). All of these are unconfirmed.
- Hangar-interior snow fixed in 12.3 means snow is applied by default unless masked or occluded. A renderer approximating XP12 should use upward-facing normals x mask x `snow_cover`, with masks honored per material.

### Gaps
- No public documentation of the weather-mask channel layout, `WEATHER_TRANSPARENT`, `SNOW_ALBEDO_LUMA` parameters, puddle generation, or the snow placement rule (normal threshold, altitude, occlusion). A web search found none, and the official FX article's reader question went unanswered.
- A secondary-source claim that "scenery authors will be able to specify desired locations for both puddles and snow banks/drifts by defining weather masks" surfaced in search summaries, but its original source page could not be verified (the two articles fetched do not contain it).
- No documented runway-wetness rendering rules beyond the friction enum.

## Q7. Shadows and reflections per asset

### Takeaway
Shadow casting is controlled per asset (`GLOBAL_no_shadow`, `ATTR_no_shadow`/`ATTR_shadow`, `NO_SHADOW` in standard-shader and forest
files, per-mesh `NO_SHADOW` in .for) and per pass (`*_shadow_blend` alpha-tests the shadow map). Baked shadows for out-of-range distances
come from `TEXTURE_CONTROL` alpha. Reflections come from the PBR F0/gloss inputs plus aircraft glass modes; there are no per-asset
reflection opt-outs in the specs.

### Cited Findings
- OBJ: `ATTR_shadow`/`ATTR_no_shadow` (1010) per triangle range, default casting on. `GLOBAL_no_shadow` excludes the whole object (not allowed with `ATTR_shadow`). `ATTR_shadow_blend`/`GLOBAL_shadow_blend` (1000) alpha-test shadows at a cutoff while the main pass blends. Conditionals `GLOBAL_SHADOWS` / `SCENERY_SHADOWS` (load-time) — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). Shipped OBJs: `GLOBAL_no_shadow` 111, `ATTR_no_shadow` 41, `GLOBAL_shadow_blend` 24, `ATTR_shadow_blend` 18 — Local
- Standard shader: `NO_SHADOW` exempts the asset from shadow generation (useful for billboards and frame rate). `TEXTURE_CONTROL` alpha is a baked shadow (0 = most shadowed) shown only outside the dynamic shadow range — [Standard Shading Options](https://developer.x-plane.com/article/standard-shading-options-for-x-plane-10-scenery/)
- Forests: file-level `NO_SHADOW` (1005), plus an optional `NO_SHADOW` on each XP12 `MESH` line — [Forest spec](https://developer.x-plane.com/article/forest-for-file-format-specification/)
- Cockpit shadows were a known problem in early XP12 because sun/shadow contrast matches real-world (very high) levels (Supnik comment, 2021) — [Photometric Lighting](https://developer.x-plane.com/2021/11/photometric-lighting-what-is-it-and-why-do-we-need-it/)
- Glass: `BLEND_GLASS` keeps specular reflections on otherwise translucent glass — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). F0 defaults to 0.06 for `BLEND_GLASS` in the aircraft glass lighting modes (interior glass, exterior glass, exterior rain glass, interior reflective glass) — [XP12 material model](https://developer.x-plane.com/article/the-x-plane-12-material-model). Rain glass uses Plane Maker lighting modes "Outside Rain Glass" (outward normals) and "Inside Reflector" (inward-facing windows/visors, with override draw weight). They are "complementary and intended to be used together" — [Windshield Rain Effects](https://developer.x-plane.com/article/windshield-rain-effects/). HUD glass: `ATTR_hud_glass` (1200) — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/)
- Water reflections are a separate 3D pass: `sim/graphics/view/world_render_type` 0 = normal, 1 = reflections in water — Local: DataRefs.txt
- "Reflection detail" is a CPU-heavy rendering option. Its cost is cumulative with world-object count ("more things to calculate reflections for") — Local: [X-Plane Manual](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Instructions/X-Plane%20Manual.html)
- 12.1.0: bloom is visible in reflections, not just on direct light sources (Laminar staff comment, Feb 28 2024) — [A Few Notes on 12.1.0](https://developer.x-plane.com/2024/02/a-few-notes-on-12-1-0-for-developers/)

### Inferences
- The water reflection pass re-renders world geometry, so plugin and OBJ draw code can see a reflection pass. A renderer emulating XP12 should give OBJs a cheaper reflection LOD and honor the same shadow flags in its shadow pass.
- No per-asset "no reflection" flag exists. Glossiness and F0 alone determine how much environment/water reflection an asset shows.

### Gaps
- The search surfaced a 12.2.0 note that private art controls must be removed and that cube-map positions should return to default because "X-Plane ... references the position from the OBJ… not Planemaker". The exact source page was not verified.
- Whether draped polygons and terrain receive shadows from all casters (and the cockpit vs world shadow cascades) is not documented for authors.

## Q8. Water: how water bodies are authored and what authors can change

### Takeaway
XP12 water is 3D water from DSF water triangles (mesh layer `water`/`terrain_Water`, no .ter), or from a .ter carrying `WATER_COLOR_MASK`.
Per-vertex fetch ratio scales waves (1 = ocean, 0 = pond), and an interpolation ratio blends ground elevation with the required bathymetry raster
(`sea_level`). Waves come from weather (wind/wave datarefs); authors cannot hand-paint them.

### Cited Findings
- Water mesh: layers named `water` or `terrain_Water` are water triangles with no .ter. They skip normal params and instead carry two ST values: fetch ratio (0..1, 1 = open ocean, 0 = ponds) and depth. XP12 requires a bathymetry raster and treats depth as an interpolation ratio (0 = ground elevation, 1 = bathymetry sample). XP11 used actual depth, or 0 = coastline / 1 = use bathymetry. Bathymetry is in the `sea_level` raster. Elevation raster data applies to water-terrain vertices and vertices set to -32768.0. The sound raster code for water is 30 — [DSF Usage In X-Plane](https://developer.x-plane.com/article/dsf-usage-in-x-plane/)
- .ter-defined water (XP12): a .ter with `WATER_COLOR_MASK` defines water. It expects four ST coords: fetch ratio, bathymetric depth flag, and a UV pair for the water texture — [DSF Usage In X-Plane](https://developer.x-plane.com/article/dsf-usage-in-x-plane/)
- 12.1.0: water bathymetry and turbidity fixed, making orthophoto base meshes with 3D water possible. Complete DSF/water docs were promised for 12.1.0 (Supnik comment) — [A Few Notes on 12.1.0](https://developer.x-plane.com/2024/02/a-few-notes-on-12-1-0-for-developers/)
- 12.3.0: new static water polygons and water-edge markings "designed to nicely blend the edge of water polygons" — [SDK News 12.3.0](https://developer.x-plane.com/2025/11/12-3-0-radar-love/)
- Waves: `sim/weather/region/wave_amplitude` (m, writable) and `wave_dir` (writable); `wave_length` and `wave_speed` are "not writable starting in v12" (derived) — Local: DataRefs.txt. The weather UI's "Bodies of Water" sets wave height and direction, and changing height also changes wave length and speed — Local: X-Plane Manual
- Authoring hooks near water: .pol `SKIP_WATER` appears in 3 shipped .pol and 2 .lin files (undocumented) — Local. OBJ `ATTR_hard water` gives a water physics surface — [OBJ8 spec](https://developer.x-plane.com/article/obj8-file-format-specification/). .ter `WET` = physics-only water — [Terrain Type spec](https://developer.x-plane.com/article/terrain-type-ter-file-format-specification/)
- No `WATER_COLOR_MASK` usage was found in the default scenery .ter/.pol files (searched `Resources`) — Local

### Inferences
- For editor preview: shade DSF water by depth (from `sea_level` bathymetry x the per-vertex ratio) for color/turbidity, and drive wave amplitude by the fetch ratio x weather wave height. Wave length/speed should be derived from height, not authored.
- `SKIP_WATER` probably keeps a draped polygon/line from drawing over water triangles. This is inferred from its name and its use in lawn-track FX .pol files.

### Gaps
- The "complete DSF usage and water docs" promised for 12.1.0 did not appear in the fetched DSF Usage page (its revision history ends 2/22/23). No public spec for `WATER_COLOR_MASK` channel meaning, turbidity, water color by depth, or the 12.3 static water polygon format.
