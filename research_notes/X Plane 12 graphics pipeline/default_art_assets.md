# X-Plane 12.4.3 default art assets: materials, seasons, snow/wet, water and lights

Source conventions. Most findings come from the local, read-only X-Plane 12.4.3 install at
`K:/SteamLibrary/steamapps/common/X-Plane 12/Resources/`. Links marked "local:" point into that install as file URLs.
Counts come from read-only Python scans written for this task (`scan_directives.py`, `scan_mixed.py`, `suffix_scan.py`,
`normal_usage.py`, `normal_stats.py`, `chan_stats.py`, `texref_scan.py`, `imghdr_scan.py`). They and their outputs live in the session scratchpad
(`C:/Users/traye/AppData/Local/Temp/claude/G--Repositorys-Coalition-of-Freeware-Developers-Scenery-Editor-X/67f57d29-13ad-4b5f-ab75-468c7bb89349/scratchpad/assets/`),
which is temporary. Facts taken from developer.x-plane.com are marked **(web)**.

How the counts were made. "lines" is the number of lines whose first token is the directive. "files" is the number of files with at least one such line.
The scan read 9,848 `.ter`, 8,391 `.pol`, 8,149 `.obj`, 806 `.fac`, 497 `.for`, 437 `.lin`, 567 `.agp`, 52 `.ags`, 21 `.agb`, 220 `.dcl`,
6 `.net`, 58 `.str`, 2 `.bch` and 3 `.lib` files, plus every `library.txt`, all under `Resources/default scenery/`.

## Q1. Which shading-related directives appear in the shipped art files, how often, and with what syntax?

### Takeaway
Nearly all shipped art uses the XP11/XP12 "standard shader" vocabulary. Every file type uses TEXTURE / TEXTURE_NORMAL, NORMAL_METALNESS, DECAL_LIB, NO_BLEND / DITHER_ALPHA and TEXTURE_LIT.
XP12 adds several directives that developer.x-plane.com does not document: SUPER_ROUGHNESS, WEATHER, WEATHER_TRANSPARENT, SNOW_ALBEDO_LUMA, SEASONAL, BLEND_HASH, NORMAL_TRANSLUCENCY, FAR, LUMINANCE, LIT_FADE_PARAMS, RUNWAY_TILE / RUNWAY_NOISE_PARAMS / RUNWAY_MARKINGS, PREVIEW_RGB and 7-parameter COMPOSITE_PARAMS. Several documented directives are never used:
GLOSS, DECAL, DECAL_RGBA, DECAL_KEYED, NORMAL_DECAL, NORMAL_DECAL_KEYED, TEXTURE_DETAIL, TEXTURE_NORMAL_DETAIL, LOAD_CENTER and COMPOSITE_TEXTURE.

### Cited Findings
- **Header versions in use** (line 2 of each file): `.ter` 800 TERRAIN (all 9,848); `.pol` 850 DRAPED_POLYGON (all 8,391);
  `.fac` 1000 FACADE (669) and 800 (137); `.for` 1200 FOREST (467) and 800 (30); `.dcl` 1000 DECAL; `.lin` 850 LINE_PAINT; `.agp`/`.ags`/`.agb` 1000;
  `.net` 800 ROADS; `.str` 850 OBJECT_STRING; `.bch` 800 BEACH; `.obj` 800 (8,023 files) and **700 (121 legacy OBJ7 files)**.
  — local: [default scenery](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/), via a header census.
- **Directive count table** (lines / files per extension). Source: local scan output `directives.txt` and `directives_mixed.txt` in the scratchpad.

| Directive | Where used (lines/files) | Shipped example |
|---|---|---|
| `TEXTURE` | obj 7382/7382 · pol 8378 · fac 1194/796 · for 964/497 · agp 492 · lin 437 · net 72 · ags 52 · agb 21 | `TEXTURE ../textures/bld_EUindustr1_ALB.png` (1000 autogen/EU/industrial/gateKeep_01.obj) |
| `BASE_TEX` (.ter) | ter 9848 | `BASE_TEX ../textures10/loveg/grass_cld_wet_flat_c.dds` |
| `TEXTURE_NORMAL` | obj 5410 · fac 928/749 · for 934/467 · lin 385 · pol 365 · ter 62 · net 43 · ags 7 · agb 2 · bch 1 | obj: `TEXTURE_NORMAL ../textures/x_NML.png` (no ratio); fac/pol/lin/ter: `TEXTURE_NORMAL 3.0 ../textures12/apt/apt_ground1_NML.png` (ratio first) |
| `NORMAL_TEX` (.ter legacy spelling) | ter 484 · pol 1 | `NORMAL_TEX 20,000000 ../textures10/shared/clumpy_turf4_NML.png` |
| `TEXTURE_DRAPED` / `TEXTURE_DRAPED_NORMAL` | obj 1310 / 1046 | `TEXTURE_DRAPED_NORMAL 1.0 ../../textures/str_industrial2_NML.png` |
| `TEXTURE_MAP` | obj 12 lines / 7 files (only `normal` and `material_gloss` usages) | `TEXTURE_MAP material_gloss Chocks2_MAT.png` (sim objects/chocks) |
| `NORMAL_METALNESS` | obj 4349/4344 · fac 748/717 · pol 340 · lin 324 · ter 114 · net 35; plus misspellings `NORMAL_metalness` (1 obj) and `NORMAL METALNESS` (sim objects/vr/holodeck/hangar_house.obj) | bare flag after the normal map |
| `NORMAL_TRANSLUCENCY` | for 934/467 · obj 12 (hedges in 1200 forests/sum) | bare flag |
| `SPECULAR` | obj 1067 · lin 159 · pol 42 · net 13 · fac 1 | most common `SPECULAR 1` (1004×); also 0.2, 0, 0.5 |
| `GLOBAL_specular` / `ATTR_shiny_rat` | obj 6105/6092 / 70/48 | `GLOBAL_specular 1.0` (5474×), 0.2 (259×), 0.05 (187×) |
| `GLOSS` | **0 uses** | — |
| `SUPER_ROUGHNESS` | pol 6995 · ter 6960 · for 934 · lin 137 · obj 33 | only two values: `0.5` (14,117×) and `1.0` (942×, forests) |
| `BLEND_GLASS` | obj 2 (sim objects/vr/Flashlight.obj, vr/holodeck/glass.obj) | bare flag |
| `BLEND_HASH` | for 934/467 | always `BLEND_HASH 0.5` |
| `NO_ALPHA` | ter 9848 · pol 8147 · net 13 · fac 4 | bare flag (one fac has `NO_ALPHA 1`) |
| `NO_BLEND` | fac 750/701 · obj 370 · net 14; plus `GLOBAL_no_blend` 4500 and `ATTR_no_blend` 682 | `NO_BLEND 0.5` (1051×), 0.4, 0.35, 0.3 |
| `DITHER_ALPHA` | lin 327 · pol 121 · obj 84 · dcl 20 · net 18 · agp 14 · agb 6 · bch 1 | `DITHER_ALPHA 0.4 0.25` |
| `TEXTURE_LIT` | obj 3528/3527 · fac 121 · pol 80 · ter 49 · net 16 (900 roads uses `TEXTURE_LIT //`) | `TEXTURE_LIT ../textures/bld_EUindustr1_LIT.png` |
| `LIT_TEX` (.ter legacy) | ter 400 (all in XP9 `terrain/`) | `LIT_TEX ../textures/human/dry_cityhill_sq_LIT.dds` |
| `GLOBAL_luminance` | obj 403 | `GLOBAL_luminance 1000` (192×), 60 (141×), 100, 150, 200, 500, 2000 |
| `LUMINANCE` / `LIT_FADE_PARAMS` (.ter) | ter 49 / 49 (terrain12 urban/ind) | `LUMINANCE 600`; `LIT_FADE_PARAMS 4096 7000 15000` (also 3072/8192 first value) |
| `ATTR_light_level` | obj 266 lines / 35 files (+ `ATTR_light_level_reset` 84) | — |
| `DECAL_LIB` | pol 7724/7244 · ter 7442/6962 · obj 2044/1686 · fac 773/616 · lin 436 · agp 51 · ags 46 · net 36 · agb 21 · bch 1 | `DECAL_LIB lib/g10/decals/maquify_1_alpha_key.dcl` |
| `DECAL_PARAMS` | dcl 108/91 · bch 2 · obj 1 | `DECAL_PARAMS 64 0.0  0 0 0 0 1.5 0.00  1.0 1.0 1.0 0.0 0.0 -1.6 textures/grass_and_asphalt_DTL.png` |
| `DECAL_PARAMS_PROJ` | dcl 246/170 · pol 6 · net 4 | `DECAL_PARAMS_PROJ 30 30 0.5  0 0 0 5.0 0.0 -2.0  0 0 0 -2.5 0.0 2.0 textures/RGBA_DECAL_shrub_dirt_sdry3.png` |
| `NORMAL_DECAL_PARAMS` | pol 171 · lin 41 · dcl 34 · fac 3 | `NORMAL_DECAL_PARAMS 6.0 0.7 0 2.0 0 0 -0.25 ../textures/NDHF_asphalt_1.png 1.0` |
| `NORMAL_DECAL_PARAMS_PROJ` | dcl 22/16 · pol 6 · lin 2 | `NORMAL_DECAL_PARAMS_PROJ 40 40 0.0 0.0 0.0 0 0 0.5 ../textures/NDLF_soft_twy_1.png 1.0` |
| `DECAL`, `DECAL_RGBA`, `DECAL_KEYED`, `NORMAL_DECAL`, `NORMAL_DECAL_KEYED` | **0 uses** (`DECAL` appears only as the .dcl header word) | — |
| `TEXTURE_DETAIL`, `TEXTURE_NORMAL_DETAIL` | **0 uses** | — |
| `TEXTURE_TILE` | ter 730 · lin 67 · net 32 · pol 25 | `TEXTURE_TILE 4 4 256 256 ../maps/swizzle_4x4_V4-bars_3.png`; `TEXTURE_TILE 1 8 1 64 swizzle8_64.png` |
| `TEXTURE_CONTROL` / `TEXTURE_TERRAIN` | ags 39 · agp 32 · obj 28 · net 14/20 · agb 13 · lin 10 · pol 1 | `TEXTURE_CONTROL textures/EU_urban1_GND_CTL.png`; `TEXTURE_TERRAIN 160.0 160.0 ../../textures/agp_grass_wet_TRN.png` |
| `TEXTURE_MODULATOR` | obj 901 · fac 571 · lin 58 | `TEXTURE_MODULATOR ..\..\textures\hangars_1_MOD.png` |
| `COMPOSITE_TEX` / `_PROJECTED` / `_PARAMS` / `_NOISE` | ter 2385 / 1968 / 1968 / 1968 · pol 133 each | terrain10: `COMPOSITE_PARAMS 0.104 0.85 0.292 1.82 2.05 0.26` (6 floats); terrain12: `COMPOSITE_PARAMS 0.25 -1.00 0.48 0.95 0.10 1.40 -0.1` (7 floats) |
| `COMPOSITE_BORDERS` / `BORDER_TEX` / `PROJECTED` | ter 9848 each | `PROJECTED 1216 1216` |
| `COMPOSITE_TEXTURE` | **0 uses** | — |
| `AUTO_VARY` | ter 263 (all in the XP9 `terrain/` folder) | bare flag |
| `AUTO_HEADING` / `AUTO_SLOPE` / `AUTO_SLOPE_HILL` / `AUTO_SLOPE_CLIFF` / `AUTO_SLOPE_HEADING` | ter 1412 / 149 / 2597 / 2597 / 2448 | `AUTO_SLOPE_HILL 2328 1164 35 40 ../textures10/loveg/sprs_cld_dry_hill_d.dds` |
| `PROJECT_ANGLE` / `PROJECT_OFFSET` | ter 3000 / 1504 | `PROJECT_ANGLE 0 1 0 0` |
| `BUMP_LEVEL` | ter 114 (all `1.0`) · pol 1 (`0.25`, rwy_skids_1.pol) | — |
| `LOAD_CENTER` | **0 uses** | — |
| `FAR` (.ter, XP12) | ter 63 | `FAR 5000 20000 0.125 ../textures12/urban_FAR/uw4_FAR.dds` |
| `WEATHER` | for 934/467 · obj 234/213 · pol 165 · fac 105/91 · lin 38 · ags 38 · net 37 · agb 15 | `WEATHER ../textures/asphalt_2a_wm.png`; `WEATHER ../textures/trees_bb1_SM.png` |
| `WEATHER_TRANSPARENT` | obj 296 · lin 99 · pol 20 | bare flag |
| `SNOW_ALBEDO_LUMA` | lin 209 · obj 109 · pol 56 · fac 45/27 · agp 4 · ags 2 · ter 1 | `SNOW_ALBEDO_LUMA 0.4 0.2 0.0 0.0` (65×); `1.0 0.5 0.0 0.0`; hangar interiors `0.0 0.0 0.0 0.0` |
| `SEASONAL` (.for) | for 776/388 | `SEASONAL ../textures/trees_bb1_ALB_wi.png` |
| `SHADER_WALL` / `SHADER_ROOF` / `SHADER_CEILING` / `SHADER_FLOOR` (.fac) | 795 / 351 / 30 / 17 | per-part shader blocks |
| `SHADER_2D` / `SHADER_3D` (.for) | 467 each | billboard and 3D-mesh tree shader blocks |
| `RUNWAY_TILE` / `RUNWAY_NOISE_PARAMS` / `RUNWAY_MARKINGS` (.pol) | 158 / 158 / 98 | `RUNWAY_TILE 1 2 256 256 static_3.png`; `RUNWAY_NOISE_PARAMS 0.8 0.3 0.4 0.7 0.8`; `RUNWAY_MARKINGS 1 1 1 0.5 heliport_1.png` |
| `TEXTURE_NOWRAP` | pol 13 (Ground_Signs) | `TEXTURE_NOWRAP DrapedDirSigns.png` |
| `SURFACE` | pol/lin: dirt 7986, asphalt 130, concrete 79, grass 17, water 6, snow 4, lakebed 4, gravel 4 | `SURFACE water` (water_body_*.pol) |
| `REQUIRE_WET` / `REQUIRE_DRY` | obj 7 / 1 (1000 roads bridge pylons) | `REQUIRE_WET 1` |
| `SKIP_WATER` / `SKIP_SURFACE water` | pol 3 · lin 2 / for 497 | — |
| `GLOBAL_tint` | obj 1345 | `GLOBAL_tint 0.25 0.5` (213×) |
| Other mixed-case OBJ attributes | `ATTR_draped` 1312, `ATTR_LOD_draped` 938, `ATTR_layer_group(_draped)` 557/665, `ATTR_no_cull` 532, `ATTR_cull` 434, `GLOBAL_no_shadow` 111, `ATTR_shade_smooth` 79, `ATTR_shade_flat` 38, `GLOBAL_shadow_blend` 24, `GLOBAL_cockpit_lit` 21, `ATTR_diffuse_rgb` 12, `ATTR_emission_rgb` 1 | — |
| Lights in OBJ | `LIGHT_PARAM` 7894 lines / 618 files · `LIGHT_NAMED` 6985 / 1015 · `LIGHT_CUSTOM` 3 / 2 · `VLIGHT` 3 / 2 (legacy) | `LIGHT_PARAM full_custom_halo_night -11.8 5.65 21.0 1.0 0.937 0.808 1.0 16.0 0.0 -1.0 0.0 0.087` |
| Conditionals | `IF NOT SCENERY_SHADOWS` … `ENDIF` (2 lines, 1 obj: sim objects/apt_aircraft/jet/CRJ1_AFR/CRJ1_AFR_static.obj) | — |
| Undocumented or unusual | `PREVIEW_RGB 0.8 0.8 0.8` (lin 26, sim objects/apt_lines); `TEXTURE_BRIDGE 0 railroad.dds` (900 roads/road.net); `JUNC_SHADER 7 4000 none` (1000 roads/roads.net); `CREASE_ANGLE -3.0 3.0` (ags 52); `#SURFACE`/`# NORMAL_METALNESS` commented in many XP12 pols; 39× commented `# SNOW_ALBEDO_LUMA 0.6 0.0 0.0 0.0` | — |

- **(web) Standard shading syntax** (developer.x-plane.com "Standard Shading Options"):
  - `TEXTURE_NORMAL [ratio] [file]`: the ratio is the normal map's resolution relative to the albedo; 4.0 repeats it 4× per albedo tile.
  - `SPECULAR [ratio]`: multiplies the normal map's specular channel. `BUMP_LEVEL [0..1]`: scales bump height.
  - `NO_BLEND [cutoff]`, `DITHER_ALPHA [softness] [bleed]`, `NO_ALPHA`.
  - `DECAL_PARAMS [scale] [dither]` + two sets of `[r g b a m k]` + file. Set 1 drives the RGB decal, set 2 the alpha decal. `DECAL_PARAMS_PROJ [scale_x] [scale_y] [dither] …` repeats per meter. Up to two decals per shader.
  - 12.1.0+: `NORMAL_DECAL_PARAMS [scale] [r] [g] [b] [a] [m] [k] [file] [gloss]` and `NORMAL_DECAL_PARAMS_PROJ [sx] [sy] …`.
  - `TEXTURE_TILE [x_tiles] [y_tiles] [x_pages] [y_pages] [map]`: in the map, red selects the X tile and green the Y tile.
  - `TEXTURE_CONTROL` channels: R = RGB decal and first normal decal; G = terrain layer; B = alpha decal and second normal decal; A = baked shadow.
  - `TEXTURE_MODULATOR` (12.1.0+): its red maps to control red and its green to control blue; it replaces the terrain and baked-shadow channels to save VRAM.
  — [Standard Shading Options (web)](https://developer.x-plane.com/article/standard-shading-options-for-x-plane-10-scenery/)
- **(web) OBJ8 spec definitions:**
  - `NORMAL_METALNESS` (1100): the blue channel sets base reflectance.
  - `BLEND_GLASS` (XP11): translucent in diffuse while keeping specular.
  - `GLOBAL_specular` (1000): a non-zero value is needed for the normal map's specular channel to show.
  - `GLOBAL_tint <albedo> <emissive>` (1000): varies brightness on instanced objects.
  - `GLOBAL_luminance <nits>` (1200): baseline luminance of the LIT texture, in nits.
  - `TEXTURE_MAP <usage> <file>` (1200): usages `normal`, `material_gloss`, `gloss`; cannot be combined with TEXTURE_NORMAL.
  - `REQUIRE_WET`/`REQUIRE_DRY` (1000): restrict library placement to wet or dry terrain.
  - `LIGHT_PARAM` (940), `LIGHT_CUSTOM` (850).
  - `ATTR_light_level v1 v2 dref [brightness]`: the optional brightness enables photometric mode.
  — [OBJ8 spec (web)](https://developer.x-plane.com/article/obj8-file-format-specification/)
- **(web) .ter spec:** `PROJECTED <x m> <y m>`, `PROJECT_ANGLE x y z a`, `PROJECT_OFFSET`, `COMPOSITE_BORDERS`, `BORDER_TEX`, `AUTO_VARY` (900), `LOAD_CENTER` (920) and `WET` (physics only) are documented. `COMPOSITE_PARAMS`, `COMPOSITE_NOISE`, `NORMAL_TEX`, `AUTO_SLOPE*`, `FAR`, `LUMINANCE`, `LIT_FADE_PARAMS` and `SUPER_ROUGHNESS` are **not**. — [Terrain .ter spec (web)](https://developer.x-plane.com/article/terrain-type-ter-file-format-specification/)
- **(web)** The developer site's spec index has no XP12 page for WEATHER, SNOW_ALBEDO_LUMA, SEASONAL, `.agp` or `.dcl`. Searches for SUPER_ROUGHNESS, BLEND_HASH, NORMAL_TRANSLUCENCY, SNOW_ALBEDO_LUMA and WEATHER_TRANSPARENT found no official documentation. — [Spec index (web)](https://developer.x-plane.com/docs/specs/), [XP12 material model (web)](https://developer.x-plane.com/article/the-x-plane-12-material-model)
- **Shipped syntax differs from the web docs:**
  - `TEXTURE_TERRAIN` ships with 3 arguments (`TEXTURE_TERRAIN 160.0 160.0 file`, `TEXTURE_TERRAIN 16 16 textures/BallastGreyWet_TRN.png`). The web page shows only `[texture_file]`.
  - In `.net`, `TEXTURE 1 file` and `TEXTURE_NORMAL 1 file` carry a leading numeric argument.
  — local: [1000 roads/roads.net](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20roads/roads.net); [Standard Shading Options (web)](https://developer.x-plane.com/article/standard-shading-options-for-x-plane-10-scenery/)
- **Locale-style decimal commas in 432 terrain files:** `NORMAL_TEX 20,000000` (265 lines), `5,300000` (147), `26,000000`, `50,000000`, `40,000000`. Example: `1000 world terrain/terrain10/sp_tmp_wet_fl.ter`. — local: [terrain10](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20world%20terrain/terrain10/)
- **XP12 airport-terrain example** (`terrain12/urban_tmp_wet1.ter`): `BASE_TEX`, `COMPOSITE_TEX`, `NORMAL_TEX 3.0 apt_ground1_NML.png`, `BORDER_TEX`, `COMPOSITE_NOISE mix_02.png`, `TEXTURE_LIT ../textures12/urban_FAR/uw2_FAR_LIT.dds`,
  `LIT_FADE_PARAMS 3072 7000 15000`, `LUMINANCE 600`, `FAR 5000 20000 0.125 ../textures12/urban_FAR/uw4_FAR.dds` (commented "scale = 2048 m"), `NORMAL_METALNESS`, `SUPER_ROUGHNESS 0.5`, `BUMP_LEVEL 1.0`, `PROJECTED 256 256`,
  `COMPOSITE_PROJECTED 280 280`, 7-float `COMPOSITE_PARAMS`, `DECAL_LIB lib/g10/decals/ter_wet_loveg_1.dcl`. — local: [terrain12/urban_tmp_wet1.ter](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20world%20terrain/terrain12/urban_tmp_wet1.ter)
- **XP12 pavement example** (`sim objects/apt_pavement/pavement/asphalt_1a.pol`): `LAYER_GROUP runways +1`, `TEXTURE asphalt_1a_ALB.png`, `TEXTURE_NORMAL 1.0 asphalt_2a_NML.png`, `WEATHER asphalt_2a_wm.png`,
  `TEXTURE_TILE 4 4 256 256 ../maps/swizzle_4x4_V4-bars_3.png`, `NORMAL_METALNESS`, `DECAL_LIB lib/g10/decals/apt_asphalt_1.dcl`, `NORMAL_DECAL_PARAMS 6.0 0.7 0 2.0 0 0 -0.25 NDHF_asphalt_1.png 1.0`, `SURFACE asphalt`, `SCALE 18 18`.
  — local: [asphalt_1a.pol](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/sim%20objects/apt_pavement/pavement/asphalt_1a.pol)
- **Facade example** (`airport scenery/Common_Elements/Hangars/hangar_01a_M.fac`, header 1000):
  - `SHADER_WALL` { `TEXTURE hangars_1_ALB.png`, `TEXTURE_NORMAL 1.0 hangars_1_NML.png`, `TEXTURE_MODULATOR hangars_1_MOD.png`, `NORMAL_METALNESS`, `NO_BLEND 0.5`, `DECAL_LIB …bld_hangar_2.dcl` }
  - `SHADER_ROOF` { `TEXTURE roof_flat_1b_ALB.png`, `TEXTURE_NORMAL 1.0 roof_flat_1b_NML.png`, `DECAL_LIB …AG_flatroof_1h.dcl`, `WEATHER ../../textures/roof_flat_1_WM.png` }
  - Paths use mixed `\` and `/` separators.
  — local: [hangar_01a_M.fac](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/airport%20scenery/Common_Elements/Hangars/hangar_01a_M.fac)
- **Most-referenced decal libraries:** `lib/g10/decals/maquify_1_alpha_key.dcl` (6140 refs), `grass_and_stony_dirt_1.dcl` (1938), `stony_dirt_1.dcl` (1496), `rail_ballast_dry.dcl` (1496), `maquify_1_green_key.dcl` (1400), `low_freq_mod_1.dcl` (960),
  `long_grass_and_gravel_1.dcl` (960), `bld_hangar_1.dcl` (499), `bld_hangar_2.dcl` (424). `maquify_1_green_key.dcl` keys an RGBA decal to "anything vaguely green" in the albedo: `DECAL_PARAMS_PROJ 30 30 0.5 -12.0 20.5 -8.0 0 0.0 0.0 12 -20 8 -0.0 0.0 1.0 textures/RGBA_DECAL_shrub_dirt_sdry3.png`.
  — local: [1000 decals](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20decals/)

### Inferences
- A renderer that wants parity with the shipped art needs these in its material model:
  - albedo + normal/material map with a NORMAL_METALNESS flag;
  - SPECULAR/GLOBAL_specular scaling, plus SUPER_ROUGHNESS (seemingly a fixed roughness floor/override: 0.5 on terrain, 1.0 on trees);
  - NO_BLEND alpha test, DITHER_ALPHA and BLEND_HASH (hashed alpha);
  - up to two RGB/alpha decals and two normal decals, keyed by `r g b a m k` coefficients;
  - TEXTURE_CONTROL / TEXTURE_TERRAIN / TEXTURE_MODULATOR detailing;
  - TEXTURE_TILE swizzle tiling;
  - terrain composites (`COMPOSITE_*` with a noise texture);
  - LIT emissive in nits (`GLOBAL_luminance`, `LUMINANCE`);
  - per-asset weather masks (`WEATHER`) and a snow response (`SNOW_ALBEDO_LUMA`).
- The 6-float and 7-float COMPOSITE_PARAMS variants (terrain10 vs terrain12) probably reflect two blend formulas. The 7th value varies (-0.1 vs 0.4 between `apt_cld_dry.pol` and `apt_cld_rain.pol`), which suggests a climate-dependent mix bias.
- WEATHER_TRANSPARENT appears only on draped markings: Ground_Signs, apt_lines, parking markings, pavement_fx and draped container rows. It probably makes the layer take the weather state of the surface below, so snow or wetness covers the markings, instead of using its own mask.
- `SNOW_ALBEDO_LUMA` = `0 0 0 0` on hangar-interior facades/agps suggests the first values scale snow coverage. Zero disables it under cover.
- Parsers must accept both `/` and `\` path separators, tab or space separation (the web spec says spaces only), decimal commas, and lowercase or misspelled variants (`NORMAL_metalness`, `NORMAL METALNESS`).

### Gaps
- No official definition was found for SUPER_ROUGHNESS, WEATHER (channel meaning), WEATHER_TRANSPARENT, SNOW_ALBEDO_LUMA (4 parameters), BLEND_HASH, NORMAL_TRANSLUCENCY, SEASONAL, FAR, LUMINANCE, LIT_FADE_PARAMS, RUNWAY_TILE / RUNWAY_NOISE_PARAMS / RUNWAY_MARKINGS, the COMPOSITE_PARAMS components, CREASE_ANGLE or PREVIEW_RGB. Their meanings above are inferred from usage only.
- The `gloss` parameter of NORMAL_DECAL_* is undefined on the web page; shipped values are 0.5 to 1.0.

## Q2. Which texture-file conventions (suffixes, formats, path resolution) does the shipped art follow?

### Takeaway
Textures follow a suffix convention:
- `_ALB` albedo, `_NML`/`_NRM` normal or material map, `_LIT` emissive
- `_SM`/`_WM` weather masks, `_MOD` modulator, `_CTL` control map
- `_TRN` terrain detail, `_DTL` decal detail, `_MAT` material/gloss, `_FAR` far LOD
- decal prefixes `RGBA_DECAL_`, `NDHF_`/`NDLF_`, `A_LF_`, `DECAL_GS_`/`GA_`

Every DDS in default scenery is legacy DXT1/DXT5 with no DX10 header. Every normal map is an uncompressed 8-bit PNG.
Art files usually reference `.png`, but 15,585 references resolve only because a same-named `.dds` exists, so a loader must try `.dds` for a `.png` reference. lights.txt references a `.dds` that exists only as `.png`, so the reverse fallback is needed too.

### Cited Findings
- **Suffix census** (PNG/DDS in default scenery):
  - `NML` 394 (PNG RGBA8 300, PNG L8 94); `ALB` 384 (DDS DXT5 263, DXT1 111, a few PNG); `LIT` 224 (DXT1 197, PNG RGB8 19, DXT5 7)
  - `NRM` 35 (PNG RGBA8/RGB8); `FAR` 37 (DXT5); `SM` 22 (PNG RGB8); `WM` 10 + `wm` 14 (PNG RGB8)
  - `CTL` 16 (PNG RGBA8/RGB8, some DXT5); `MOD` 15 (PNG RGB8); `TRN` 14; `GND` 14 (ground albedo); `MAT` 4 (PNG RGB8); `DTL` 3 (PNG RGBA8)
  - Terrain textures end in `_c`, `_c2`, `_d`, `_s`, `_s2`, all DXT5 (e.g. `broadl_hot_rain_flat_c2.dds`).
  — local: scan `suffix.txt` (scratchpad), over [default scenery](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/)
- **Format census** across all default scenery textures: DDS DXT5 1731, DDS DXT1 620, PNG RGBA8 505, PNG L8 179, PNG RGB8 169, PNG LA8 12, PNG L1 1. No BC4/BC5/BC7/DX10 DDS. — local: scan `texref_scan.py` output
- **Texture reference resolution** over all art files (comments excluded):
  - 27,660 refs hit an existing `.png`; 26,137 hit an existing `.dds`.
  - **15,585 `.png` refs have only a `.dds` on disk** (e.g. `1000 autogen/EU/high_DenseBlock01.ags: textures/EU_urban1_GND_ALB.png`; all 1200 forests `.for` files reference `trees_*_ALB*.png` while the disk has `trees_*_ALB*.dds`).
  - 46 refs have both a png and a dds; 132 refs are missing (e.g. `../../textures/EmbankmentsWet.dds` in 1000 roads bridge piers).
  — local: [1200 forests/textures](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1200%20forests/textures/)
- `lights.txt` says `TEXTURE 1000_lights_close.dds`, but only `bitmaps/world/lites/1000_lights_close.png` (1024×512 RGB8) exists. — local: [lites](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/world/lites/)
- **Decal texture prefixes** in `1000 decals/textures`: `RGBA_*` 59 (RGBA decals: RGB = colour decal, A = alpha decal; e.g. `RGBA_DECAL_shrub_dirt_sdry3.png` RGBA 1024²), `A_LF_*` 24 (L8 alpha low-frequency, e.g. `A_LF_flatroof_1a.png`), `DECAL_*` 12, `NDHF_*` 8 (normal decal high-frequency).
  `NDLF_soft_twy_1.png` (normal decal low-frequency) is in sim objects/apt_pavement/textures. — local: [1000 decals/textures](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20decals/textures/)
- **Sampled channel content (Pillow, 6k random pixels):**
  - `NDHF_roof_flat_1.png`, `NDHF_gravel_1.png`: RG ≈ 127 ± normal, **B = 0**.
  - `ter_sfx1_gravel_MOD.png`, `fencing_MOD.png`: **R and G used, B = 0**.
  - `EU_industr1_GND_CTL.png`: RGB binary 0/255, A = 255; `ter_sfx_conc_CTL.png`: R and B used, G = 0.
  - `agp_grass_wet_TRN.png`: plain grass-coloured albedo (≈68,77,47).
  - `grass_and_asphalt_DTL.png`: RGBA, A ≈ 128 mid-grey.
  - `Chocks2_MAT.png` (TEXTURE_MAP material_gloss): **R = 10 constant, G = 74–145, B = 0**; `Chocks2_NRM.png`: RG normal, B = 0.
  - Noise textures `textures10/shared/fornoise.png` and `natnoise.png`: grey RGBA 1024²; `textures12/shared/mix_02.png`: L8 512²; `tiles_8x4.png`: 64² RG tile selector, B = 0.
  — local: scan `chan_stats.py` output over files under [default scenery](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/)
- (web) The TEXTURE_TILE map uses red for the X tile offset and green for the Y offset. That matches `tiles_8x4.png` and the `swizzle*` maps having only R and G. — [Standard Shading Options (web)](https://developer.x-plane.com/article/standard-shading-options-for-x-plane-10-scenery/)

### Inferences
- A loader should resolve `x.png` to `x.dds` first when present, and `x.dds` to `x.png` as a fallback, case-insensitively and with `\` normalised.
- All albedo/LIT DDS are BC1/BC3 without sRGB flags. An editor has to assume an sRGB transfer for `_ALB`/`_LIT` and linear for `_NML`/`_SM`/`_WM`/`_CTL`/`_MOD`. This is inferred from usage, not stated in any file.
- `_MAT` (material_gloss) appears to pack R = F0/metalness (10/255 ≈ 0.04, a dielectric) and G = gloss, which mirrors the B/A layout of NORMAL_METALNESS maps.

### Gaps
- Colour space handling (sRGB vs linear per suffix) is not declared anywhere in the art files.
- Why 132 references are missing is unknown; some may be dead assets.

## Q3. How are seasons represented on disk?

### Takeaway
Seasons are switched almost entirely by the library system. `EXPORT_SEASON` and `EXPORT_EXCLUDE_SEASON` take comma-separated sets of the codes `spr`, `sum`, `fal`, `win`, and the seasonal variants live in `fal/`, `win/`, `spr/` and `sum/` subfolders or files with a `_fal` suffix.
How many states a type has depends on the type:
- **terrain:** 2 or 3 states (spr+sum base, fal, win or fal+win)
- **autogen:** 2 states (`_fal`)
- **forests:** 4 states (spr/sum/fal/win), and each `.for` also names a `SEASONAL` albedo for the next season
- **airport:** 3 `.agp` only

No library uses `REGION_DREF` or any dataref-keyed region. No art file uses a `#season` comment or a season-keyed directive other than `SEASONAL`.

### Cited Findings
- **Season directive counts** in all library.txt files:
  - `EXPORT_EXCLUDE_SEASON spr,sum` 4207, `fal,win` 2641, `win` 1566, `fal` 1566 (9,980 lines, in 2 libraries).
  - `EXPORT_SEASON spr,sum` 408, `fal,win` 408, `win`/`sum`/`spr`/`fal` 247 each (1,804 lines, in 4 libraries).
  - Libraries with season lines: 1000 world terrain 10,068, 1200 forests 988, 1000 autogen 722, airport scenery 6, sim objects 8 (all commented out).
  — local: [1000 world terrain/library.txt](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20world%20terrain/library.txt)
- **Terrain:** 4,083 virtual paths have seasonal exports (2012 `.ter`, 2018 `.pol`, 47 `.lin`, 6 `.obj`).
  - 2,485 use `spr,sum` + `fal,win` (2 states); 1,430 use `spr,sum` + `fal` + `win` (3 states); the rest repeat these patterns.
  - Typical lines:
    - `EXPORT_EXCLUDE_SEASON spr,sum lib/g10/terrain10/crp_tmp_wet.ter terrain10_SEA/crp_tmp_wet.ter`
    - `EXPORT_EXCLUDE_SEASON fal lib/g10/terrain10/crp_tmp_wet.ter terrain10_SEA/fal/crp_tmp_wet.ter`
    - `EXPORT_EXCLUDE_SEASON win … terrain10_SEA/win/crp_tmp_wet.ter`
  - Folders: `terrain10/fal` (3,844 files), `terrain10/win` (1,500), `terrain10_{CANsq,NAW,SEA}/{fal,win}`, `terrain10_{NHS,SHS,VLC}/fal`, `terrain12/fal`, `terrain_fx/fal`.
  - Texture folders: `textures10/{hiveg,human,loveg,soil}/fal`, `textures10/{hiveg,human}/win`, `textures12/{apt,urban,urban_FAR}/fal` (295 files under `fal` and 111 under `win` in textures).
  — local: [1000 world terrain](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20world%20terrain/)
- **Terrain variants change only the textures.** `terrain10/win/broad_cld_dry_fl.ter` is identical to `terrain10/broad_cld_dry_fl.ter` except `BASE_TEX ../../textures10/hiveg/win/broadl_cld_sdry_sflat_d.dds` replaces `../textures10/hiveg/broadl_cld_sdry_sflat_d.dds`. Spring has no separate terrain art (it shares the base with summer).
  — local: [terrain10/win/broad_cld_dry_fl.ter](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20world%20terrain/terrain10/win/broad_cld_dry_fl.ter)
- **Forests:**
  - 247 `.for` virtual paths, each with 4 seasons, e.g. `EXPORT_SEASON spr lib/vegetation/AG_temp_1.for spr/AG_temp_1.for` … `win/AG_temp_1.for`.
  - Folders: `1200 forests/spr` 97, `sum` 186, `fal` 97, `win` 97 files.
  - Each shader block names `TEXTURE` (the season's albedo) and `SEASONAL` (the next season's albedo), in a cycle: spr `trees_*_ALB_sp` → `SEASONAL trees_*_ALB`; sum `trees_*_ALB` → `SEASONAL …_fa`; fal `…_fa` → `SEASONAL …_wi`; win `…_wi` → `SEASONAL …_sp`.
  - Normal maps also vary: `trees_3D1_NML_fa.png` (fal) and `trees_3D1_NML_wi.png` (win). Weather masks vary: `trees_bb1_SM_wi.png` (win).
  - Seasonal albedos are DDS DXT5: 2048×4096 for the 3D trees, 1024×2048 for billboards.
  — local: [1200 forests/library.txt](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1200%20forests/library.txt), [1200 forests/win/AG_temp_1.for](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1200%20forests/win/AG_temp_1.for)
- **Autogen:** 242 virtual paths (144 `.agb`, 98 `.ags`) switch spr,sum ↔ fal,win, e.g. `EXPORT_SEASON fal,win lib/g10/autogen/natural.ags EU/sub_Resid01_fal.ags`.
  Seasonal autogen files carry a `_fal` suffix (`park_ec01_fal.ags`, `LESres_0v0_fal.agb`, `golf_course1_fal.ags`). Ground textures do too, e.g. `grass_wet_patchy_TRN_fal.dds`.
  — local: [1000 autogen/library.txt](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20autogen/library.txt)
- **Airport:** `EXPORT_SEASON spr,sum/fal,win` for `lib/airport/hangars/military/ground_shelters/20x30_wet.agp` (→ `…_wet_fa.agp`) and LEBB `_su`/`_wi` agps (texture `LEBB_02_ALB_su.dds`).
  sim objects/library.txt holds commented-out seasonal grass-runway exports: `# EXPORT_SEASON fal,win lib/airport/default_runways/grass/runway.lin apt_pavement/pavement/fal/rwy_grass_1.lin`.
  — local: [airport scenery/library.txt](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/airport%20scenery/library.txt), [sim objects/library.txt](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/sim%20objects/library.txt)
- **No dataref-driven regions:** the shipped libraries use `REGION_DEFINE`, `REGION_BITMAP` (360×180 PNGs, one pixel per 1° tile, e.g. `maps/region_VLC.png`, `global_dry.png` L8), `REGION_RECT -180 -90 179 89` and `REGION`. There are **0 `REGION_DREF`** lines.
  Terrain regions are `terrain10_VLC/SHS/SEA/NHS/NAW/CANsq/AUS/`. Others include forests `cacti`/`yucca`, roads `euro`/`south_america`/`usa`, `backward_driving` and beaches `tropical`. — local: [library.txt files](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/)
- Seasonal look in a sample terrain texture: summer, fal and win `broadl_cld_sdry_sflat_d.dds` (1024² DXT5) have mean RGB 59,63,42 / 65,63,43 / 69,67,49. Fall adds orange foliage patches; winter is browner and leafless. **No snow is painted into winter textures.** — local: visual comparison of [textures10/hiveg](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20world%20terrain/textures10/hiveg/)

### Inferences
- To match X-Plane, an editor needs a season selector in its library resolver. That selector must evaluate `EXPORT_SEASON` / `EXPORT_EXCLUDE_SEASON` sets. Since 4-season forests and 2/3-state terrain coexist, resolve each virtual path against the current season code, not a global art set.
- `SEASONAL` most likely feeds a shader cross-fade between the current and next season's albedo for gradual transitions. The cycle in the `.for` files strongly suggests this; how the blend factor is computed is unknown.
- Terrain names use `sp` for "sparse" (e.g. `sp_tmp_wet_fl.ter`, `broadl_sp_cld_wet_flat_c.dds`), not spring. `dry/sdry/wet/rain` are climate moisture classes, not surface wetness.

### Gaps
- What season X-Plane picks (date, latitude/hemisphere, climate, user setting) and how fast `SEASONAL` blends cannot be determined from the art files; that logic is in the executable.
- No shipped asset shows whether a region bitmap can be combined with season sets inside one `REGION` block beyond what the terrain library does.

## Q4. How are snow, wet surfaces/puddles and ice represented?

### Takeaway
Weather snow and wetness are **shader-driven**. Global support textures in `bitmaps/world/weather/` (`snow_ALB/DCL/NML`, `ice_ALB/DCL/NML`, `noise`) are modulated per asset by:
- `WEATHER <mask>` textures: `_SM` masks carry only the blue channel; `_WM` masks carry R, G and some B, with puddle blobs and crack lines;
- `SNOW_ALBEDO_LUMA` (4 floats), and `WEATHER_TRANSPARENT` on draped markings.

There are also climate-driven monthly MODIS snow-cover maps (`bitmaps/Snow Cover/`).
Baked snow and ice exist only as art for permanent land classes (140 `snow_*` and 140 `ice_*` terrains) and for the apt.dat snow surface (`*_snow.pol/.lin`). No puddle or wet albedo variants ship.

### Cited Findings
- `bitmaps/world/weather/` (global shader inputs):
  - `snow_ALB.png` 512² RGBA8 (RGB ≈ 233 white, A 71–187 variation); `snow_DCL.png` 2048² L8; `snow_NML.png` 1024² RGBA8 (RG ≈ 128 ± 6, **B = 10 constant, A = 153 constant**)
  - `ice_ALB.png` 2048² RGBA8 (RGB 242, A mean 28); `ice_DCL.png` 1024² RGBA8; `ice_NML.png` 1024² RGBA8 (**B = 13, A = 192 constant**)
  - `noise.png` 1024² L8
  — local: [bitmaps/world/weather](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/world/weather/)
- `bitmaps/Snow Cover/`: 12 files `MOD10C1_M_SNOW_2025-01.png` … `-12.png`, each 3600×1800 palettised PNG (0.1° per pixel global grid). — local: [Snow Cover](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/Snow%20Cover/)
- **WEATHER masks:**
  - 48 distinct mask textures are referenced. By suffix: `SM` 1080 refs, `WM` 219, `wm` 156, `SM_wi` 97 (winter tree billboard mask), `int` 14.
  - `_SM` samples have R = G = 0 and only B populated: `trees_3D1_SM.png` B 24–84; `trees_bb1_SM.png` B 0–54; `EU_industr1_GND_SM.png` B 0–96.
  - `_WM` samples: `asphalt_2a_wm.png` R ≈ 60, G ≈ 63, B ≈ 5. Visually, broad yellow (R+G) dampness noise, bright green blobs in the wheel-track areas (puddles) and thin red crack networks.
  - `roof_flat_1_WM.png`: R ≈ 74 horizontal streaks, G ≈ 11 small blobs, B ≈ 83 broad noise. `underpass_WM.png` R-dominant.
  — local: [asphalt_2a_wm.png](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/sim%20objects/apt_pavement/textures/asphalt_2a_wm.png), [roof_flat_1_WM.png](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/airport%20scenery/textures/roof_flat_1_WM.png), [1200 forests/textures](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1200%20forests/textures/)
- **SNOW_ALBEDO_LUMA values** (4 floats, last two always 0.0 in shipped files):
  - `0.4 0.2` (65×), `1.0 0.5` (29×), `1.5 2.0` (28×), `1.0 0.6/0.2/0.1`, `0.8 0.2`, `0.7 0.1`, `1.2 0.75` (lawn tracks), `1.2 1.5` (`terrain12/apt_tmp_wet.ter`, the only .ter), `0.2 0.2 0.2 0.0` (water bodies)
  - `0.0 0.0 0.0 0.0` on hangar interiors (`hangar_int_1.fac`, `hangar_88x86_1_open.agp`)
  — local: [default scenery](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/), via a value census
- **Permanent snow and ice land classes:**
  - `terrain10/snow_*` (140 `.ter` + 140 `.pol`): base textures are 78 rock, 49 snow (e.g. `textures10/soil/snow_vcld_wet_sflat_d.dds`), 9 ice, 3 sparse, 1 cliff. Example: `snow_cld_dry_flat.ter` uses `rock_cld_sdry_flat_c.dds`.
  - `ice_*` (140): 128 ice textures (e.g. `ice_pol_sflat.dds`, `cliff_ice_pol.dds`).
  - The NHS region swaps `ice_pol_*` terrains with `EXPORT_EXCLUDE … terrain10_NHS/ice_pol_*.ter`.
  — local: [terrain10](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20world%20terrain/terrain10/), [textures10/soil](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20world%20terrain/textures10/soil/)
- **apt.dat snow surface art:** `sim objects/apt_pavement/pavement/twy_snow.pol`, `pad_snow.pol`, `heli_snow.pol` and `rwy_snow.lin` set `SURFACE snow` and use `snow_twy_1_ALB.png` + `TEXTURE_NORMAL 4.0 snow_twy_1_NML.png` (`# NORMAL_METALNESS` is commented out) with `DITHER_ALPHA 0.1 0.1`. — local: [twy_snow.pol](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/sim%20objects/apt_pavement/pavement/twy_snow.pol)
- **Other weather overlays in bitmaps:**
  - `world/overlays/windshield2d/2d_rain_drops.png` (128×256 RGBA) and `2d_snow_flakes.png` (8×128 RGBA)
  - `world/overlays/ice.png` (256² RGBA, 2D panel icing)
  - `skycolors/sky_colors_snowy.png` among 10 sky-colour tables
  — local: [bitmaps/world/overlays](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/world/overlays/)
- **"Wet" in X-Plane vocabulary usually means over water, not rain-wet:**
  - `REQUIRE_WET 1` on bridge pylons (`1000 roads/objects/structures/15106Wet.obj`, `HwyPylWet.obj`) vs `REQUIRE_DRY 1` (`RailPylSteelFt.obj`).
  - Road types are commented `#VROAD primary/wet (22.625000)`.
  - (web) REQUIRE_WET/REQUIRE_DRY restrict placement to wet or dry terrain, and the `.ter` `WET` directive is physics-only.
  — local: [1000 roads/objects/structures](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20roads/objects/structures/); [OBJ8 spec (web)](https://developer.x-plane.com/article/obj8-file-format-specification/); [.ter spec (web)](https://developer.x-plane.com/article/terrain-type-ter-file-format-specification/)
- (web) Press coverage of the 2021 snow preview describes snow covering ground, trees, roads and structures driven by weather variables. It is not developer documentation. — [ThresholdX preview (web)](https://cms.thresholdx.net/xp12sn)

### Inferences
- The `MOD10C1_M_SNOW_YYYY-MM` names match NASA's MODIS MOD10C1 snow-cover product on a 0.05° climate-modelling grid, apparently resampled to 0.1°. This is background knowledge, not stated in the install. The maps likely give X-Plane a climatological snow probability per month and location.
- `snow_NML` B = 10/255 ≈ 0.039 and `ice_NML` B = 13/255 ≈ 0.05 fit the NORMAL_METALNESS "blue = F0" convention, with dielectric F0 around 0.04. Their alpha 153/255 ≈ 0.6 and 192/255 ≈ 0.75 would be gloss. So the weather snow and ice layers are themselves PBR layers in the metalness packing.
- `_SM` = probably a "snow mask": blue = where snow accumulates; R/G (wetness/puddles) are unused for vegetation and ground decals.
  `_WM` = a fuller "weather mask": G = puddle mask (green blobs in wheel tracks and on flat roofs), R = dampness/streaks/cracks, B = snow accumulation.
  This channel assignment comes from visual pattern analysis only and is **unconfirmed**.
- An editor wanting XP12-like winter visuals needs a global snow amount, the monthly MODIS snow-cover lookup (or a user override), the `snow_*`/`ice_*` layer textures, per-asset WEATHER masks and SNOW_ALBEDO_LUMA, and must keep snow off assets with `0 0 0 0`.

### Gaps
- The exact semantics of WEATHER mask channels and the four SNOW_ALBEDO_LUMA parameters are undocumented. Confirming them would need the shader sources or a RenderDoc inspection, which is out of scope here: the `.rdc` files were not opened by instruction.
- How X-Plane turns the MODIS monthly maps, METAR or user weather into a snow amount is not visible in the art. A forum report says snow is not generated from METAR, which is unverified and anecdotal. — [AVSIM thread (web)](https://www.avsim.com/forums/topic/624058-does-xplane-12-put-snow-on-the-ground-based-on-the-metar/)
- No ripple, raindrop or puddle-normal texture ships in `bitmaps/` or default scenery. Rain ripples, if rendered, are likely procedural or inside the shader package (not examined).

## Q5. How is water represented?

### Takeaway
DSF water has no shipped `.ter`; it is drawn by a built-in water shader. The art supplies only:
- per-1°-tile water colour textures (`bitmaps/world/water/<10° folder>/<tile>.png`, 15,281 PNGs, default `any.png` RGBA 13,32,51,191);
- regional sea-floor decals (`lib/g10/decals/water.dcl`);
- coastline beach art (`lib/g12/beaches.bch`);
- airport "water body" draped polygons and shoreline `.lin`s.

### Cited Findings
- No file in `1000 world terrain` has "water" in its name. Water appears only as `SURFACE water` (6 pol files), `SKIP_SURFACE water` (all 497 `.for`) and `SKIP_WATER` (3 pol, 2 lin). — local: [1000 world terrain](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20world%20terrain/)
- `bitmaps/world/water/`: 349 folders named by 10° block (`+00+000`, `+40-080`, …) holding 15,281 PNGs named per 1° tile (`+00+006.png`, `+41-074.png`), each 512×512 RGBA8, plus `any.png` (64² RGBA, constant 13,32,51,191).
  Samples: `+00+006.png` mean RGB 13.7,33.8,51.9, A 183–191; `+41-074.png` RGB 28,53,59, A 188–224. — local: [bitmaps/world/water](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/world/water/)
- **Sea floor decals:** `1200 beaches/library.txt` exports `lib/g10/decals/water.dcl` as `decal/sea_floor_1.dcl` in REGION `tropical` (bitmap `tropical.png` 360×180 L8) and `decal/sea_floor_2.dcl` elsewhere.
  Each is `DECAL_PARAMS_PROJ 1200.0 1200.0 0.0 0 0 0 0 0 0.0 0 0 0 -3.0 0 2.0 floor_1.png` (or `floor_2.png`), with a commented "LF modulator" variant. — local: [1200 beaches](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1200%20beaches/)
- **Coastlines:**
  - `lib/g12/beaches.bch` → `1200 beaches/coastlines.bch`: `TEXTURE coastlines_ALB.png` (on disk `coastlines_ALB.dds`), `TEXTURE_NORMAL 1.0 coastlines_NML.png` (2048² RGBA; B bimodal 0/255, A ≈ 0–18), `DECAL_LIB lib/g10/decals/ter_coastlines_1.dcl`, `SCALE_X/Y 2048`, `SIZE_X/Y 512`, plus `TILE`/`XONL2`/`XONR2`/`XONLG`/`XONRG` beach tiles.
  - Legacy `900 beaches/beach.bch` uses `BASE_TEX coastlines.png`, `DECAL_PARAMS … RGBA_DECAL_sandy_beach_3.png` / `DECAL_GS_sand.png`, `DITHER_ALPHA 0.1 1.0`.
  - Shore decals: `ter_shores_1.dcl`, `ter_shores_2.dcl`.
  — local: [1200 beaches/coastlines.bch](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1200%20beaches/coastlines.bch), [900 beaches/beach.bch](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/900%20beaches/beach.bch)
- **Airport water bodies** (`airport scenery/ground/water/water_body_{1,2,3}{a,b}.pol`):
  - Directives: `TEXTURE ../../textures/water_1a_ALB.png` (256² flat colour 13,41,55), `TEXTURE_NORMAL 1.0 ../../textures/water_NML.png` (1024²; B = 10 constant, A ≈ 208), `NORMAL_METALNESS`, `SNOW_ALBEDO_LUMA 0.2 0.2 0.2 0.0`, `SURFACE water`, `SCALE 30 30`, `SPECULAR 1.0`, `LAYER_GROUP beaches +1`, `NORMAL_DECAL_PARAMS 3.0 0 0 0 0 0 0.8 ../../textures/water_NML.png 0.8`, `DECAL_PARAMS_PROJ 400 400 … 0.1 seafloor_1.png` (L8 1024²).
  - Shorelines in the same folder: `shore_{grass,gravel,rocks,sand}_*_{1,2,4}m.lin`.
  — local: [airport scenery/ground/water](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/airport%20scenery/ground/water/)
- `bitmaps/shipping-lanes-for-boats.png` 1800×900 RGB8 (0.2° global grid). — local: [bitmaps](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/)

### Inferences
- The per-tile water PNGs most likely hold the water body colour (RGB) and a clarity/opacity value (A ≈ 0.72–0.88) used by the built-in water shader. `any.png` is the fallback for tiles without a file. The meaning of A is inferred, not confirmed.
- Airport water bodies imitate the global water: flat dark albedo + F0 0.04 normal map + scrolling-looking normal decal + sea-floor projected decal. An editor can render DSF water with the same recipe as an approximation.

### Gaps
- No wave, foam or ocean-spectrum textures ship in `bitmaps/` or default scenery. Waves and foam are probably procedural or in the shader package (not examined).
- How the per-tile water colour maps are sampled (per vertex or per pixel, depth-dependent?) is unknown from the art alone.

## Q6. What does lights.txt define?

### Takeaway
`Resources/bitmaps/world/lites/lights.txt` (header `A / 850 / LIGHT_SPECS`) is the built-in named-light catalogue. It holds:
- **494 unique light names**, defined through **872 overload records**: BILLBOARD_HW 366, SPILL_HW_DIR 240, BILLBOARD_SW 115, SPILL_HW_FLA 80, SPILL_SW 34, SPILL_GND 24, SPILL_GND_REV 13;
- **97 LIGHT_PARAM_DEF** parameter signatures.

V12 lights are photometric: intensity is written in candela (`375cd`, `95000cd`). Billboards come from a single 16×8-cell atlas, and software lights are driven by `sim/graphics/animation/lights/*` datarefs.

### Cited Findings
- **Header comments define the columns:**
  - Name, R, G, B, A, Size/Intensity, Cell (size, row, col), Dx, Dy, Dz, Width, Freq, Phase, Amp, Day, Dataref.
  - Per-overload parameterisable columns: BILLBOARD_HW (RGB, size, dir, width, freq/phase), BILLBOARD_SW (+A, dref), SPILL_GND (size), SPILL_HW_DIR (RGBA, size, dir, width), SPILL_HW_FLA (RGBA, size, freq, phase), SPILL_SW (RGBA, size, dir, width).
  - Notes in the file: "Intensity for V12 lights expressed in Candela (cd)"; "FROM V12 THERE IS ONLY ONE BILLBOARD X LIGHT, TINTED WITH SAME RBG VALUES OF THE SPILL"; a light's front faces south (+Z).
  - Width = cos(half viewing angle) for spill. Amp > 1000 gives a square wave with pulse width (amp-1000)/1000. Negative size = axial billboard.
  — local: [lights.txt](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/world/lites/lights.txt)
- **Atlas:** `TEXTURE 1000_lights_close.dds`, `X_DIVISIONS 16`, `Y_DIVISIONS 8`. The disk holds `1000_lights_close.png`, 1024×512 RGB8. `#LIGHT_PARAMS 0.69 0.08 0.005 1.0` (decay exponent, scale, glare distance calibration) is commented out. — local: [lights.txt](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/world/lites/lights.txt)
- **Sections:**
  - CAR AND ROAD LIGHTS; V12 AIRPLANE LIGHTS ("all photometric, all clean"); V11 AIRPLANE LIGHTS (v9/v10 compatibility components)
  - beacons, strobes, navigation lights, STATIC aircraft NAV lights, In-Cockpit Spills, Plane-Maker preview lights
  - "Sergio's REAL airport lights … NOW UPGRADED TO V12 WITH INTENSITY IN CANDELA"; "LEGACY X-PLANE 10/11 BEACON LIGHTS"; HIRL/approach set to 25% intensity with brightness levels; OLD OBJECTS; SHIPS AND SAILBOATS
  — local: [lights.txt](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/world/lites/lights.txt)
- **Example records:**
  - `BILLBOARD_HW taillight 0.9 0.02 0 1 375cd 1 6 6 0 0 1.0 -0.4 0 0 0 0`
  - `SPILL_HW_DIR apt_beacon_green_axial 0.05 1.0 0.6 1.0 95000cd 0 0.087 -0.996 0.939 0 # 5° up`
  - `SPILL_HW_FLA obs_strobe_night 0.99 0.97 1.33 1 3000cd 1 0 10 0`
  - `BILLBOARD_SW carrier_waveoff 1.8 0.16 0 1 15000cd 1 5 0 0.160 0.065 0.985 0.5 sim/graphics/animation/lights/carrier_waveoff`
  - `LIGHT_PARAM_DEF airplane_landing_bb 9 R G B INDEX INTENSITY DX DY DZ WIDTH`
  - `SPILL_HW_DIR full_custom_halo R G B A SIZE DX DY DZ WIDTH 1`
  - RGB values above 1.0 occur (e.g. 1.36, 4.37), i.e. HDR tints.
  — local: [lights.txt](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/world/lites/lights.txt)
- **Datarefs:** 142 lines reference `sim/...`, most often `sim/graphics/animation/lights/fresnel_vertical` (19), `wigwag` (10), `airplane_landing_light` (10), `airplane_beacon_light_rotate` (10), `strobe` (8), `rabbit` (8). — local: [lights.txt](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/world/lites/lights.txt)
- **Usage in default scenery OBJs:** 326 distinct (type, name) pairs. Top:
  - `LIGHT_PARAM spot_params_sp_pm` 2282, `spot_params_bb_pm` 1474, `airplane_generic_static` 1074, `spot_params_sp_day_pm` 582, `spot_params_bb_day_pm` 572, `spot_params_sp` 402
  - `LIGHT_NAMED airplane_nav_tail_static` 1014, `RoadBridgeDeck_var1` 652, `appch_w` 641, `airplane_nav_right/left_static` 513 each, `obs_red_night` 315
  - `LIGHT_PARAM UV_cladding_ST` 280
  — local: scan `light_usage.txt` (scratchpad) over [default scenery](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/)
- **Related light inputs:**
  - OBJ `LIGHT_CUSTOM` (2 files, `sim/graphics/animation/elevator_params`) and legacy `VLIGHT` (2 files in 900 europe objects/legacy).
  - Emissive textures via `TEXTURE_LIT` + `GLOBAL_luminance` (nits).
  - `Resources/default data/DOF.DAT` (79.6 MB, FAA Digital Obstacle File, currency 08/31/25, with an "LT" lighting column).
  — local: [default data](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20data/); (web) [lights.txt spec listed](https://developer.x-plane.com/docs/specs/)

### Inferences
- A faithful light renderer needs per-light candela intensity, a cosine-cutoff directional spill (Width), flash waveforms (triangle/strobe/square via Amp), and day/night gating (Day). It also needs the atlas cell lookup (16×8 grid on 1024×512, size = power-of-two cell multiplier) and dataref-driven parameterisation for `_SW` and LIGHT_PARAM lights.
- `DOF.DAT`'s lighting column (e.g. `R`) probably decides which obstacle objects get `obs_red_night`-style lights. Not verified.
- Many lights keep "V11"/"legacy" sections purely for compatibility. For scenery only, the airport, obstacle, road and spot (`spot_params_*`) families matter most.

### Gaps
- The `spot_params_*` LIGHT_PARAM_DEF parameter meanings were not exhaustively tabulated.
- The lights.txt web specification page was not fetched.

## Q7. Which normal-map / material-map channel conventions do the shipped textures follow?

### Takeaway
Two layouts coexist:
- **NORMAL_METALNESS layout (XP11+/XP12):** RG = tangent normal XY, **B = F0/metalness** (≈10–13/255 for dielectrics, high for metal), **A = gloss/specular**. Z must be reconstructed from RG.
- **Legacy layout:** RGB = full normal with B = Z, A = specular level. 8-bit **grayscale "NML" PNGs are specular-only masks**.

Sampling confirms the split. Of 427 normal textures referenced, 308 are used only with NORMAL_METALNESS, 102 only without, and 17 by both, which is an art inconsistency.
Forest normals use a third mode, NORMAL_TRANSLUCENCY, where B = translucency. Terrain normals (`NORMAL_TEX`) have B = 0 even where the `.ter` omits NORMAL_METALNESS.

### Cited Findings
- **Usage mapping:** 427 distinct normal textures referenced by `TEXTURE_NORMAL`/`TEXTURE_DRAPED_NORMAL`/`NORMAL_TEX`; with-NM only 308, without-NM only 102, mixed 17. Examples of mixed use: `airport scenery/textures/hangars_1_NML.png` (507 NM users, 9 non-NM); `1000 autogen/global_objects/textures/clutter_industrial1_NML.png` (139 / 74). — local: scan `normal_usage.tsv` (scratchpad)
- **NM-group samples.** |Zrecon−B| is the mean |sqrt(1−x²−y²) − B| in [-1,1] units; ~0 means B holds Z.
  - `1000 roads/textures/RailUS_NML.png`: B = 0 everywhere, A mostly 0.
  - `1000 world terrain/textures12/apt/apt_soil_NML.png`: B = 0, A 8–23; `apt_grass_rows_02_NML.png`: B = 0, A ≈ 252.
  - `airport scenery/textures/EDDM_04_NML.png`: B median 0 (50% < 16), max 214; A 6–242.
  - `sim objects/vr/holodeck/floor_NML.png`: **B = 10 constant**; `bld_EUhigh2_NML.png`: B ≥ 10 (99% < 16); `clutter_industrial1_NML.png`: B ≥ 13.
  - Mean |Zrecon−B| across these is 1.6–2.0, so **B is not Z**.
  — local: scan `normal_stats.py` output
- **Legacy-group samples:**
  - `1000 autogen/US/urban_high/textures/HighRise1_NML.png` (160 non-NM users): B mean 196, |Zrecon−B| = 0.46, A 0–255 (specular).
  - `1000 autogen/US/suburban/textures/sub_res_SW1_GND_NML.png`: B ≈ 249, |Zrecon−B| = 0.039, A ≈ 0–17, so **B is Z**.
  — local: scan `normal_stats.py` output
- **Grayscale NML:** 94 `_NML` PNGs are L8. Examples: `1000 autogen/EU/textures/bld_EUmid1_NML.png` (2048² L8, 81% < 16), static aircraft liveries `CRJ1_AUA_NML.png`, `DC10_FDX_NML.png`, `MD80_DAL_NML.png`.
  (web) Standard Shading: "Grayscale: specular level in the gray channel"; "RGBA: tangent-space normals in RGB, specular level in alpha"; normal maps should be PNG.
  — local: scan output; [Standard Shading Options (web)](https://developer.x-plane.com/article/standard-shading-options-for-x-plane-10-scenery/)
- **Forest translucency maps:** `1200 forests/textures/trees_3D1_NML.png` (2048×4096 RGBA8, used with `NORMAL_TRANSLUCENCY`) has B bimodal (p5/50/95 = 0/0/255) and A 201–246. `trees_3D1_NML_wi.png` B 73% < 16, 14% > 239. — local: [1200 forests/textures](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1200%20forests/textures/)
- **Terrain normals:** `textures10/shared/clumpy_turf4_NML.png` (used by `NORMAL_TEX 20,000000` in terrain10 `.ter` files without NORMAL_METALNESS) and `textures12/apt/apt_ground1_NML.png`: RG ≈ 128, **B = 0**, A 232–255. — local: [textures10/shared](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20world%20terrain/textures10/shared/)
- **Weather and water normals in NM layout:** `snow_NML.png` B = 10, A = 153; `ice_NML.png` B = 13, A = 192; `water_NML.png` B = 10, A ≈ 208. `bitmaps/world/moon_NML.png` is legacy RGB (B ≈ 253 = Z). — local: [bitmaps/world/weather](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/world/weather/), [airport scenery/textures](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/airport%20scenery/textures/)
- **Normal decals** (`NDHF_*`, `NDLF_*`): RG normal, B = 0. **TEXTURE_MAP normal** (`Chocks2_NRM.png`): RG normal, B = 0. **TEXTURE_MAP material_gloss** (`Chocks2_MAT.png`): R = 10 constant, G = 74–145, B = 0. — local: [sim objects/chocks](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/sim%20objects/chocks/)
- **(web) Material model:**
  - "When X-Plane's normal/material map is in 'metalness' mode, the blue channel is used as the material F0". F0 0.00–0.08 is used as both metalness and F0; above 0.08 it counts as metal with dielectric reflection clamped to 0.08.
  - Recommended values: 0.0 for self-shadowing grass, 0.02–0.04 for plastic/concrete/vinyl, 0.06 for glass.
  - A translucency-mode normal map sets F0 to 0.0.
  - With no normal map and gloss > 0, F0 = 0.04.
  — [XP12 material model (web)](https://developer.x-plane.com/article/the-x-plane-12-material-model)
- **(web) OBJ8:** `ATTR_shiny_rat` modulates the normal map alpha; `GLOBAL_specular` must be non-zero for the specular/alpha channel to show; NORMAL_METALNESS may follow TEXTURE_NORMAL or TEXTURE_DRAPED_NORMAL. — [OBJ8 spec (web)](https://developer.x-plane.com/article/obj8-file-format-specification/)

### Inferences
- The shipped B ≈ 10/255 (0.039) baseline on concrete, hangar, snow and water maps matches the web guidance of F0 0.02–0.04 for dielectrics. B = 0 on grass and terrain matches "0.0 for self-shadowing effects like grass". Treat B as linear F0 (0–1), with a metal threshold above ~0.08 (≈ 20/255).
- In NORMAL_METALNESS mode A is a gloss value (1 − roughness or similar) scaled by GLOBAL_specular/SPECULAR. In legacy mode A is the specular level. The exact gloss-to-roughness curve is not documented.
- Terrain `NORMAL_TEX` maps with B = 0 but no NORMAL_METALNESS flag suggest X-Plane always reconstructs Z for terrain, or treats terrain as metalness mode implicitly. An editor should not use B as Z for terrain normals.
- For the 17 mixed-use textures, X-Plane's own non-NM users will look wrong under a strict legacy interpretation. Match X-Plane per material flag rather than per texture.

### Gaps
- The exact gloss-to-roughness mapping, and whether SUPER_ROUGHNESS overrides or clamps it, cannot be read from the art.
- Whether X-Plane reconstructs Z for legacy (non-NM) maps or uses B directly could not be verified without shader inspection.

## Q8. Which support textures ship in `Resources/bitmaps/` (and adjacent folders), and what are they?

### Takeaway
The rendering-relevant part of `bitmaps/` is small:
- monthly MODIS snow cover, weather snow/ice layers and noise
- per-tile water colours, sky-colour tables, cirrus density and smoke puff
- the light atlas, taxi/runway sign atlases, moon/Earth-from-orbit textures
- 2D cockpit overlays (rain, snow, ice, prop disc)

There are **no** BRDF, colour-grading, blue-noise, rain-ripple or ocean-wave textures in `bitmaps/`. The only BRDF LUT is the raw file `Resources/brdf_lookup` (524,288 bytes), which decodes as a 256×256 two-channel float32 table. Most of `bitmaps/` (19,921 PNGs) is cockpit and UI art.

### Cited Findings
- **Image headers** read from PNG/DDS/KTX2:

| File | Format / size | Class |
|---|---|---|
| `Snow Cover/MOD10C1_M_SNOW_2025-01…12.png` (12) | PNG 3600×1800 palette | climate snow cover (monthly) |
| `world/weather/snow_ALB.png` / `snow_DCL.png` / `snow_NML.png` | PNG 512² RGBA / 2048² L8 / 1024² RGBA | shader snow layer |
| `world/weather/ice_ALB.png` / `ice_DCL.png` / `ice_NML.png` | PNG 2048² RGBA / 1024² RGBA / 1024² RGBA | shader ice layer |
| `world/weather/noise.png` | PNG 1024² L8 | weather noise |
| `world/water/<10°>/<1° tile>.png` (15,281) + `any.png` | PNG 512² RGBA (any.png 64²) | water colour per tile |
| `skycolors/sky_colors_{clean,foggy,hazy,hialt,mount,ocast,orbit,snowy,socked,stratus}.png` | PNG 256×640 RGB8 | sky colour tables, one per weather type |
| `world/clouds/cirrus1_density.png` / `smoke_puff.png` | PNG 2048² LA8 / 512² RGBA | cirrus density, smoke particle |
| `world/lites/1000_lights_close.png` | PNG 1024×512 RGB8 | light billboard atlas (16×8) |
| `runways/taxi.dds` / `taxi_LIT.dds` | DDS 2048×1024 DXT5 / DXT1, 12 mips | taxi sign atlas + emissive |
| `world/moon.dds` / `moon_NML.png` / `moon_NML.ktx2` | DDS 1024² DXT1 / PNG 1024² RGB / KTX2 vkFormat 131, 11 levels | moon (the only KTX2 in bitmaps) |
| `earth1.dds`, `earth2.dds` / `earth1-nrm.png`, `earth2-nrm.png` | DDS 4096² DXT5 / PNG 2048² RGBA | Earth from orbit |
| `Earth Orbit Textures/±LL±LLL{.dds,-ele.png,-nrm.png}` (354 dds + 648 ele + 648 nrm) | DDS 2048² DXT5; ele PNG 2048² L8; nrm PNG 2048² RGB8 | orbital Earth tiles + elevation + normals |
| `world/overlays/{bird_strike,hail_strike,helo_disc,ice,prop_disc*,static,target_arrows,wake}.png` | PNG RGBA, 128²–2048×512 | 2D/cockpit overlays |
| `world/overlays/windshield2d/2d_rain_drops.png` / `2d_snow_flakes.png` | PNG 128×256 / 8×128 RGBA | 2D windshield precipitation |
| `world/maps/{EGPWS,NEXRAD,RADAR,clouds,color-ir,g1000_land,sec_land}.png` | PNG 256×2 or 256×64 | 1D palettes for map displays |
| `world/maps/RWY_markings_1.png`, `g1000_noise.png`, `map_lightning.png`, `obstacles.png` | PNG | map display art |
| `world/powerlines.png` | PNG 256×2 RGB8 (constant ≈ 45,46,52) | power-line colour |
| `world/chute.dds` | DDS 128² DXT5 | parachute |
| `shipping-lanes-for-boats.png` | PNG 1800×900 RGB8 | boat placement map |
| `white.png`, `checkerboard.png`, `missing.png`, `black*.png`, `blank.png` | 2×2 / tiny | placeholders |

  — local: [bitmaps](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/) (headers via `imghdr_scan.py`)
- **File totals in `bitmaps/`:** 19,921 PNG, 372 DDS, 238 TXT (cockpit instrument definitions), 1 KTX2. Folders `cockpit/`, `interface/`, `interface11/` and `icons/` hold UI and panel art. — local: [bitmaps](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/)
- **`Resources/brdf_lookup`:** 524,288 bytes = 256×256×2 float32. Decoded as little-endian (x, y) pairs: (0,0) → (0.0, 0.995); (255,0) → (1.0, 0.0); (128,0) → (0.969, 0.031); (0,255) → (0.939, 0.046); (255,255) → (0.312, 0.0). Range 9e-13 to 1.00004. — local: [Resources](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/)
- `Resources/effects/` holds particle systems (`*.pss` + atlas PNGs: aircraft_explosion, aircraft_system, bullet_explosion, weapon_system). This is outside the requested scope but adjacent. — local: [effects](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/effects/)
- `Resources/default data/` contains no rendering tables: no season, climate or landuse tables. It holds navdata (`earth_*.dat`, 15,920 CIFP `.dat`), `airspaces/`, `DOF.DAT` (obstacles) and ReadMe notes. — local: [default data](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20data/)

### Inferences
- `brdf_lookup` is consistent with a split-sum environment-BRDF LUT (scale, bias). The y = 0 row runs from (0, ~1) at grazing to (1, 0) at normal incidence, which is the expected roughness-0 behaviour. The values at high roughness do not match the common Karis UE4 LUT, so X-Plane may use a different roughness parameterisation or orientation (unverified).
- The sky-colour PNG tables are probably XP10/11-era legacy kept for compatibility; XP12 atmosphere is computed, and other researchers cover settings/shaders.
- Noise, blue-noise, cloud volumes, colour-grading LUTs and rain ripples are likely embedded in the shader package or the executable. They were not found as files in the scoped folders.

### Gaps
- The `brdf_lookup` axis orientation and roughness mapping were not confirmed.
- Whether `skycolors/*.png` is still sampled in XP12 cannot be determined from files alone.

## Q9. What legacy (pre-XP12 / XP11-and-older) content still ships?

### Takeaway
A large share of the default library is XP9/XP10/XP11-era content kept for old DSFs and add-ons:
- XP9 terrain (`terrain/`, `pol9/`, exported under `lib/g8` and `lib/terrain`, 4,017 export lines) is the only user of `AUTO_VARY`/`LIT_TEX`;
- 121 OBJ7 (`A 700`) objects, the `700/900 *` packages, and VLIGHT/LINES/VLINE geometry;
- legacy non-metalness normal maps and grayscale specular maps;
- lights.txt v9/v10/v11 compatibility sections; `library.txt_old`; `interface11`.

### Cited Findings
- **Terrain library export roots:** `terrain10` → `lib/g10` 12,180; `terrain` → `lib/g8` 2,185 and `lib/terrain` 699; `pol9` → `lib/g8` 1,133; `terrain12` → `lib/g10` 570; `terrain_fx` → `lib/airport` 350; regional `terrain10_*` → `lib/g10` 1,158 (VLC 704, NHS 200, CANsq 144, SHS 36, NAW 30, SEA 24, AUS 20).
  All 263 `AUTO_VARY` and 400 `LIT_TEX` lines are in the XP9 `terrain/` folder. — local: [1000 world terrain/library.txt](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/1000%20world%20terrain/library.txt)
- **OBJ versions:** 121 `.obj` with version 700 (e.g. `900 us objects/buildings/B1_a1_10x10.obj`, `sim objects/legacy env files/smoke_stack.obj`) vs 8,023 at 800. `VLIGHT`, `LINES` and `VLINE` (7,879 lines in 53 files) appear in `900 europe objects`. — local: [900 us objects](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/900%20us%20objects/)
- **Legacy packages:** `700 roads` (net + 5 dds), `900 roads`, `900 forests` (23 `.for`), `900 beaches`, `900 europe objects`, `900 us objects`, `900 world object placeholders`. Library sections are marked `SEMI_DEPRECATED` (23) and `DEPRECATED` (17). `1000 world terrain/library.txt_old` (4,036 lines) is still on disk. — local: [default scenery](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/default%20scenery/)
- **Legacy material data:** 102 normal textures are used only without NORMAL_METALNESS (B = Z layout); 94 `_NML` are grayscale specular-only (XP10 convention); 30 `.for` and 137 `.fac` still carry version 800 headers. — local: scan outputs (see Q7)
- **lights.txt** keeps "V11 AIRPLANE LIGHTS" (v9/v10 compatibility) and "LEGACY X-PLANE 10/11 BEACON LIGHTS … ONLY HERE FOR COMPATIBILITY". — local: [lights.txt](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/bitmaps/world/lites/lights.txt)

### Inferences
- An editor that loads arbitrary third-party or Gateway scenery must still support the XP9 terrain directives (`LIT_TEX`, `AUTO_VARY`, `COMPOSITE_TEX` without params, `NORMAL_TEX`), OBJ7 (or skip it gracefully), and the legacy normal/specular layouts. Shipped XP12 assets alone exercise only the terrain10/terrain12 and NORMAL_METALNESS paths.

### Gaps
- Which legacy library paths are still referenced by XP12's shipped global DSFs was not checked; DSF parsing is out of scope for this pass.
