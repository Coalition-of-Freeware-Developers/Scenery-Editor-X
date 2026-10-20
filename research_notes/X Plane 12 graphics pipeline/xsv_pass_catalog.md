# X-Plane 12 GPU pass catalog, reverse-read from the local shader binding maps (xsv), SPIR-V/MSL archives, X-Plane.exe strings and logs

Source key used below (all local, read-only; links are `file:///` URLs):
[MAP](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/) = `Resources/shaders/bin/<family>_mapping.xsv` ·
[SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/) = `bin/spv/<family>.xsa` (zip of SPIR-V modules, debug names kept) ·
[MSL](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/msl/) = `bin/msl/<family>.xsa` (zip of Metal source cross-compiled from the same SPIR-V) ·
[EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe) = printable strings of `X-Plane.exe` ·
[LOG](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Log.txt) = 12.4.3 log ·
[LOG06](file:///G:/Repositorys/Coalition-of-Freeware-Developers/Scenery-Editor-X/XP12_Vulkan_Log.txt) = 12.06b1 log ·
[BRDF](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/brdf_lookup) = `Resources/brdf_lookup` ·
[SCRIPTS](file:///C:/Users/traye/AppData/Local/Temp/claude/G--Repositorys-Coalition-of-Freeware-Developers-Scenery-Editor-X/67f57d29-13ad-4b5f-ab75-468c7bb89349/scratchpad/xsv/) = parsing scripts and raw outputs (`reflect.py`, `mapping.py`, `groups.py`, `exestrings.py`, `famctx.py`, `brdf.py`, `brdf_check.py`, `out/reflect_all.txt`, `out/family_context.txt`, `out/mapping_summary.txt`).

Scope note: the mapping files only carry names, descriptor types and permutation tables. The same folder also ships `spv/` and `msl/` archives with full per-permutation reflection data (descriptor sets/bindings, image formats, workgroup sizes, struct members, shader I/O). I used them to check and expand what the mapping files say. Every "Cited Finding" below comes from these files. Anything marked **Inference** is my interpretation.

## 0. Build identity: which X-Plane do these files belong to?

### Takeaway
The installed `X-Plane.exe` and shader archives are **12.4.4-b4 (compiled Oct 6 2026)**, not 12.4.3 build 124311. Only `Log.txt` comes from 12.4.3-r2 (build 124311, compiled Jun 24 2026, session of Aug 12 2026). The catalog below describes the installed 12.4.4-b4 shaders. A 12.4.3 binary is not present on disk, so 12.4.3 and 12.4.4 cannot be diffed.

### Cited Findings
- The exe contains the strings `12.4.4-b4`, `12.4.4-b4-fbbdaacd`, `12.4.4`, `Compiled on Oct  6 2026 15:01:15` and `124414` — [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- The log header reads "X-Plane 12.4.3-r2-15ff1e4d (build 124311 ...)", "Compiled on Jun 24 2026", "Started on Wed Aug 12 19:23:46 2026" — [LOG](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Log.txt)
- File dates: `X-Plane.exe` 2026-10-06; `spv/*.xsa` Sep 10 / Oct 1 2026 (deferred_gbuf, ssr_deferred, cloud_render_*, weather_apply_* on Oct 1); mapping files are dated Dec 19 2025, May 9 2026 or Sep 10 2026; `brdf_lookup` 2025-10-11 — [MAP](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/)
- The bin folder holds **97** `*_mapping.xsv` files plus the `spv/` and `msl/` folders (99 entries in total, which is the "99 files" in the brief). `spv/` has 98 archives (one extra, `empty.xsa`); `msl/` has 97 — [MAP](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/)

### Inferences
- **Inference:** `124414` is probably the 12.4.4 build number, matching the "124311" pattern of 12.4.3.
- **Inference:** the mapping files dated Dec 19 2025 have probably not changed since an earlier 12.x release. Families re-dated Sep/Oct 2026 (deferred_gbuf, terrain, hdr, light_tile, light_spill_compact, weather_apply_*, cloud_map, debug) changed in the 12.4.x cycle.

### Gaps
- No 12.4.3 shader binaries are available, so whether a pass changed between 12.4.3 and 12.4.4-b4 is unknown.

## 1. Pass families: what each binds, and what that implies about its algorithm

### Takeaway
The 97 families form a modern deferred pipeline. In the order the data flows:
1. A GPU-driven G-buffer, written by the `terrain` uber-shader with 5 MRTs.
2. Screen-space helpers: Hi-Z `z_minmax`, AMD FidelityFX **CACAO** SSAO, hierarchical SSR, and weather (rain/snow/ice) applied to the G-buffer.
3. A 2.5D-culled tiled light list, plus "spilled" light volumes.
4. Full-screen deferred lighting with IBL probe arrays and a GGX split-sum LUT.
5. A **Hillaire-2020-style** atmosphere: transmittance LUT, multi-scattering LUT, sky-view/skydome LUT, and a 3D aerial-perspective volume.
6. Ray-marched volumetric clouds and froxel volumetric fog, both temporally upscaled.
7. Post: luminance histogram, local tone mapping by **exposure fusion** (Laplacian/Gaussian stacks), bloom, AMD FidelityFX **DoF**, a tonemap with ASC-CDL grading and FXAA, then **FSR 1** (EASU+RCAS). There is no TAA and no motion-vector target.

### Cited Findings — overview table (all 97 families)
Kind, permutation count and module counts come from the mapping header and the SPIR-V archives. "Permutation #defines" are the upper-case strings that sit right after the family's name literal in the exe; they look like the shader's `#define` switches. Sources: [MAP](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe), generated by `table.py` in [SCRIPTS](file:///C:/Users/traye/AppData/Local/Temp/claude/G--Repositorys-Coalition-of-Freeware-Developers-Scenery-Editor-X/67f57d29-13ad-4b5f-ab75-468c7bb89349/scratchpad/xsv/).

| family | kind | permutations (mapping) | unique SPIR-V modules (v/f/c) | workgroup | permutation #defines next to the name in X-Plane.exe |
|---|---|---|---|---|---|
| acf_map_icon | raster | 4 | v1/f1 | - | - |
| airport_raster | raster | 4 | v1/f1 | - | - |
| astronomical | raster | 12 | v6/f3 | - | BODY_TYPE |
| atmosphere | raster | 24 | v1/f6 | - | (name not found as a literal) |
| background_blur | raster | 128 | v5/f12 | - | - |
| blit | raster | 32 | v1/f8 | - | - |
| bloom | compute | 4 | c3 | 16x16x1 | SRC_TYPE |
| blur | raster | 8 | v2/f2 | - | - |
| cacao | compute | 114 | c26 | 8x8x1,1x1x1,16x16x1,4x16x1 | MODE, RESOLUTION |
| cloud_categorize | raster | 12 | v1/f3 | - | - |
| cloud_map | raster | 128 | v2/f11 | - | QUALITY, CLEARTO, COLORS |
| cloud_minify | compute | 7 | c7 | 16x16x1 | - |
| cloud_precipitation | compute | 1 | c1 | 16x16x1 | - |
| cloud_render_compute | compute | 108 | c24 | 16x16x1 | - |
| cloud_render_raster | raster | 864 | v4/f48 | - | LIGHTING_MODE, TRACE_COLOR, TRACE_PRECISION, TRACE_MODE |
| cloud_shadows | compute | 3 | c3 | 16x16x1 | - |
| cloud_update | compute | 2 | c2 | 16x16x1 | - |
| cloud_upscale | compute | 4 | c4 | 16x16x1 | - |
| compute_fft | compute | 24 | c24 | 16x16x1 | PERMUTE |
| cube_filter_compute | compute | 4 | c4 | 16x16x1 | USE_ALPHA, USE_COS |
| cube_filter_raster | raster | 16 | v1/f4 | - | - |
| debug | raster | 1152 | v8/f144 | - | TARGET, SRGB, IS_3D, SEP_SAMP, IS_SPARSE |
| deferred_gbuf | raster | 20480 | v3/f988 | - | DEFERRED_MODE, WANT_SHAD, SOFT_SHADOW_MODE, SCATTER, SSAO, METERING, GBUF_FMT |
| depth_of_field | compute | 40 | c7 | 8x8x1,16x16x1 | FFX_DOF_OPTION_MAX_RING_MERGE_LOG, FFX_DOF_OPTION_COMBINE_IN_PLACE, FFX_DOF_OPTION_REVERSE_DEPTH |
| depth_resolve | raster | 8 | v1/f1 | - | IS_MSAA |
| device_hang | compute | 1 | c1 | 64x1x1 | (name not found as a literal) |
| dome | raster | 32 | v1/f6 | - | PROJECTION_MODE, LAYER |
| exposure_fusion | compute | 16 | c10 | 16x16x1 | TYPE, HAS_MS |
| fake_terrain | raster | 64 | v4/f5 | - | WANT_PBR, WANT_INSTANCING |
| fix_hdr | compute | 2 | c2 | 8x8x1 | - |
| font | raster | 24 | v4/f3 | - | - |
| fsr | compute | 6 | c6 | 64x1x1 | PRECISION? (sits between 'CACAO' and 'FSR target') |
| ground_lights | raster | 4 | v1/f1 | - | - |
| gstack | compute | 144 | c54 | 16x16x1 | GSTACK_MODE, OUTPUT_FORMAT, FILTER_NAN |
| hdr | raster | 960 | v3/f13 | - | FXAA_FORMAT, SHOW_FXAA_STENCIL, EXPOSURE_FUSION, FALSE_COLOR (attributed: block before 'bloom' in OGL_hdr.cpp) |
| histo_debug | raster | 4 | v1/f1 | - | - |
| in_place_blur | compute | 2 | c2 | 16x1x1 | DIRECTION |
| legacy_depth | raster | 144 | v8/f27 | - | - |
| legacy_flat | raster | 288 | v12/f45 | - | NUM_TU, GAMMA, ALPHA_TEST, PHOTOMETRIC_AWARE, PREMULTIPLIED, IMM_GROUP0 |
| light | raster | 24 | v4/f1 | - | USE_NEW_FOG |
| light_spill_compact | compute | 2 | c2 | 64x1x1 | LAYOUT |
| light_tile | compute | 6 | c6 | 16x16x1 | TILE_MODE |
| light_transform | compute | 2 | c2 | 64x1x1 | - |
| light_vis | raster | 8 | v2/f1 | - | SOURCE_MODE |
| line3d | raster | 12 | v2/f3 | - | - |
| lumi_histo | compute | 6 | c4 | 16x16x1 | - |
| manip3d | raster | 8 | v1/f2 | - | - |
| minify | compute | 240 | c240 | 256x1x1 | CONFIG, COLOR, USE_SUBGROUP_OPS |
| mixed_reality | raster | 48 | v4/f4 | - | MESH_SRC |
| msaa_categorize | raster | 4 | v1/f1 | - | SAMPLE_COUNT; spec: SAMPLE_COUNT |
| ocean_bake_textures | compute | 2 | c2 | 8x8x1 | - |
| ocean_mesh_bake | compute | 2 | c2 | 8x8x1,64x1x1 | - |
| ocean_mesh_data | compute | 2 | c2 | 1x1x1 | - |
| ocean_meta_data | raster | 1024 | v13/f14 | - | WANT_DECALS |
| ocean_readback | raster | 8 | v2/f2 | - | - |
| ocean_shading | raster | 256 | v4/f28 | - | ENABLE_TURBIDITY, VISUALIZE |
| ocean_spectrum | compute | 3 | c3 | 8x8x1 | - |
| ocean_variance | compute | 2 | c2 | 4x4x1 | - |
| particle | raster | 432 | v3/f36 | - | CROSSFADE, ZFORMAT, CLOUD_OPACITY |
| planet | raster | 8 | v1/f2 | - | - |
| profile_view | raster | 4 | v1/f1 | - | - |
| rain | raster | 4 | v1/f1 | - | - |
| rain_draw | raster | 8 | v2/f1 | - | - |
| rain_droplets_compute | compute | 12 | c6 | 64x1x1 | LAST_ITERATION |
| rain_droplets_wipers | compute | 2 | c2 | 8x8x1 | - |
| rain_forces | raster | 8 | v1/f2 | - | - |
| rain_forces_visualize | compute | 1 | c1 | 8x1x8 | - |
| rain_ice_accumulation | compute | 4 | c3 | 8x8x1 | HAS_THERMAL_SOURCES |
| rain_normals | raster | 4 | v1/f1 | - | - |
| rain_surface | raster | 8 | v1/f2 | - | RIP_FORMAT |
| rain_visualize | compute | 1 | c1 | 64x1x1 | - |
| resolve | raster | 48 | v2/f7 | - | - |
| ripple | compute | 1 | c1 | 16x16x1 | - |
| scatter_compute_multi | compute | 1 | c1 | 8x8x1 | - |
| scatter_compute_scattering | compute | 4 | c4 | 16x16x1 | USE_ZBUF |
| scatter_compute_skydome | compute | 8 | c6 | 16x16x1 | SHADOW_MODE, OUTPUT_MODE |
| scatter_compute_t | compute | 1 | c1 | 16x16x1 | - |
| scatter_render_atmosphere | raster | 48 | v2/f9 | - | SCATTER_MODE |
| sectional2 | raster | 64 | v1/f10 | - | COLOR_CONVERT, EGPWS_STYLE |
| shading_rate | compute | 2 | c2 | 8x8x1 | HAS_SHAPES |
| shading_rate_vis | raster | 4 | v1/f1 | - | - |
| shadow_rect | raster | 4 | v1/f1 | - | - |
| single_pass_blur | compute | 2 | c1 | 8x8x1 | MINIFY |
| ssr_deferred | compute | 24 | c24 | 16x16x1 | GBUF_TYPE, TRACE_FROM, REF_MODE, READ_MODIFY_WRITE |
| ssr_mesh | raster | 8 | v1/f2 | - | SSR_TYPE |
| svt | raster | 4 | v1/f1 | - | (name not found as a literal) |
| tchotchke | raster | 8 | v2/f2 | - | - |
| terrain | raster | 829440 | v378/f4236 | - | GEOM_MODE, USE_SHAD, TONE_MODE, BORDER_MODE, TEX_MODE, PROJ_BASE, WANT_SOFT_SHADOWS, ALPHA_TO_COVERAGE, ALPHA_MODE, HAS_DECS, HAS_NORM_DECS, NITE_MODE (OGL_terrain_shader.cpp block); spec: HAS_NORM_DECS, HAS_DECS, ALPHA_MODE, NITE_MODE, BLEND_MODE |
| vegetation_cull | compute | 2 | c2 | 64x1x1 | - |
| vegetation_data | compute | 12 | c6 | 32x1x1 | PHASE, IS_LEGACY |
| volumetric_apply | raster | 48 | v1/f6 | - | - |
| volumetric_fog | compute | 4 | c4 | 64x4x1 | APPLY_MODE |
| vr_depth_copy | raster | 8 | v1/f2 | - | FORMAT |
| weather_apply_compute | compute | 16 | c16 | 8x8x1 | - |
| weather_apply_raster | raster | 64 | v1/f16 | - | DEBUG_MODE |
| wxr_mask | raster | 4 | v1/f1 | - | (name not found as a literal) |
| z_minmax | compute | 9 | c9 | 16x16x1 | ZMIN_MODE |

Notes on the table: "permutations" is the full permutation space declared in the mapping. "Unique modules" is the number of deduplicated SPIR-V modules (v = vertex, f = fragment, c = compute). For example, `terrain` declares 829,440 permutation slots that resolve to 4,614 modules, and `deferred_gbuf` declares 20,480 slots that resolve to 991.

### Cited Findings — per-pass catalog
Formats come from the SPIR-V `OpTypeImage` format of storage images. Sampled textures carry no format in SPIR-V. "sN.bM" means descriptor set N, binding M.

**Atmosphere / sky**
- `scatter_compute_t` (compute, 16×16×1): writes `img_output_t` = image2D **rgba16f**; UBO `u_scattering_data`. The UBO has 27 members, including: `u_plan_rad_mtr`, `u_atmo_rad_mtr`, `u_mie_limit`, `u_scatter_r`, `u_scatter_m`, `u_absorb_o`, `u_absorb_m`, `u_ele_r_mtr_inv`, `u_ele_m_mtr_inv`, `u_ele_o_mtr`, `u_rad_o_mtr_inv`, `u_fog_ele`, `u_absorb_f`, `u_scatter_f`, `u_planet_uv`, `u_mie_decode`, `u_scatter_resolution`, `u_ap_samples_per_m`, `u_ap_min/max_samples`, `u_dome_current_sample`, `u_dome_min/max_samples`, `u_dome_history_sample_alpha`, `u_scattering_sample_offsets[18]`. In the MSL, the kernel maps each texel to a (view-zenith, altitude) pair, integrates Rayleigh/Mie/ozone/fog density over 40 steps and writes exp(−optical depth). The exe label is "Transmittance"/`scatter_transmit` — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/scatter_compute_t.xsa), [MSL](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/msl/scatter_compute_t.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `scatter_compute_multi` (compute, 8×8×1): reads `u_trans_tex`; writes 2D **rgba16f** `img_output_t`; exe label `scatter_multi` — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/scatter_compute_multi.xsa)
- `scatter_compute_skydome` (compute, 16×16×1, 6 modules, defines `SHADOW_MODE`, `OUTPUT_MODE`): reads `tex_mie_phase`, `u_multi_tex`, `u_trans_tex`, and optionally `u_tex_cloud_shadow_cascades` + `u_shadow_data`; writes 2D **rgba16f**. UBO `u_scattering_view` = {`u_modelview_matrix_inv`, `u_camera_wrl`, `u_frustum_coefficients`, `u_L_wrl`, `u_E_sun`, `u_ap_near_far_z`, `u_fade_dist`}. Workgroup shared memory `sh_intermediate vec3[256]`. Exe labels `scatter_sky_view`, `scatter_skydome`, `scatter_ambient`, "Sky-view Image", "Skydome Image", "Sky Ambient" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/scatter_compute_skydome.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `scatter_compute_scattering` (compute, 16×16×1, define `USE_ZBUF`): writes `img_output_in_scatter` and `img_output_out_scatter`, both image3D **r11g11b10f**; reads optional `tex_gbuffer_z`, `tex_mie_phase`, `u_multi_tex`, `u_trans_tex`, cloud shadows. Exe labels `scatter_inscatter`, `scatter_outscatter`, "Skydome compute in/out" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/scatter_compute_scattering.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- Consumers of the 3D volumes (`u_tex_in_scatter` and `u_tex_out_scatter`, both texture3D): deferred_gbuf, terrain (forward variants), cloud_render_*, volumetric_fog, particle, ocean_shading, planet, light, scatter_render_atmosphere. The volumes are addressed through `u_new_sky_data` = {`u_frustum_coefficients_inv[2]`, `u_ap_near_far_z`, `u_plan_rad_mtr`, `u_atmo_rad_mtr`, `u_scatter_resolution_inv`} — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/)
- `scatter_render_atmosphere` (raster, full-screen `v_ratio`; 2 VS / 9 FS; define `SCATTER_MODE`): reads `gbuffer_pos` (2D, array or MS), `tex_mie_phase`, `u_multi_tex`, `u_trans_tex`, the in/out-scatter volumes, `u_scattering_data`, `u_scattering_view`; writes `data_out0`; exe label "Atmosphere" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/scatter_render_atmosphere.xsa)
- `atmosphere` (raster, 1 VS / 6 FS, writes up to 4 MRTs per layer via `gl_Layer`): uses Bruneton-style names — `transmittance_texture` (2D), `scattering_density_texture`, `single_rayleigh_scattering_texture`, `single_mie_scattering_texture`, `multiple_scattering_texture` (all 3D). UBO `u_atmosphere_data` = {`solar_irradiance`, `sun_angular_radius`, `rayleigh_density` (mat4), `rayleigh_scattering`, `bottom_radius`, `mie_density`, `mie_scattering`, `top_radius`, `mie_extinction`, `mie_phase_function_g`, `absorption_density`, `absorption_extinction`, `mu_s_min`, `ground_albedo`, `sky/sun_spectral_radiance_to_luminance`}. UBO `u_compute_atmosphere_data` = {`x_luminance_from_radiance`, `u_layer`, `u_scattering_order`}. No other family binds these texture names — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/atmosphere.xsa), [MAP](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/atmosphere_mapping.xsv)
- `dome` (raster, defines `PROJECTION_MODE`, `LAYER`): reads `u_tex_skydome` (2D), `u_tex_trans`, `u_dome_data` = {`u_proj_coefficients[2]`, `u_sun_dir_eye`, `u_pos_cam_wrl`, `u_sun_dir_wrl`, `u_sky_inv`, `u_dome_per_view`, `u_sun_glare_falloff`, `u_sun_glare_mask`} — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/dome.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `astronomical` (raster, `BODY_TYPE`; strings "solar limb", `earth_astro.dat`, `world/moon.png`, `moon_NML`): reads `u_tex0`, `u_tex0_nrm`, `u_tex_skydome`, `u_tex_trans`, `u_astronomical_data` {`u_light_diffuse`, `u_light_position`, `u_light_emissive`} — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/astronomical.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `planet` (orbital Earth): reads `tex_planet_albedo`, `tex_planet_normal`, `tex_planet_elevation` (VS), `tex_water`, scatter LUTs and `u_planet_params`. One FS variant writes 5 MRTs (G-buffer compatible) and one writes a single output — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/planet.xsa)

**Clouds, fog**
- `cloud_update` (compute): converts `new_weather` SSBO columns into 3D `img_current_weather_data_coverage` (**r16ui**) and `..._properties` (**rgba8**) — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/cloud_update.xsa)
- `cloud_minify` (compute): 3D SDF (**rg8**) plus jump-flood caches (**r32ui**); labels `cloud_weather_sdf`, `cloud_jump_flood_a/b`, "flood r/g", "bake r/g" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/cloud_minify.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `cloud_precipitation` (compute): 2D **r11g11b10f** `img_precipitation`, read by `volumetric_fog` as `u_tex_cloud_precipitation` — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/cloud_precipitation.xsa)
- `cloud_shadows` (compute): 2D **rgba16f** `img_cloud_shadow` with `_previous` history. `u_cloud_shadow_data` holds shadow↔world and previous-shadow reprojection matrices plus jitter. Labels `render_cloud_shadows`, `filter_cloud_shadows`, `copy_cloud_shadows`. Consumers bind `u_tex_cloud_shadow_cascades` (2D array) with `u_cloud_shadow_sun_cascades_count` and up to **9** cascade matrices in `u_shadow_data` — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/cloud_shadows.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `cloud_render_compute` (16×16, 24 modules) and `cloud_render_raster` (4 VS / 48 FS, defines `LIGHTING_MODE`, `TRACE_COLOR`, `TRACE_PRECISION`, `TRACE_MODE`) share the same inputs:
  - weather map `coverage`, `properties` and `hiz` (all texture3D);
  - `u_tex_noise_lo` and `u_tex_noise_hi` (3D), `u_tex_noise_cirrus` (2D);
  - `u_tex_sky_ambient`, `u_tex_trans`, the in/out-scatter volumes, cloud shadows, `u_tex_previous_cloud_depth`, `gbuffer_pos`.
  - Outputs: compute writes `img_output_cloud_t` **rgba16f** + `img_output_depth_t` **rg16f** (2D arrays); raster writes `data_out0` + `data_out1`.
  - Workgroup shared memory includes `s_sdf_intersections[256]`, `z_min`, `z_max`. `u_cloud_render_data` (59 members) has segment marching, scan/intersection sample counts, a skip rate and SDF skip rate, a phase g, multi-scatter length and a temporal jitter — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/cloud_render_compute.xsa), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/cloud_render_raster.xsa)
- `cloud_upscale` (compute): reads history colour/depth and source colour/depths; writes in/out cloud (**rgba16f**) and depth (**rg16f**) plus proxy versions. `u_cloud_upscale_data` has eye↔history matrices, alpha, a depth-change limit, and brightness/transmission clips. Labels "clouds-integrate", "clouds-proxy" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/cloud_upscale.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `cloud_categorize` (raster): reads `u_tex_cloud_colour`, `u_tex_cloud_depth`, `u_tex_volumetric_fog`, `gbuffer_pos`; writes `data_out0`. Labels "categorize clouds", "categorize msaa" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/cloud_categorize.xsa)
- `volumetric_fog` (compute 64×4×1; label pairs `volumetric_fog_density` / `volumetric_fog_accumulate`): writes 3D **rgba16f** `img_output_volumetric_density` and `img_output_volumetric_fog` with `u_tex_volumetric_density_history`. Reads weather maps, noise, precipitation, cloud shadows, sky ambient, scatter volumes and `gbuffer_pos`. `u_volumetric_fog_data` has 55 members: reprojection and history projection, depth slices, temporal alpha, rain/fog scatter/albedo, height-fog density/elevation/width, `u_volumetric_lights_*`, `u_volumetric_scene_z_mip/layers` — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/volumetric_fog.xsa)
- `volumetric_apply` (raster): reads `u_tex_cloud_colour0/1`, `u_tex_cloud_depths0/1`, `u_tex_volumetric_fog`, `gbuffer_pos` (2D array or MS); writes `data_out0`. Labels "volumetric fog apply", "Onscreen Clouds", "HDR clouds" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/volumetric_apply.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)

**G-buffer, depth, deferred** (details in §2–§4)
- `terrain`: the G-buffer and forward uber-shader. `fake_terrain` (defines `WANT_PBR`, `WANT_INSTANCING`) is a reduced variant with `tex_cubemap_ext` — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/fake_terrain.xsa)
- `z_minmax` (compute 16×16, `ZMIN_MODE`): reads `image_input_raw_z` or `image_input_mip_z` (2D, array or MS); writes `image_output_z` **rg32f**. `u_z_minmax_data` holds `u_clip_info_rev`, `u_clip_info_fwd`, `u_upscale`, size limits and `u_sample_count`. Labels "z-mip", "z-mip-total", "Z depth average", "Z depth first sample" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/z_minmax.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `minify` (compute 256×1×1, 240 modules, defines `CONFIG`, `COLOR`, `USE_SUBGROUP_OPS`): `spdIntermediate vec4[16][16]`, `spdCounter`, SSBO `SpdGlobalAtomicBuffer`, destination arrays of 6, 8 or 12 mips in **rgba16f / rgba8 / r16ui / r32ui** (2D or array) — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/minify.xsa)
- `depth_resolve` (raster, `IS_MSAA`): reads `u_depth_tex` (2DArrayMS); writes `data_out0` (colour, no FragDepth). Labels "depth resolve rp", "depth-resolve" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/depth_resolve.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `resolve` (raster): reads `u_tex` (2D, array, MS or MS-array) plus optional `u_tonemap_data`; `u_resolve_shader_data` = {`u_screen_size`, `u_sample_count`, `u_sample_weight`}. Labels "resolve msaa", "resolved msaa" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/resolve.xsa)

**AO / reflections**: `cacao`, `ssr_deferred`, `ssr_mesh`, `cube_filter_*` (§4).

**Post / tone**
- `fix_hdr` (compute 8×8): `u_hdr` = image2DArray(MS) **rgba16f**, read_write. In the MSL, any texel with ±inf is clamped to ±65504. Label "Fix HDR" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/fix_hdr.xsa), [MSL](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/msl/fix_hdr.xsa)
- `lumi_histo` (compute 16×16): SSBO `histogram` uint[256] and `result_data` uint[256]; reads `image_input_hdr`; `io_output` **rgba16f**. `u_histo_data` = {`u_bin_scales`, `u_blend_speed`, `u_pixel_count`, size, `u_histo_mode`, `u_spot_center`, `u_spot_radius`, `u_3d_window_count`}. Labels "lumi-histo", "Luma histogram readback", "Luma history buffer"; `histo_debug` draws it — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/lumi_histo.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `exposure_fusion` (compute 16×16; `TYPE`, `HAS_MS`): **every** resource comes in `_2d` / `_2d_arr` variants, plus `_2d_ms` / `_2d_ms_arr` for the original image:
  - storage images `img_luma_*`, `img_weights_*`, `img_accum_*` (all **rgba16f**);
  - sampled `tex_luma_*`, `tex_weights_*`, `tex_accum_*`, `tex_original_image_*`.
  - `u_exposure_fusion_data` = {`u_master_exposure`, `u_shadow_gain`, `u_mip`, `u_width`, `u_height`, `u_uv_scale`, `u_grayscale_coef`, `u_sigma2`} plus `u_tonemap_data`.
  - Labels "Lumi Stack", "Weight Stack", "Accum Stack", "EF luminance calc", "EF weight calc", "Exposure Fusion Temp", "EF blur", "EF gauss accum", "EF laplac accum", "EV %.0f".
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/exposure_fusion.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `bloom` (compute 16×16, `SRC_TYPE`): `s_rip_in` (2DArray or 2DArrayMS) → `i_rip_out` / `i_blur_in` / `i_blur_out` (**rgba16f** arrays). `u_bloom_data` = {`u_dst_width/height`, `u_sample_count(_inv)`, `u_cut_lo_hi`, `u_filter_size`, `u_offset_gain[15]`}. Labels "Bloom rip", "Bloom blur horizontal", "Bloom blur vertical", "Apply Bloom" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/bloom.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `hdr` (tonemap; raster, 3 VS / 13 FS; defines next to it: `FXAA_FORMAT`, `SHOW_FXAA_STENCIL`, `EXPOSURE_FUSION`, `FALSE_COLOR`):
  - reads `u_tex1` (HDR, 2DArray or MS), `tex_lumi_guide` (2DArray), `u_fxaa_stencil_mask` (**uint** 2DArray or MS);
  - `u_hdr_data` = {`u_offset`, `u_y_offset`, `u_bloom_start_mip`, `u_bloom_end_mip`, `u_bloom_coef`, `u_sample_count(_inv)`, `u_fxaa_stencil`};
  - `u_tonemap_data` = {`u_tonemap_mode`, `u_display_maximum`, `u_mid_gray`, `u_wcs_matrix(_inv)`, `u_wcs_lumi`, `u_asc_cdl_slope/offset/power/saturation`, `u_output_gain`, `u_output_gamma`, `u_exposure_rat`};
  - labels "Tonemap", "LDR-temp".
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/hdr.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `fsr` (compute **64×1×1**, 6 modules, 3 of them with Float16/Int16): `u_input_texture` (2DArray) + separate `u_input_sampler` → `i_output_texture` **rgba16f** 2DArray. `u_fsr_data` = {`Const0`, `Const1`, `Const2`, `Const3`, `Sample`} (all uvec4). No depth or motion inputs. Labels "FSR target", "FSR Upscale", "FSR sharpen"; graph functions `OGL_fsr_build_upscale_graph` and `OGL_fsr_build_sharpen_graph` — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/fsr.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `depth_of_field` (compute 8×8 and 16×16; `FFX_DOF_OPTION_MAX_RING_MERGE_LOG`, `FFX_DOF_OPTION_COMBINE_IN_PLACE`, `FFX_DOF_OPTION_REVERSE_DEPTH`):
  - `r_input_color` and `r_input_depth` (2DArray);
  - `rw_internal_bilat_color` (rgba16f[4] mips), `rw_internal_near` / `rw_internal_far` (rgba16f), `rw_internal_radius` / `rw_internal_dilated_radius` (rg16f), `rw_output_color` (rgba16f array);
  - SSBO `DofGlobalVars_t.maxTileRad`; workgroup shared memory `FfxDofLDS*` and `spdIntermediate*`;
  - `u_ffx_dof_ubo` = {`u_cocScale`, `u_cocBias`, `u_inputSizeHalf`, `u_inputSize`, `u_cocLimit`, `u_maxRings`, `u_layer`};
  - labels "Dilate", "Blur", "Composite", "DoF Near", "DoF Far".
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/depth_of_field.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `gstack` (compute 16×16, `GSTACK_MODE`, `OUTPUT_FORMAT`, `FILTER_NAN`): `s_input` → `i_output` (rg16f or rgba16f, 2D or array); `u_blur_data2` holds `u_kernel[16]`, `u_blend_speed`, `u_exposure_scale_compute`. `single_pass_blur` and `in_place_blur` are helpers. Labels "blur gstack", "cockpit gstack + histo" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/gstack.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `shading_rate` (compute 8×8, `HAS_SHAPES`): writes `image_output` **r8ui** 2DArray. `shading_rate_data` = {`u_target_size`, `u_fill_rate`, `u_num_shapes`, `u_flip_y`} plus `shapes` SSBO. Log string: "Building VRS attachment for %i px width ... Per eye PPD". `shading_rate_vis` is a debug view — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/shading_rate.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `vr_depth_copy` (raster, `FORMAT`): `u_depth_source` (2DArray) with `u_sample_count`; labels "VR depth surface", "Capture VR depth" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/vr_depth_copy.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)

**Water**
- `ocean_spectrum` (8×8): `u_ocean_spectrum_data` = {`u_scale`, `u_angle`, `u_spread_blend`, `u_swell`, `u_alpha`, `u_peak_omega`, `u_gamma`, `u_short_waves_fade`, `u_depth`, `u_time`, `u_size`, ...}; writes `i_h0` and `i_wave_table` (rgba16f).
- `compute_fft` (16×16, 24 modules, `PERMUTE`; labels "FFT butterfly lookup", "FFT temp storage").
- `ocean_bake_textures` → `i_target` / `i_previous` (rgba16f arrays). `ocean_variance` → 3D **rg16f** (labels "Variance build", "Variance interpolate").
- `ocean_mesh_data` / `ocean_mesh_bake` → mesh, index and indirect SSBOs ("Ocean indirect draw").
- `ocean_meta_data` (raster, `WANT_DECALS`): writes MRTs `out_water_nrm`, `out_water_meta`, `out_water_color`, or `out_depth_fetch_displace`.
- `ocean_shading` (raster, `ENABLE_TURBIDITY`, `VISUALIZE`): reads `u_water_nrm_tex`, `u_water_meta_tex`, `u_water_color_tex`, `u_variance` (3D), `u_skydome` (**cube**), `tex_ssr`, scatter volumes, CSM data and cloud shadows. `u_ocean_shading_data` has `u_spec_alpha2`, `u_spec_F0`, `u_foam_params`, `u_turbidity`, `u_cascade_data`.
- Labels "Water stencil", "Water prime", "Water Projection", "Water Depth", "mix down water"; art control `water/async_compute`.
- — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/ocean_shading.xsa), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/ocean_meta_data.xsa), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/ocean_spectrum.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)

**GPU-driven vegetation**
- `vegetation_data` (32×1×1; `PHASE`, `IS_LEGACY`) builds `indexed_indirect_command_t` commands and a "swedish" bucket allocator.
- `vegetation_cull` (64×1×1) frustum- and LOD-culls `vegetation_point_t` {`position`, `rot`, `season_rat_scale`, `rand`} into `vegetation_culled_point_t` {`index`, `wind_speed_fade`, `wind_direction`}. `u_vegetation_cull_data` has `u_frustum_plane[24]`, LOD multipliers/feather, `u_pass_type`, `u_is_shadow_pass`.
- The terrain VS reads `vegetation_info`, `_mesh`, `_points`, `_result` and `_source_points` SSBOs.
- — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/vegetation_cull.xsa), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/terrain.xsa)

**Forward / overlays / UI / avionics**
- `particle` (`CROSSFADE`, `ZFORMAT`, `CLOUD_OPACITY`; uses `gbuffer_pos`, `u_blur`, `u_tex_cubemap`, scatter volumes, cloud colour/depth, volumetric fog).
- `light` (billboards; §3) and `ground_lights`.
- `rain` (falling-precipitation quads; `u_rain_data.u_fade_distance`).
- `legacy_flat` / `legacy_depth` (`NUM_TU`, `GAMMA`, `ALPHA_TEST`, `PHOTOMETRIC_AWARE`, `PREMULTIPLIED`, `IMM_GROUP0`), `line3d`, `manip3d`, `font`, `blit`, `blur`.
- `background_blur` (UI glass; reads `tex_lumi_guide`, `u_tonemap_data`).
- `shadow_rect`: a UI drop shadow, `u_shadow_rect` = {`u_rect_size`, `u_sigma`, `u_corner_radius`}, exe label "drop shadow". This is **not** a scene shadow pass.
- `tchotchke` / `mixed_reality` (VR UI).
- `svt` = synthetic-vision terrain for the avionics, **not** sparse virtual texturing: `synthetic_vision_data` {`u_ecef`, `u_grid_size`, `u_geoid_height`}, `synthetic_vision_terrain_data` {`u_runways[3]`, `u_terr_warn`, `u_ccip_x/v`}; exe strings `svt shader`, `svt/fovpx`, "SVT tile ... requested".
- `sectional2` (`COLOR_CONVERT`, `EGPWS_STYLE`), `profile_view`, `cloud_map` (weather-radar/NEXRAD: `u_cloud_map_data.u_nexrad_*`, `u_radar_*`), `wxr_mask` (`u_wxr_mask_data.u_clip_plane`).
- `acf_map_icon`, `airport_raster`, `debug` (`TARGET`, `SRGB`, `IS_3D`, `SEP_SAMP`, `IS_SPARSE`, with `sparse_image_residency_table_t`), `device_hang` (an SSBO "sink").
- — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)

**Shared uniform blocks**
- `u_transform_block_fat` (47 families) = {`u_viewport_count`, `u_layer_count`, `u_transform_count`, `u_layer_offset`, `u_mv_offset`, `modelview_matrix[4]`, `modelview_inverse[4]`, `projection_matrix[4]`, `mvp_matrix[4]`}.
- In 48 of the 53 families that have vertex shaders, the VS writes `gl_Layer` and `gl_ViewportIndex` (`ShaderLayer` / `ShaderViewportIndex` capabilities). Most also pass a flat `v_eye`.
- `u_gbuffer_data` = {`u_viewport_size`, `u_gbuffer_size`, `u_proj_coefficients[2]`, `u_clip_info[2]`, `u_sample_offset`, `u_head_to_eye[2]`}.
- `u_environment_data` = {`u_fog_rgb`, `u_fog_scale`, `u_sun_position`, `u_exposure_ref`, `u_time`, `u_wind_speed`, `u_exposure_scale` (vec2), `u_wind_component`, `u_wind_direction`, `u_E_sun`}.
- `u_immediate_data` = immediate-mode vertex attributes (`u_imm_a_normal`, `color`, `instance_tint`, `lod_fade`, `night_level`, `texcoord0-3`, `season`, `point_size`, `photometric`, `u_imm_snow_luma_coef`).
- No SPIR-V module declares `MultiView`; none uses push constants or input attachments.
- — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/), [SCRIPTS](file:///C:/Users/traye/AppData/Local/Temp/claude/G--Repositorys-Coalition-of-Freeware-Developers-Scenery-Editor-X/67f57d29-13ad-4b5f-ab75-468c7bb89349/scratchpad/xsv/out/reflect_all.txt)

### Inferences
- **Inference (atmosphere = Hillaire 2020):** the `scatter_compute_*` set matches Hillaire's "A Scalable and Production Ready Sky and Atmosphere Rendering Technique" (EGSR 2020) one-to-one:
  - transmittance LUT (`_t`, 2D rgba16f);
  - multiple-scattering LUT (`_multi`, 2D rgba16f, small 8×8 dispatch);
  - sky-view LUT (`_skydome` with `OUTPUT_MODE` sky-view/skydome/ambient);
  - camera-frustum aerial-perspective volume (`_scattering`).
  X-Plane's variations are separate in-scatter and out-scatter (transmittance) 3D volumes in r11g11b10f, cloud-shadowed variants, a ground-fog layer in the medium, and a tabulated Mie phase (`tex_mie_phase` + `u_mie_decode`) instead of an analytic Henyey–Greenstein.
- **Inference:** the older `atmosphere` family is Bruneton's 2017 "precomputed atmospheric scattering" reference implementation (identical uniform and texture names). In this build no other shader samples its outputs, so it may be vestigial, a tool path, or a reference path.
- **Inference (exposure fusion):** the passes and parameters match local tone mapping by exposure fusion (Mertens et al. 2007, as popularised for real time by Wronski):
  - synthetic exposures (`u_master_exposure`, `u_shadow_gain`);
  - well-exposedness weights (Gaussian with `u_sigma2`);
  - Gaussian stacks for weights and Laplacian stacks for luminance, accumulated coarse-to-fine.
  Only luminance is fused. The result `tex_lumi_guide` is a guide that `hdr` (and UI `background_blur`) use to rescale colour.
- **Inference (CACAO):** `cacao` is AMD FidelityFX CACAO, given the `FFX_CACAO_*` shared memory, the `u_PatternRotScaleMatrices` … `u_DeinterleavedDepthBufferNormalisedOffset` constants, the deinterleave / importance-map / blur / bilateral-upscale stages, and the "lowest-low-medium-high-highest" quality names.
- **Inference (FSR):** `fsr` is **FSR 1** (EASU upscale + RCAS sharpen), not FSR 2. Evidence: a single colour input, the 4×uvec4 `Const0..3` EASU constants plus a `Sample` constant, the 64-thread workgroup, half-precision variants, and no depth, motion or reactive inputs.
- **Inference:** `minify` is AMD FidelityFX SPD (single-pass downsampler). It is used for mip chains such as bloom start/end mips, SSR history "l miptree" and Hi-Z helpers.
- **Inference:** `depth_of_field` is AMD FidelityFX DoF.
- **Inference:** `ocean_spectrum` parameters (JONSWAP `u_peak_omega` / `u_gamma`, `u_swell`, `u_spread_blend`, `u_short_waves_fade`, `u_depth`) describe an FFT ocean with 4 cascades (`water/cascade_0..3_scale`) and a TMA/JONSWAP-style directional spectrum.

### Gaps
- LUT and volume resolutions (transmittance, multi-scatter, sky-view, froxel grid) are runtime values (`u_scatter_resolution`), not visible in shaders or strings.
- The exact contents of `tex_mie_phase` (file or computed) were not found.
- `hdr`, `fsr`, `atmosphere`, `svt`, `wxr_mask` and `device_hang` do not appear as standalone string literals in the exe. Their define attribution (e.g. `PRECISION` for fsr) is positional and uncertain.

## 2. G-buffer layout

### Takeaway
The world G-buffer has **5 colour MRTs plus depth/stencil**. The exe names them `gbuf-albedo`, `gbuf-material`, `gbuf-normal`, `gbuf-emissive`, `gbuf-weather`, `gbuf-depth`; the deferred shader binds them as `gbuffer_tex`, `gbuffer_mat`, `gbuffer_nrm`, `gbuffer_lit`, plus `tex_weather_*`, plus `gbuffer_pos` (depth).

| MRT | name | storage (from in-place weather views) | content |
|---|---|---|---|
| 0 | gbuf-albedo | RGBA8 (sRGB-encoded) | albedo, premultiplied by coverage |
| 1 | gbuf-material | RGB10A2 (seen as R32UI) | 3 packed 10-bit material values |
| 2 | gbuf-normal | RG16F (seen as R32UI) | view-space normal, Lambert-azimuthal-equal-area encoded |
| 3 | gbuf-emissive | format unknown | emissive × exposure |
| 4 | gbuf-weather | format unknown | rain / puddle / snow masks |

There is **no motion-vector target**. MSAA variants exist for everything.

### Cited Findings
- Exe resource and pass names: `gbuf-albedo`, `gbuf-material`, `gbuf-normal`, `gbuf-emissive`, `gbuf-weather`, `gbuf-depth`, "Clear G-Buffer", "world g-buffer", "g-buffer collection", "Prepare g-buffer collector"; copy-style weather uses `gbuf-copy-albedo/material/normal`; source file `engine/graphics/OGL_gbuffer.cpp`; graph function `OGL_build_world_gbuffer_graph` returning `OGL_gbuffer_data` — [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `terrain`: 864 of its 4,236 fragment modules write **5 colour outputs** `data_out0..4` (432 also write `gl_SampleMask`). These 864 are the only terrain modules that read `tex_weather_texture`. The other 3,372 fragment modules write a single `data_out0` and read the lighting inputs (IBL, CSM, scatter volumes, clouds, tonemap) — the forward path — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/terrain.xsa), [SCRIPTS](file:///C:/Users/traye/AppData/Local/Temp/claude/G--Repositorys-Coalition-of-Freeware-Developers-Scenery-Editor-X/67f57d29-13ad-4b5f-ab75-468c7bb89349/scratchpad/xsv/out/groups_terrain.txt)
- What the terrain G-buffer writer outputs, per the MSL (`terrain_1`), with `a` = coverage × LOD fade throughout:
  - `data_out0` = (albedo.rgb·a, a);
  - `data_out1` = (a, (clamp(s, −1, 1)·½ + ½)·a, (½ + ½·m.x − ½·m.z)·a, a), where `m` comes from `u_material_basis` plus (0, `u_specular_gain`, 0) or from the material texture;
  - `data_out2` = (n.xy·sqrt(2/(1+n.z))·a, 0, a), with n the **eye-space** normal;
  - `data_out3` = emissive·`u_exposure_scale.y` (premultiplied or not depending on `NITE_MODE`);
  - `data_out4` = weather vector w with w.z stored as z·½+½. The source depends on `u_weather_mode`: modes 2/3 sample `tex_weather_texture`; mode 1 computes snow from `u_imm_snow_luma_coef` / `u_material_snow_luma_coef`; the default is (0, 0, −1).
  - — [MSL](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/msl/terrain.xsa)
- What `deferred_gbuf` reads (MSL `deferred_gbuf_1`):
  - `gbuffer_pos.x` = depth, turned into view z with `u_clip_info[eye]` and into a view position with `u_proj_coefficients[eye]`;
  - `gbuffer_tex.w == 0` means no geometry (output 0);
  - `gbuffer_nrm.xy` is decoded as n = (xy·sqrt(1 − |xy|²/4), 1 − |xy|²/2);
  - from `gbuffer_mat`: gloss = max(0, 2·G − 1), perceptual roughness = 1 − 0.96875·gloss, metalness = clamp(2·B − 1, 0, 1), and R multiplies the sun term as clamp(R + N·L·shadow);
  - the BRDF LUT is sampled at (N·V, perceptual roughness); specular IBL comes from `tex_cubemap_probes` at LOD = roughness·5 + `u_material_specular_bias`; diffuse IBL comes from the same cube array at **LOD 6**.
  - — [MSL](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/msl/deferred_gbuf.xsa)
- Storage formats, as seen by `weather_apply_compute` editing the G-buffer in place:
  - `img_gbuffer_albedo_*` = **rgba8**, decoded and re-encoded with the sRGB transfer function in-shader;
  - `img_gbuffer_material_*` = **r32ui**, with three 10-bit fields at bits 0–9, 10–19, 20–29 (each = 1023·value) → an RGB10A2 surface viewed as R32UI;
  - `img_gbuffer_normal_*` = **r32ui** holding a packed **half2** → an RG16F surface viewed as R32UI.
  - Each comes in `_2d`, `_2d_array` and `_2d_ms_array` forms.
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/weather_apply_compute.xsa), [MSL](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/msl/weather_apply_compute.xsa)
- CACAO reads normals through `g_NormalIn` = `texture2d_array<uint>` (or MS) and unpacks `.x` as half2. This matches the RG16F-as-R32UI normal — [MSL](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/msl/cacao.xsa)
- `deferred_gbuf` binds, in set 1: `gbuffer_lit` b0, `gbuffer_mat` b1, `gbuffer_nrm` b2, `gbuffer_pos` b3, `gbuffer_tex` b4, each as 2D, 2DArray or 2DArrayMS depending on permutation (`GBUF_FMT`) — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/deferred_gbuf.xsa)
- `ssr_deferred` binds `gbuffer_vel` as a **uint** texture and tests bit `& 4u`. `hdr` binds `u_fxaa_stencil_mask` (uint) with mask `u_fxaa_stencil` — [MSL](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/msl/ssr_deferred.xsa), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/hdr.xsa)
- MSAA: in 432 terrain modules the alpha-to-coverage mask is written in the shader (`gl_SampleMask`; exe strings `ALPHA_TO_COVERAGE`, "In-shader alpha coverage %s (vendor %s%s)", `--force_alpha_coverage`). `msaa_categorize` (spec constant `SAMPLE_COUNT`) reads `gbuffer_nrm` MS and **discards** pixels whose samples all share one normal; it has no colour write. 247 `deferred_gbuf` fragment modules use `SampleRateShading` / `gl_SampleID` — [MSL](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/msl/msaa_categorize.xsa), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/deferred_gbuf.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- The terrain specialization constants are `ALPHA_MODE`(id 0), `HAS_DECS`(1), `HAS_NORM_DECS`(2), `NITE_MODE`(3), `BLEND_MODE`(4) — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/terrain.xsa)
- The gbuffer debug-mode menu string is "none-roughness-light clipping-light volume-cloud shadow sample-cloud shadow-tile lights-tile lights color-tile index-spilled tiles" — [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- The device reports `native_reverse_z: 1` and `has_reverse_z: 1`. `z_minmax` carries both `u_clip_info_rev` and `u_clip_info_fwd`. FFX DoF is built with `FFX_DOF_OPTION_REVERSE_DEPTH` — [LOG](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Log.txt), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/z_minmax.xsa)

### Inferences
- **Inference — MRT order:** data_out0 = albedo, 1 = material, 2 = normal, 3 = emissive ("lit"), 4 = weather. The exe lists the names in exactly this order, and the encodings match what `deferred_gbuf` and `weather_apply` decode.
- **Inference — formats:**
  - albedo: probably R8G8B8A8_SRGB as a render target, aliased to UNORM for storage;
  - material: A2B10G10R10_UNORM;
  - normal: R16G16_SFLOAT;
  - emissive and weather: unknown. Emissive is pre-scaled by exposure, so RGBA16F or R11G11B10F are plausible.
  - depth: reverse-Z, likely D32_SFLOAT with stencil.
- **Inference — material channels:**
  - R = a per-material lighting factor. Terrain writes 1.0. It acts like an ambient/sun-visibility term.
  - G = a signed value packed as ½ + ½·s: positive = gloss/smoothness, negative = an unknown second property.
  - B = metalness (upper half) and a second "negative" property (lower half: clamp(1 − 2B)).
  - A2 = coverage.
- **Inference:** `gbuffer_vel` is not velocity. It looks like the stencil aspect read as uint, carrying per-pixel classification bits (water, cockpit, FXAA mask; exe strings `stencil/debug_key`, `stencil/debug_mask`, `volumetric/fog/stencil_mode`).
- **Inference:** all G-buffer outputs are premultiplied by coverage, so decals and `BLEND_MODE` materials can alpha-blend into the G-buffer. HAS_DECS / HAS_NORM_DECS add detail and normal decals.
- **Inference:** the G-buffer is natively **layered** (2D-array) for stereo. `v_eye` with `u_head_to_eye[2]` and `u_proj_coefficients[2]` select the eye. Mono uses the `_2d` permutations.

### Gaps
- The exact VkFormat of emissive and weather, and whether depth is D32 or D24S8, are not visible. The exe has only a generic VK_FORMAT name table.
- The meaning of the negative halves of material G and B is not determined.

## 3. Lights: tiled light lists, spill volumes, billboards

### Takeaway
Dynamic lights go through a compute **2.5D tiled light culling** pipeline:
1. `light_transform` transforms and culls the light list.
2. `light_tile` uses the Hi-Z min/max to bin lights into screen tiles with **2 depth slices per tile** and up to 512 lights per tile.
3. `light_spill_compact` compacts lights flagged as overflowing ("spilled") into interior and exterior lists with indexed-indirect draw commands.

The full-screen `deferred_gbuf` pass ("mix down lights") reads the tile lists. A separate `deferred_gbuf` light-volume variant draws the spilled lights ("Spilled tiled lights"). Legacy/billboard lights (`light`) and ground lights are forward overlays.

### Cited Findings
- `light_transform` (compute 64×1×1):
  - `g_in_light_list` → `g_out_light_list` (`light_list_data_t` = {`position`, `cos_theta`, `color`, `attenuation`, `direction`, `light_mask`});
  - `light_transform_data` = {modelview, projection, `u_light_time`, distance attenuation, start/count, `u_light_transform_night_level`, colour attenuation, min size, threads, `u_light_transform_z_test`, `u_light_transform_layers`, `u_light_transform_cull_frustum`, `u_light_transform_frustum_planes[6]`, `u_light_transform_light_mask`};
  - also reads `u_tex_z_minmax`.
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/light_transform.xsa)
- `light_tile` (compute 16×16, `TILE_MODE`, 6 modules):
  - reads `u_tex_z_minmax` and `g_light_list`;
  - writes `g_tile_data` (`light_tile_list_t` = {`tile_end_z[2]`, `tile_light_slice_range[2]`, `tile_lights[512]`}), `g_index_data` (`light_tile_index_t` = {`light_count`, `tile_lights[32768]`}), `g_spill_state` (uint[]) and debug data;
  - workgroup shared memory: `frustum_planes`, `tile_near`, `tile_far`, `tile_slice_depth_bits`, `tile_slice_end_z[3]`, `tile_slice_lights uint[512][2]`, `aabb_center/half[2]`;
  - `light_tile_data` = {`u_light_tile_count`, `u_light_tile_size`, `u_light_tile_index_count/size`, `u_light_tile_max_lights`, `..._max_lights_per_tile`, `u_light_tile_z_level`, `u_light_tile_max_distance`, `u_light_attenuation_factor`, `u_light_tile_bitmask`, `u_light_tile_inband_depth`, `u_force_spill`}.
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/light_tile.xsa)
- `light_spill_compact` (compute 64×1×1, subgroup ballot, `LAYOUT`): turns `g_light_list` + `g_spill_state` into `g_spill_interior` / `g_spill_exterior` index lists and `g_spill_indirect` (`indexed_indirect_command_t[2]`), with `u_spill_mask` — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/light_spill_compact.xsa)
- Exe labels around these passes: "light vis", "spill volumes", `--tile_lights`, "view lights", "light index", "light clusters", "spill flags", "spill interior compact", "spill exterior compact", "spill indirect", "clear transforms", "transform lights", "index lights", "tile lights", "compact spills", "Tile lights", "Spilled tiled lights", "mix down lights"; art controls `gbuf/use_tile_lights`, `gbuf/use_spilled_tile_lights`, `lights/do_spill_fog` — [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `deferred_gbuf` light inputs:
  - main path: `light_tile_data`, `g_tile_data`, `g_tile_index_data`, `g_spill_state`, `g_light_list`, plus `u_deferred_shading_data` (`u_light_nts_cutoff`, `u_light_atten_const`, `u_tile_light_use_tiles`, `u_tile_light_light_mask`, `u_spill_fog_samples_*`);
  - light-volume variant: VS with `g_spill_compact` (light indices) + `g_light_list`, using `BaseInstance` / `InstanceIndex`; varyings `pos_light_eye`, `rgb_light`, `direction`, `v_intensity_cd_radius_m`, `semi_width`.
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/deferred_gbuf.xsa)
- `light_vis` (`SOURCE_MODE`): a debug visualiser of tiles and lights — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/light_vis.xsa)
- `light` (`USE_NEW_FOG`, 4 VS / 1 FS):
  - VS reads `light_vertices` SSBO (`light_vertex` {`vertex`, `normal`, `color`, `texcoord0/1`}), `u_ogl_light_data` (`u_light_exp`, `u_light_scale`, `u_light_range`, `u_photo_k`, `u_photo_cell[3]`, `u_percent_night`, ...), cloud colour/depth, out-scatter and volumetric fog (fogging done per vertex);
  - FS samples `u_lights_tex0`;
  - related exe strings `OBJ_lights.cpp`, `lights.txt`, `BILLBOARD_HW`, `SPILL_GND`.
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/light.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `ground_lights`: textured quads with `a_vertex` / `a_texcoord0` and `u_tex`. The forward terrain path has `u_light_data` with 4 fixed lights (`u_light_position_linear_attenuation[4]`, `u_light_diffuse[4]`, `u_light_direction_spot_cos_cutoff[4]`) — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/ground_lights.xsa), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/terrain.xsa)
- Photometric units: art controls `lighting/E_sun_lx`, `lighting/E_moon_lx`, `photometric/light_storage_scale`; light varyings in candela (`v_intensity_cd_radius_m`) — [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/deferred_gbuf.xsa)

### Inferences
- **Inference:** this is tiled deferred shading with a 2-slice depth split per tile (a "2.5D" approach using `tile_slice_depth_bits`). Tile frusta come from `z_minmax` rg32f min/max.
- **Inference:** a light that does not fit a tile's 512-light budget (or is forced via `u_force_spill`) is marked in `spill_state`, compacted, and drawn as a light volume. Volumes are split into **interior** (camera inside the volume) and **exterior** lists, which need different cull/depth states. Two indirect commands are emitted.
- **Inference:** the deferred light loop runs in scene-referred photometric units (cd, lx), with the result scaled by `u_exposure_scale`.

### Gaps
- Tile size in pixels and slice count limits are runtime values (`u_light_tile_size`). 16×16 is likely because of the workgroup size, but that is unconfirmed.

## 4. Shadows and reflections

### Takeaway
**Shadows:**
- Scenery uses **cascaded shadow maps**: 4 cascades in a `texture2DArrayShadow` (`tex_smap0`), rendered by a graph labelled "CSM Shadows".
- Clouds cast a separate temporally filtered shadow map (`cloud_shadows`), sampled as up to **9** cloud-shadow cascades.
- `shadow_rect` is a UI drop shadow, not a scene shadow pass.

**Reflections:**
- `ssr_deferred`: hierarchical (Hi-Z) screen-space reflections over the G-buffer and water, with history reprojection.
- `ssr_mesh`: SSR for cockpit glass.
- IBL: an array of up to 8 cube probes, GGX-prefiltered by `cube_filter_*` and lit with a split-sum `brdf_lookup` (256×256 RG32F GGX).

### Cited Findings
- `u_shadow_data`:
  - `shadow_fade`, `shadow_s[4]`, `shadow_t[4]`, `shadow_r[4]`;
  - `u_cloud_eye_to_shadow_matrix`, `u_cloud_shadow_sun_cascades_count`, `u_cloud_eye_to_shadow_cascade_matrix[9]`, `u_cloud_wrl_to_shadow_cascade_matrix[9]`;
  - `u_cloud_ms_attenuation(_mix)`, `u_cloud_ambient_minimum`, `u_shadow_master`, `u_eye_right_to_left`, `tex_smap_offset`, `u_shadow_count`, `u_shadow_bias`, `u_debug_show_csm`.
  - `tex_smap0` is `sampled(texture2DArrayShadow)` in deferred_gbuf and terrain.
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/deferred_gbuf.xsa)
- CSM graph strings: `csm_shadow_maps`, `REN_collector`, `csm_state`, `csm_params`, "CSM collection", "Collector fill", "Collector finalize", "Collector sort", "Clear CSM", "CSM Shadows"; function `OGL_build_csm_graph`; art controls `shadow/csm/lambda`, `shadow/csm/far_limit_interior/exterior`, `shadow/csm/near_limit_*`, `shadow/overlap_ratio`, `shadow/want_soft`, `shadow/passes_per_frame`; defines `WANT_SHAD`, `SOFT_SHADOW_MODE` (deferred) and `USE_SHAD`, `WANT_SOFT_SHADOWS` (terrain) — [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- No dedicated shadow-depth shader family exists. The 12.06 log listed a `csm_resolve` family that is absent in this build — [LOG06](file:///G:/Repositorys/Coalition-of-Freeware-Developers/Scenery-Editor-X/XP12_Vulkan_Log.txt), [MAP](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/)
- `vegetation_cull` has a `u_is_shadow_pass` flag. All terrain VS output `gl_Layer`, and `u_transform_block_fat` holds 4 matrices per draw — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/vegetation_cull.xsa), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/terrain.xsa)
- `cloud_shadows` produces a 2D rgba16f map with history, jitter and reprojection; consumers sample a 2D array of cascades (see §1) — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/cloud_shadows.xsa)
- `ssr_deferred` (compute 16×16, `GBUF_TYPE`, `TRACE_FROM`, `REF_MODE`, `READ_MODIFY_WRITE`):
  - reads `gbuffer_mat`, `gbuffer_nrm`, `gbuffer_pos`, `gbuffer_vel`(uint), `tex_hiZ`, `tex_ssr` (history), `tex_brdf`, `s_water_nrm`, `water_pos`;
  - writes `i_output` **rgba16f** 2DArray;
  - `u_ssr_data` = {`u_proj[2]`, `u_reproj[2]`, `u_local_reproj[2]`, `u_modelview(_inverse)`, `u_max_iterations`, `u_hiz_width/height`, `u_viewport_to_buffer_ratio`, `u_half_ssr_history_res`, `u_ssr_min_gloss`, `u_ssr_lod_bias`, `u_ssr_use_prefiltered`, `u_ssr_sample_count`};
  - labels "SSR results", "pre-ssr", "%s l miptree", "Seed SSR history".
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/ssr_deferred.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `ssr_mesh` (raster, `SSR_TYPE`): instanced objects with `tex_material_map`, `tex_hiZ`, `tex_ssr`, `tex_brdf`, `u_ssr_mesh_material_data.u_mat_params`. Related exe strings: "Seed the SSR history buffer with a given image and set it as the glass decorator", "SSR Mesh", `OGL_build_cockpit_ssr_graph`, `ssr/want_cockpit` — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/ssr_mesh.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- IBL:
  - `ibl_probes_data` = {`u_ibl_probes_count_diffuse`, `u_ibl_probes_count_specular`, `u_probe_data[8]`, `u_probe_layer[8]`, `u_eye_to_probe[8]`}, with `tex_cubemap_probes` = `textureCubeArray`;
  - `cube_filter_compute` (16×16; `USE_ALPHA`, `USE_COS`) reads `u_src_tex` (cube) and writes `i_mip` = 6 storage cube images (rgba16f). `u_cube_blur_data` = {`u_samples`, `u_alpha`, `u_cube_dim`, `u_mip`, `u_filter_specular`, `u_filter_mip_limit`, `u_filter_log_omega_s`, `u_min_cos_nts`, `u_min_ggx_nts`, `u_N/u_N_ds/u_N_dt[6]`, `u_filter_sample_uv[128]`};
  - `cube_filter_raster` is the raster fallback;
  - exe strings: "blur cube map", `cube_blur_rp`, `brdf_lookup`, `env_brdf`, `ibl/filter_specular`, `ibl/ggx/floors_nts`, `ibl/cos/floors_nts`, `REN_build_update_environment_probes_graph`, "External Cubemap", "Internal Cubemap", `cubemap/interior_acf_cloud_shadow`.
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/cube_filter_compute.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- **brdf_lookup** (524,288 bytes, no header):
  - Only the RG32F 256×256 decoding gives clean values in [0, 1] with no NaN or inf. RGBA16F, RG16F, RG8 and RGBA16 decodings give garbage, NaNs or out-of-range values.
  - At x = 0..255 (N·V = x/255) and y = 0 (roughness 0), A + B = 1, with B = (1 − N·V)^5 (e.g. x = 128 → (0.9688, 0.0312)).
  - Against a 1,024-sample importance-sampled split-sum integral, the file matches **GGX with α = roughness² and the height-correlated Smith visibility**, Schlick Fresnel, and N·V clamped near 1e-3: (x=128, y=128) file (0.8332, 0.0223) vs model (0.8327, 0.0219); (x=0, y=255) file (0.9387, 0.0459) vs (0.9366, 0.0451); (x=255, y=128) file (0.9161, 0) vs (0.9149, 0). The Schlick-G (k = α/2) and α = roughness variants do not match.
  - — [BRDF](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/brdf_lookup), `brdf.py` / `brdf_check.py` in [SCRIPTS](file:///C:/Users/traye/AppData/Local/Temp/claude/G--Repositorys-Coalition-of-Freeware-Developers-Scenery-Editor-X/67f57d29-13ad-4b5f-ab75-468c7bb89349/scratchpad/xsv/)
- `tex_brdf` is sampled at (max(N·V, 0), perceptual roughness) in deferred_gbuf. The same `tex_brdf` binding appears in terrain, ssr_*, fake_terrain — [MSL](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/msl/deferred_gbuf.xsa)

### Inferences
- **Inference:** the 4 CSM cascades are rendered with the terrain/object pipelines in depth-only mode, probably layered (one instanced pass writing `gl_Layer` per cascade). That fits the 4-matrix transform block and the absence of a shadow-specific family.
- **Inference:** "soft" shadows (`SOFT_SHADOW_MODE`) are PCF-style filtering in `deferred_gbuf`. 12.06's screen-space `csm_resolve` was folded into deferred lighting.
- **Inference:** probe cubemaps hold 7 mips: mip 0 is the source; mips 1–5 hold GGX-prefiltered specular for roughness·5; mip 6 holds cosine-filtered irradiance (`USE_COS`). Up to 8 probes are blended per pixel, with separate diffuse/specular counts (exterior and aircraft-interior probes).
- **Inference:** SSR is a Hi-Z ray march into the previous frame's lit HDR (the "SSR history" mip tree), reprojected with `u_reproj` / `u_local_reproj`. Water is a second trace source (`TRACE_FROM`). The result feeds `deferred_gbuf.tex_ssr` and `ocean_shading.tex_ssr`.

### Gaps
- CSM resolution, cascade split values and depth format are not in shaders. Split logic is configurable via `shadow/csm/lambda` (practical split scheme), but the values come from art controls, which another researcher covers.

## 5. Weather: rain_*, weather_apply_*, wxr_mask, rain_ice_accumulation

### Takeaway
There are two unrelated weather systems:
1. **World-surface weather** (`weather_apply_*`) runs on the G-buffer after it is written. It uses the `gbuf-weather` MRT that terrain writes (rain/wetness, puddles, snow), a world-projected snow coverage texture, and tiled snow/ice albedo/normal/decal textures. It edits albedo/material/normal in place (compute on storage views) or copy-style (raster).
2. The **`rain_*` family** is the windshield/canopy simulation: droplets, wipers, glass forces, heaters, glass ice accumulation with CPU readback. It does not touch the terrain G-buffer.

`wxr_mask` is a clip-plane mask used by the weather-radar display.

### Cited Findings
- `weather_apply_compute` (8×8, 16 modules):
  - edits `img_gbuffer_albedo/material/normal` (2D, array or MS-array; rgba8 / r32ui / r32ui) in place;
  - reads `tex_gbuffer_z_*`, `tex_weather_*` (the 5th MRT), `tex_droplets`, `tex_rain_area_noise`, `tex_snow_alb`, `tex_snow_nml`, `tex_snow_dcl`, `tex_snow_coverage`, `tex_ice_alb`, `tex_ice_nml`, `tex_ice_dcl`.
- `weather_apply_raster` (`DEBUG_MODE`) samples the same inputs plus `tex_gbuffer_albedo/material/normal_*` and writes 3 MRTs `data_out0..2` (copy style).
- Exe: textures `Resources/bitmaps/world/weather/noise.png`, `snow_ALB.png`, `snow_NML.png`, `snow_DCL.png`, `ice_ALB.png`, `ice_NML.png`, `ice_DCL.png`; labels "weather apply", "weather apply raster"; functions `build_apply_weather_graph_in_place`, `build_apply_weather_graph_copy_style`, `weather_apply_init`.
- — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/weather_apply_compute.xsa), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/weather_apply_raster.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `u_weather_apply_data`:
  - `u_weather` (vec4), `u_drop_scale`, `u_rain_area`, `u_rain_data`;
  - `u_rain_darkening_range/strength`, `u_rain_roughness_multiplier`, `u_rain_roughness_range`, `u_rain_darken_albedo_rat`, `u_rain_darken_gloss_rat`;
  - `u_snow_area`, `u_snow_slope`, `u_snow_scale_alb/nml/dcl`, `u_snow_decal1/2_mod_k/rgba`, `u_snow_proj_s/t`, `u_snow_level_scale`;
  - `u_ice_area`, `u_ice_scale_alb/nml/dcl`, `u_ice_decal1/2_mod_*`;
  - `u_up_nml`, `u_samples`, debug coefficients.
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/weather_apply_compute.xsa)
- What the MSL (`weather_apply_compute_0`) does with the weather MRT `w`:
  - w.x drives rain wetness: a smoothstep around `u_weather.x`, albedo darkening, and gloss/roughness changes; it also drives ice via `u_weather.y`;
  - w.y·k drives puddles (smoothstep around `u_weather.w`): normals blend toward up plus `tex_droplets` ripples, and gloss rises;
  - w.z (decoded 2z − 1) drives snow, together with `tex_snow_coverage` (world-projected via `u_snow_proj_s/t`, used when `u_weather.z ≤ −1`), a slope test against `u_up_nml` (`u_snow_slope`), and noise;
  - the pass then re-encodes albedo (sRGB), normal (half2 equal-area) and material (10:10:10).
  - — [MSL](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/msl/weather_apply_compute.xsa)
- How terrain feeds it: G-buffer modules write `data_out4` from `tex_weather_texture` (`u_weather_mode` 2/3) or from snow-luma coefficients (`u_weather_mode` 1: `u_material_snow_luma_coef`, `u_material_snow_luma_mix`, `u_imm_snow_luma_coef`); art controls `snow/luma_r/g/b/a`; exe datarefs `wxr/rain_now`, `wxr/puddles_now`, `wxr/snow_now`, `wxr/ice_now`, `twxr/rain_darkening_*` — [MSL](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/msl/terrain.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `ripple` (16×16): `ripple_seed_tex` → `output_tex` **rgba8**, `u_ripple_data` {`u_splash_time`, `u_weight`, `u_tex_scale`}; labels "ripple", "circles", "droplets" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/ripple.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- Windshield rain:
  - `rain_forces` (instanced aircraft mesh → `force_data`) and `rain_droplets_compute` (64×1×1, `LAST_ITERATION`): `droplets`, `droplet_grid`, `droplet_trail`, `droplet_draw`, `indirect_draw`, plus `tex_glass_forces` and `tex_glass_surface`;
  - `rain_droplets_wipers` uses `tex_glass_wiper`; `u_rain_info` = {`u_num_wipers`, `u_num_heaters`, wiper start/end/edge, friction, drag, streak threshold, `u_precip_intensity`, `u_deice_enabled`, `u_deice_rate`, `u_icing_edge`, `u_ice_*`};
  - `rain_draw` draws droplets and trails; `rain_normals` reads `u_depth_tex` + `u_ice_tex`; `rain_surface` draws the glass with `screenshot_tex` (scene behind), `u_depth_tex`, `u_normal_tex`, `u_ice_tex`, cloud colour/depth and volumetric fog;
  - `rain_ice_accumulation` (8×8, `HAS_THERMAL_SOURCES`): `u_force_data`, `u_glass_surface`, `u_thermal_tex` → `u_ice_accumulation` **r32f**, plus SSBO `ice_readback` {`icing_amount`, `count`};
  - labels "rain blit to HDR surface", "Ice Accumulation", "Readback ice", "Wipers", "rain depth (hi-res)", "rain normals".
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/rain_ice_accumulation.xsa), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/rain_surface.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `wxr_mask`: VS `a_vertex`, `u_wxr_mask_data.u_clip_plane`, FS writes `data_out0`. `cloud_map` holds NEXRAD/radar parameters (`u_radar_range_nm`, `u_nexrad_*`, `u_microburst`, `u_plane_targets`) — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/wxr_mask.xsa), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/cloud_map.xsa)
- Ground snow from the cloud/weather engine: label `ground_snow_coverage` next to `cloud_weather_*` in `wxr_renderer.cpp` — [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)

### Inferences
- **Inference — weather MRT layout:** R = rain-wetness receptivity, G = puddle/standing-water receptivity, B = snow receptivity (signed, stored ½ + ½·s). Terrain authors control these per material (a weather texture or a snow-from-luma rule). Global intensities come from `u_weather` = (rain, ice, snow level, puddles).
- **Inference:** `tex_snow_coverage` is the world-space `ground_snow_coverage` map produced by the weather engine. `tex_droplets` is likely the `ripple` output. Both are inferred from naming and data types.
- **Inference:** in-place compute is used when the device supports storage writes to these formats, including MSAA. The log reports `storage_image_multisample: 1` and `msaa_storage_image_avail: 1`. Otherwise the copy-style raster path is used.

### Gaps
- The exact semantics of the w.y scale factor and the full ice logic are only partly decoded.

## 6. Terrain sampler and texture names (seasons, snow, wetness, detail/normal, "svt")

### Takeaway
`terrain_mapping.xsv` carries 49 names, of which 26 are textures/samplers. Season, snow and weather support appear as:
- **season:** `tex_seasonal_texture`, `tex_comp1_seasonal_texture`, `tex_comp2_seasonal_texture`;
- **weather/snow:** `tex_weather_texture`, the snow-luma fields in `u_material_data` and `u_immediate_data`;
- **detail and normal decals:** `tex_deca_*`, `tex_dec2_*`, `tex_norm_deca_*`, `tex_norm_dec2_*`.

Composites and borders are `tex_comp`, `tex_comp2`, `tex_bord`. There are no virtual-texturing names. `svt` is synthetic vision.

### Cited Findings
- Terrain texture and sampler bindings (set 1 unless noted), with how many of the 4,236 fragment modules use each:

| binding | name | modules |
|---|---|---|
| b0 | `tex_bord` | 2,496 |
| b1 | `tex_brdf` | 3,292 |
| b2 | `tex_bump` | 4,156 |
| b3 | `tex_comp` | 2,256 |
| b4 | `tex_comp2` | 1,122 |
| b5 | `tex_material` | 4,156 |
| b6 | `tex_nite` | 4,156 |
| b7 | `tex_noise` | 1,158 |
| b8 | `tex_base_sampler` (separate sampler) | 4,060 |
| b9 | `tex_deca_sampler` (separate sampler) | 4,156 |
| b10 | `tex_base_texture` | 4,028 |
| b11 | `tex_comp1_seasonal_texture` | 384 |
| b12 | `tex_comp2_seasonal_texture` | 384 |
| b13 | `tex_dec2_texture` | 3,940 |
| b14 | `tex_deca_texture` | 3,940 |
| b15 | `tex_norm_dec2_texture` | 4,156 |
| b16 | `tex_norm_deca_texture` | 4,156 |
| b17 | `tex_seasonal_texture` | 1,250 |
| b18 | `tex_weather_texture` | 864, all G-buffer modules |
| b19 | `u_material_data` (UBO) | — |
| set 2 | pass-level resources | see §7 |

  — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/terrain.xsa), [MAP](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/terrain_mapping.xsv)
- `u_material_data` (34 members) groups as follows:
  - texture parameters: `tex_params`, `tex_params2`, `tex_params3`, `tex_params4`, `u_texcoord_offset`, `u_texcoord_scale`;
  - shading: `u_emission`, `u_specular_gain`, `u_bump_scale`, `u_alpha_width`, `u_alpha_cut`, `u_blend_cut`;
  - detail decals: `decal1`, `decal2` (`decal_params` = {`rgb_key`, `a_key`, `km_key`, `scale`}), with switches `u_decal1`, `u_decal2`;
  - normal decals: `normal_decal1/2_rgba_key`, `_km_key`, `_scale`, `_gloss`, with switches `u_normal_decal1`, `u_normal_decal2`;
  - snow/weather: `u_material_snow_luma_coef`, `u_material_snow_luma_mix`, `u_weather_mode`;
  - normal/material model: `u_normal_mode`, `u_material_factor`, `u_material_basis`;
  - projection planes: `u_base_plane_s`, `u_base_plane_t`.
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/terrain.xsa)
- `u_misc_data` holds `u_eye_to_wrl`, `billboard`, `num_lites`, `u_planet_clip`, vegetation bend/branch/displacement/speed/up, `u_prop_cross_fade`, `u_terrain_filter_normals`, `u_terrain_filter_kappa`, `u_sea_floor_coef`, `u_lod_bias(_exp)`, the cloud/volumetric-fog projection coefficients (`u_terrain_cloud_proj_coeff_inv`, `u_terrain_volumetric_fog_proj_coeff_inv`, `u_terrain_volumetric_fog_near_far`), and `u_msaa_samples` — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/terrain.xsa)
- Terrain varyings: `v_seasonal`, `v_normal_object_seasonal`, `v_water_height`, `v_texcoord_water`, `v_texcoord_comp(_border)`, `v_texcoord_border`, `v_texcoord_shadow`, `v_position_eye_shadow`, `v_color_night_level`, `v_position_eye_crazy_alpha_fade`, `v_normal_eye_base_s`, `v_position_object_base_t`. Vegetation points carry `season_rat_scale` — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/terrain.xsa)
- `.ter` directive strings next to the terrain loader: `SEASONAL_HILL`, `SEASONAL_CLIFF`, "composite seasonal mask", `SEASONAL_FAR`, `WATER_COLOR_MASK`, `NORMAL_TEX`, `NORMAL_TEX_NOWRAP`, `COMPOSITE_BORDERS`, `COMPOSITE_DETAIL`, `DETAIL`, `AUTO_VARY`, `AUTO_SLOPE*`, `DYNAMIC_TERRAIN`, `PROJECT_ANGLE`, `PROJECT_OFFSET`, `BUMPY` — [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- `svt` binds `u_tex_elevation`, `u_tex_noise`, `u_tex_sectional`, `u_tex_terrain`, `u_tex_watermask` with synthetic-vision UBOs; exe strings `svt shader`, `svt/noise_scale`, `svt/fovpx`, "SVT tile ... requested". The capability table reports `sparse: 0` and "sparse bindings: 0" — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/svt.xsa), [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe), [LOG](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Log.txt)

### Inferences
- **Inference:** the composite terrain path uses `tex_comp` / `tex_comp2` (composite blend masks, plus seasonal masks `tex_comp*_seasonal`) and `tex_bord` (border blending). `tex_noise` adds variation. Two detail-decal slots (`deca`, `dec2`) and two normal-decal slots are keyed by RGBA/KM keys.
- **Inference:** `tex_nite` is the night/LIT emissive map that feeds G-buffer emissive. `tex_material` is the material map that feeds material G/B (gloss/metal).

### Gaps
- The terrain shader's permutation-key encoding (how the 12 defines and their value ranges multiply to 829,440) is not decoded from the mapping tables.

## 7. Inferred frame graph (ordered, compute vs raster)

### Takeaway
The exe's render-graph labels (from `engine/REN/2_plot_frame_graph.cpp` and the `OGL_build_*_graph` functions) and the producer/consumer bindings fit one consistent order:
1. atmosphere LUTs and weather/cloud data;
2. CSM and VRS;
3. G-buffer;
4. depth helpers (Hi-Z, resolve, MSAA classification);
5. weather apply;
6. CACAO, light tiling, SSR, clouds, fog;
7. deferred lighting, then spilled lights;
8. water, cloud/fog composite, forward overlays;
9. fix-HDR, histogram, exposure fusion, bloom, DoF, tonemap, FSR 1;
10. VR depth, UI, present.

**The whole order is inferred.** String order is not proof of execution order.

### Cited Findings
- Contiguous label block from `2_plot_frame_graph.cpp`, in order: "legacy UI", "Lights", "Sky Ambient", "Sky-view image", "Skydome compute in/out", "volumetric clouds", "volumetric march", "volumetric fog parameters", "Volumetric Fog", `csm_shadow_maps`, "CSM collection", "Collector fill/finalize/sort", "Clear CSM", "CSM Shadows", "Cockpit prefill", VRS attachment, "g-buffer collection", "Prepare g-buffer collector", "gbuf-depth", "Clear G-Buffer", "world g-buffer", "compute clouds", "Clear Clouds", "render clouds", "clouds-integrate", "clouds-proxy", "Clear Cloud Depth", "depth clouds", "categorize clouds", "cloud-color", "cloud-depth", "categorize msaa", "Spilled tiled lights", "Clear HDR", "mix down lights", "mix down water", "Onscreen Clouds", "volumetric_apply", "volumetric fog apply", "HDR clouds", "HDR blended overlays", "Fix HDR", "VR blended overlays", "cockpit gstack + histo", "SSR Mesh", "Post-FX", "vr_depth_copy", "VR depth surface", "Capture VR depth", "ldr-depth", "Clear LDR", "LDR-Temp to LDR", "warp", "blit", "Offscreen", "Offscreen-Depth", "make back texes async", "Prep per frame" — [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- Graph builders present (RTTI): `REN_build_render_graph`, `REN_build_render_graph_3d_world_subgraph`, `REN_build_update_environment_probes_graph`, `REN_build_simple_post_graph`, `REN_build_offscreen_render_graph_hdr/_tone_map`, `WORLD_build_prep_per_frame_graph`, `OGL_build_scattering_graph`, `OGL_build_sky_ambient_graph`, `OGL_build_sky_view_graph`, `OGL_build_csm_graph`, `OGL_build_world_gbuffer_graph`, `build_apply_weather_graph_in_place/copy_style`, `build_z_mipchain_graph`, `OGL_cacao_build_cacao_graph`, `OGL_build_light_tile_graph`, `OGL_build_ssr_compute_graph`, `OGL_build_compute_cloud_graph`, `OGL_build_render_cloud_graph`, `OGL_build_cloud_upscale_graph`, `OGL_build_categorize_clouds(_msaa)_graph`, `OGL_build_volumetric_fog_graph`, `OGL_build_light_mixdown_graph`, `OGL_build_water_mixdown_graph`, `OGL_build_blended_overlays_graph`, `OGL_build_vr_blended_overlays`, `OGL_build_cockpit_ssr_graph`, `OGL_build_gstack_graph`, `OGL_build_postfx_graph`, `OGL_build_exposure_fusion_graph`, `OGL_build_bloom_graph`, `OGL_build_apply_bloom_graph`, `OGL_depth_of_field_build_graph`, `OGL_build_tonemap_graph`, `OGL_fsr_build_upscale_graph`, `OGL_fsr_build_sharpen_graph`, `OGL_minify_graph`, `OGL_resolve_graph`, `OGL_build_vr_depth_capture_graph`, `OGL_build_offscreen_graph`, `build_panel_graph`, `build_garmin_stuff_graph`, `build_2d_window_dag`, `build_3d_window_dag`, `build_all_monitors_dag`, `build_vr_mirror_window_dag`; plus `OGL_graph_builder_async_task_node`, `gfx_vk_aliasing_buffer_static` — [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- The graph lambdas take `OGL_gbuffer_data`, `OGL_depth_data`, `OGL_fog_data`, `OGL_offscreen_cloud_data`, `OGL_light_mixdown_inputs`, `OGL_blended_overlays_inputs`, `REN_water_mesh_data` — [EXE](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/X-Plane.exe)
- The log reports an "Async Compute" queue and the device "bucketed as: Vulkan with async compute" — [LOG](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Log.txt)

### Inferences
**Inferred frame graph** (C = compute, R = raster; arrows = data the next pass binds; each line is inferred unless a cited binding says otherwise):
0. **Per-frame prep / probes**
   - CPU "Prep per frame".
   - Environment probe update when due (`REN_build_update_environment_probes_graph`): R render the cube → C/R `cube_filter_*` (GGX mips + cosine irradiance) → `tex_cubemap_probes` cube array + `ibl_probes_data`.
1. **Weather and cloud data** (C, async-capable): `cloud_update` → 3D weather coverage/properties → `cloud_minify` (SDF/jump-flood) → `cloud_precipitation` (2D) → `cloud_shadows` (temporal 2D) → `u_tex_cloud_shadow_cascades`.
2. **Atmosphere LUTs** (C): `scatter_compute_t` (transmittance) → `scatter_compute_multi` (multi-scatter) → `scatter_compute_skydome` (sky-view, skydome, sky-ambient LUTs; cloud-shadowed variant) → `u_tex_skydome`, `u_tex_sky_ambient`.
3. **Aerial-perspective volumes** (C): `scatter_compute_scattering` → `u_tex_in_scatter` / `u_tex_out_scatter` (3D r11g11b10f). The `USE_ZBUF` variant needs depth, so it either runs after step 7 or uses the previous frame's depth (unknown).
4. **Vegetation culling** (C): `vegetation_data` → `vegetation_cull`, per view and per shadow pass → indirect commands and culled points for terrain VS.
5. **CSM** (R): collect, sort, clear → depth-only terrain/object/vegetation draws into the `tex_smap0` 2D array (4 layers).
6. **VRS** (C): `shading_rate` → r8ui shading-rate attachment (VR foveation) for the 3D passes. Then R "Cockpit prefill" (cockpit depth prefill).
7. **World G-buffer** (R): "Clear G-Buffer" → terrain/objects/roads/facades/vegetation/planet (5 MRT + depth/stencil; MSAA optional with in-shader coverage). In parallel, water `ocean_meta_data` → water nrm/meta/colour MRTs. Ocean FFT chain (C, possibly async): `ocean_spectrum` → `compute_fft` → `ocean_bake_textures` / `ocean_variance` → `ocean_mesh_*`.
8. **Depth helpers**:
   - R `depth_resolve` (when MSAA);
   - C `z_minmax` → rg32f Hi-Z mips ("z-mip"), used by light tiling, SSR and volumetrics;
   - R `msaa_categorize` → marks complex MSAA pixels (stencil);
   - C `minify` (SPD) where mips are needed.
9. **Weather apply**: C `weather_apply_compute` in place (or R `weather_apply_raster` copy-style) reads gbuf-weather + depth + snow coverage + ripple droplets and modifies albedo/material/normal.
10. **SSAO**: C `cacao` (prepare depths/normals → importance → generate → blur → upsample/reinterleave) → `u_tex_ssao` (r16f array).
11. **Light tiling**: C `light_transform` → `light_tile` → `light_spill_compact` → tile lists + spilled-light indirect draws.
12. **SSR**: C `ssr_deferred` (Hi-Z + G-buffer + previous HDR history + water) → `tex_ssr` (rgba16f array).
13. **Clouds**: C `cloud_render_compute` or R `cloud_render_raster` (reduced resolution) → C `cloud_upscale` (temporal, proxy) → R "depth clouds" → R `cloud_categorize` → cloud colour/depth.
14. **Volumetric fog**: C `volumetric_fog` density → accumulate (temporal) → `u_tex_volumetric_fog` (3D rgba16f).
15. **Lighting** (R full-screen): "Clear HDR" (RGBA16F layered, MSAA optional) → `deferred_gbuf` main ("mix down lights": tile lights + IBL + BRDF LUT + CSM + cloud shadows + SSAO + SSR + aerial perspective + emissive) → `deferred_gbuf` light volumes ("Spilled tiled lights", additive).
16. **Water**: R `ocean_shading` ("mix down water"; water MRTs + skydome cube + SSR + variance + scatter).
17. **Sky and composite**: R sky via `scatter_render_atmosphere` / `dome` / `astronomical` (placement uncertain) → R `volumetric_apply` ("Onscreen Clouds", "volumetric fog apply", "HDR clouds").
18. **Forward overlays** (R, into HDR): forward terrain/objects (single-output terrain modules), `particle`, `light` billboards, `ground_lights`, `rain`, `line3d`, legacy → "HDR blended overlays" / "VR blended overlays".
19. **Fix HDR** (C): `fix_hdr` clamps inf.
20. **Cockpit extras**: C `gstack` + `lumi_histo` ("cockpit gstack + histo"); R `ssr_mesh` (cockpit glass); windshield rain (`rain_*` C/R) → "rain blit to HDR surface".
21. **Post-FX**:
    - C `lumi_histo` → CPU readback (auto exposure);
    - C `exposure_fusion` → `tex_lumi_guide`;
    - C `bloom` (rip → blur H/V; SPD mips);
    - C `depth_of_field` (optional);
    - R `hdr` tonemap (bloom apply + exposure-fusion guide + tonemap + CDL + FXAA stencil + false colour) → "LDR-temp";
    - C `fsr` EASU → RCAS (optional) → "LDR-Temp to LDR".
    - Bloom versus DoF order is unknown.
22. **VR / output**: R `vr_depth_copy` ("Capture VR depth") → warp/blit to swapchain; R `resolve` where MSAA-to-single-sample is needed.
23. **2D / avionics / UI** (separate DAGs): panels/gauges (`svt`, `sectional2`, `profile_view`, `cloud_map`, `wxr_mask`, `airport_raster`, `acf_map_icon`), UI (`background_blur`, `font`, `blit`, `shadow_rect`, `tchotchke`, `mixed_reality`).

### Gaps
- Execution order, queue assignment (graphics vs async compute) and barriers can only be confirmed with a capture. The RenderDoc captures were deliberately not opened.
- Whether `scatter_compute_scattering` with `USE_ZBUF` and `volumetric_fog` use current-frame or previous-frame depth is unknown.

## 8. What the logs say about GPU features and settings, and how 12.06 differs

### Takeaway
On the 12.4.3 log machine (RTX 3090, Vulkan 1.4.341):
- **in use:** push descriptors, timeline semaphores, VRS, multisample storage images, buffer device address, async compute;
- **not in use:** descriptor buffers, sparse binding, shader float16/int16.
- MSAA ("HDR anti-aliasing") is on in the desktop profile and off in the VR profile. FXAA is on and FSR is off.

The 12.06b1 log (RTX 3060, Vulkan 1.3.236) shows 81 shader families against 97 now. 12.06 still had `deferred_ssao`, `ssao_blur`, `csm_resolve`, `lit`, `puff`, `cloud_opacity`, `stars`, `rain_temperature` and `thotchke`. It had no tiled lights, exposure fusion, bloom, FFX DoF, volumetric fog, VRS or Bruneton `atmosphere`.

### Cited Findings
- 12.4.3 capability table:
  - `buffer_device_address 1`, `descriptor_buffers 0`, `push_descriptors 1`, `timeline_semaphores 1`, `variable_rate_shading 1`, `storage_image_multisample 1`, `sparse 0`, `shader_float16 0`, `shader_int16 0`, `maintenance4 1`, `vulkan_13 1`;
  - `native_reverse_z 1`, `msaa shader write 1`, `msaa_storage_image_avail 1`, `alpha_to_one_avail 1`, `max samples: 3`, `tess_avail 0`, `geom_avail 0`;
  - queues: Generic/Presentation, Transfer, Async Compute; surface format picked: `VK_FORMAT_B8G8R8A8_SRGB`.
  - — [LOG](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Log.txt)
- 12.4.3 settings in profile 'default': Cloud quality 2, FSR quality level 0, SSAO quality level 2, 3-d shadows 2, HDR anti-aliasing 1, Enable FXAA 1, Texture Resolution 4, Anisotropic 16. In profile 'VR': HDR anti-aliasing 0, others the same — [LOG](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Log.txt)
- The device extension list includes `VK_KHR_fragment_shading_rate`, `VK_KHR_dynamic_rendering(_local_read)`, `VK_KHR_synchronization2`, `VK_KHR_push_descriptor`, `VK_EXT_descriptor_buffer`, `VK_EXT_mesh_shader`, `VK_KHR_ray_query`, `VK_KHR_depth_stencil_resolve`. This is the device's supported list, not necessarily what X-Plane enables — [LOG](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Log.txt)
- 12.06b1: Vulkan 1.3.236, driver 531.79, RTX 3060, "Vulkan with async compute", `msaa shader write 1`, `shader int16 1`, reverse-Z available. That log lists each "Loaded mapping Resources/shaders/bin/<family>_mapping.xsv with hash ..." and "Loaded archive .../spv/<family>.xsa"; the 12.4.3 log has no such lines. Settings: SSAO 0, shadows 0, HDR AA 1, FSR 0 — [LOG06](file:///G:/Repositorys/Coalition-of-Freeware-Developers/Scenery-Editor-X/XP12_Vulkan_Log.txt)
- Family-set difference:
  - only in 12.06: `cloud_opacity`, `csm_resolve`, `deferred_ssao`, `lit`, `puff`, `rain_temperature`, `ssao_blur`, `stars`, `thotchke`;
  - only in 12.4.4-b4: `acf_map_icon`, `airport_raster`, `astronomical`, `atmosphere`, `bloom`, `cloud_precipitation`, `depth_of_field`, `device_hang`, `exposure_fusion`, `fix_hdr`, `light_spill_compact`, `light_tile`, `light_transform`, `light_vis`, `mixed_reality`, `profile_view`, `rain_droplets_wipers`, `rain_ice_accumulation`, `shading_rate`, `shading_rate_vis`, `svt`, `tchotchke`, `volumetric_fog`, `vr_depth_copy`, `wxr_mask`.
  - — [LOG06](file:///G:/Repositorys/Coalition-of-Freeware-Developers/Scenery-Editor-X/XP12_Vulkan_Log.txt), [MAP](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/)

### Inferences
- **Inference:** between 12.06 and 12.4.x, X-Plane:
  - replaced its custom SSAO (`deferred_ssao` + `ssao_blur`) with CACAO-only;
  - folded the screen-space CSM resolve into deferred lighting;
  - replaced per-light deferred `lit` passes with tiled lighting plus spill volumes;
  - added local tone mapping, compute bloom, FFX DoF, froxel volumetric fog, VRS and VR depth export;
  - renamed `stars` → `astronomical` and `rain_temperature` → `rain_ice_accumulation`.
- **Inference:** `max samples: 3` is probably an enum index (e.g. 8×) rather than a raw count.

### Gaps
- The 12.06 shader binaries are not on disk, so 12.06 binding layouts cannot be compared.

## 9. How the binding maps were decoded, and the raw per-file names (appendix)

### Takeaway
`*_mapping.xsv` (magic `XSV6`) is a binding/permutation index. It holds:
- a sorted string table of every attribute and resource name;
- the permutation-space size;
- the pipeline kind;
- per-descriptor-set type lists (exactly matching SPIR-V set order);
- per-permutation records that point into the deduplicated module list of `spv/` and `msl/` `<family>.xsa`, and carry the compute workgroup size.

### Cited Findings
- Header layout as decoded:
  - `"XSV6"`, u16 name count, then that many (u8 length + ASCII) names, sorted alphabetically;
  - u32 permutation count (terrain 829,440; deferred_gbuf 20,480; bloom 4), u32 1;
  - u8 kind (**3** = raster VS+FS, **4** = compute), u8-length entry name `main`;
  - u8 group count, then per group: (u8 tag, u8 n, n × (u8 descriptor type, u8 array count)).
  - Descriptor type codes: **1** combined image-sampler, **2** sampler, **3** sampled image, **4** storage image, **5** uniform buffer, **6** storage buffer.
  - — [MAP](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/), `mapping.py` in [SCRIPTS](file:///C:/Users/traye/AppData/Local/Temp/claude/G--Repositorys-Coalition-of-Freeware-Developers-Scenery-Editor-X/67f57d29-13ad-4b5f-ab75-468c7bb89349/scratchpad/xsv/)
- Groups are descriptor sets in order. Terrain's groups decode as follows, matching the SPIR-V sets exactly:
  - tag1 = {ssbo, ubo×3} → set 0: `u_transform_block_fat` b0, `u_immediate_data` b1, `u_environment_data` b2, `instances` b3;
  - tag5 = {smp×2, tex×9, tex+smp×8, ubo} → set 1: the material textures and `u_material_data`;
  - tag4 = {smp, ssbo×5, tex×4, tex+smp×5, ubo×6} → set 2: probes, CSM, clouds, scatter, fog, `ibl_probes_data`, `u_light_data`, `u_misc_data`, `u_new_sky_data`, `u_shadow_data`, `u_tonemap_data`, vegetation SSBOs.
  - Tag values seen: 0 (empty set), 1, 2, 3, 4, 5.
  - — [MAP](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/terrain_mapping.xsv), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/terrain.xsa)
- Compute per-permutation record: `u32 1, u8 0, u16 module, 6×0xFF, u8 4, u32 module, u32 lx, u32 ly, u32 lz`. For example, bloom's 4 permutations reference modules 0, 1, 2, 1 at 16×16×1, and fsr references modules 0–5 at 64×1×1. Permutations therefore map many-to-one onto deduplicated modules — [MAP](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/bloom_mapping.xsv), [MAP](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/fsr_mapping.xsv)
- Archives:
  - `spv/<family>.xsa` and `msl/<family>.xsa` are ZIP files of `<family>_<n>.spv` / `.msl` (deferred_gbuf: 991 modules, 38 MB SPIR-V; terrain: 4,614 modules, 205 MB SPIR-V).
  - The SPIR-V keeps `OpName`, `OpMemberName` and `OpDecorate` (set, binding, location, SpecId), and the `GL_EXT_*` / `GL_KHR_shader_subgroup_*` extension strings. That is GLSL compiled with glslang.
  - The MSL is SPIRV-Cross output (`main0`, `[[buffer(n)]]` style).
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/), [MSL](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/msl/)
- Name suffix variants:
  - `_2d` / `_2d_arr` / `_2d_ms` / `_2d_ms_arr` (exposure_fusion) and `_2d` / `_2d_array` / `_2d_ms_array` (weather_apply) are separate names bound at different bindings. Each module uses exactly one variant (e.g. weather 4 / 4 / 8 modules).
  - `GBUF_FMT` (deferred) and `GBUF_TYPE` (ssr) select 2D, 2DArray or 2DArrayMS for the same names.
  - — [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/exposure_fusion.xsa), [SPV](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/spv/weather_apply_compute.xsa)

### Inferences
- **Inference:** the group tag is a binding-frequency class:
  - 1 = global/per-view (transform, immediate, environment);
  - 5 = per-material;
  - 4 = per-pass;
  - 2/3 = utility layouts;
  - 0 = unused.
  This suggests X-Plane builds descriptor-set layouts per frequency and binds them with push descriptors (the log shows `push_descriptors: 1`).
- **Inference:** `_2d` = mono, `_2d_arr` = layered/stereo (VR), `_ms` = MSAA source. The engine chooses the permutation from the target's layer and sample count rather than from VK multiview.

### Gaps
- The raster per-permutation record and the middle "binding remap" table of the mapping format are not fully decoded. The SPIR-V decorations make them unnecessary for reproduction.

### Appendix — raw names per mapping file (one line per file; `perms` = permutation count from the header)
Source: [MAP](file:///K:/SteamLibrary/steamapps/common/X-Plane%2012/Resources/shaders/bin/) via `mapping.py` in [SCRIPTS](file:///C:/Users/traye/AppData/Local/Temp/claude/G--Repositorys-Coalition-of-Freeware-Developers-Scenery-Editor-X/67f57d29-13ad-4b5f-ab75-468c7bb89349/scratchpad/xsv/).

- `acf_map_icon` (perms=4): a_texcoord0 a_vertex u_immediate_data u_tex0 u_transform_block_fat
- `airport_raster` (perms=4): a_vertex u_transform_block_fat
- `astronomical` (perms=12): a_normal a_texcoord0 a_vertex u_astronomical_data u_dome_data u_environment_data u_immediate_data u_new_sky_data u_samp_scatter u_tex0 u_tex0_nrm u_tex_skydome u_tex_trans u_transform_block_fat
- `atmosphere` (perms=24): a_vertex multiple_scattering_texture scattering_density_texture single_mie_scattering_texture single_rayleigh_scattering_texture transmittance_texture u_atmosphere_data u_compute_atmosphere_data
- `background_blur` (perms=128): a_texcoord0 a_vertex tex_lumi_guide tex_mask tex_src u_background_blur_data u_environment_data u_immediate_data u_tonemap_data u_transform_block_fat
- `blit` (perms=32): a_texcoord0 a_vertex blit_data u_tex_src u_transform_block_fat
- `bloom` (perms=4): i_blur_in i_blur_out i_rip_out s_rip_in u_bloom_data
- `blur` (perms=8): a_texcoord0 a_vertex u_blur_data u_immediate_data u_tex u_transform_block_fat
- `cacao` (perms=114): LoadCounter g_DeinterleavedDepth g_DeinterleavedNormals g_DepthIn g_ImportanceMap g_ImportanceMapPong g_LinearClampSampler g_NormalIn g_PointClampSampler g_PointMirrorSampler g_RwDeinterleavedDepth g_RwDeinterleavedNormals g_RwDepthMips g_RwImportanceMap g_RwImportanceMapPong g_RwOutput g_RwSsaoBufferPing g_RwSsaoBufferPong g_SsaoBufferPing g_SsaoBufferPong g_ViewspaceDepthTapSampler u_ffx_cacao_data
- `cloud_categorize` (perms=12): gbuffer_pos u_cloud_categorize_data u_gbuffer_data u_samp_scatter u_tex_cloud_colour u_tex_cloud_depth u_tex_volumetric_fog u_transform_block_fat
- `cloud_map` (perms=128): a_texcoord0 a_vertex smp_color_stripe smp_terrain_map u_cloud_map_data u_cloud_render_data u_immediate_data u_samp_noise u_samp_str u_tex_noise_hi u_tex_noise_lo u_tex_weather_map_coverage u_tex_weather_map_properties u_transform_block_fat
- `cloud_minify` (perms=7): img_in_jump_cache img_in_sdf img_io_jump_cache img_io_sdf_as_dword img_out_jump_cache img_out_jump_cache_bounce img_out_sdf tex_in_weather_coverage tex_in_weather_properties u_cloud_minify_data
- `cloud_precipitation` (perms=1): img_precipitation u_cloud_precipitation_data u_cloud_render_data u_tex_noise_lo u_tex_weather_map_coverage u_tex_weather_map_hiz u_tex_weather_map_properties
- `cloud_render_compute` (perms=108): cloud_debug_data gbuffer_pos img_output_cloud_t img_output_depth_t u_cloud_render_data u_gbuffer_data u_new_sky_data u_samp_noise u_samp_scatter u_samp_str u_shadow_data u_tex_cloud_shadow_cascades u_tex_in_scatter u_tex_noise_cirrus u_tex_noise_hi u_tex_noise_lo u_tex_out_scatter u_tex_previous_cloud_depth u_tex_sky_ambient u_tex_trans u_tex_weather_map_coverage u_tex_weather_map_hiz u_tex_weather_map_properties
- `cloud_render_raster` (perms=864): a_vertex cloud_debug_data gbuffer_pos u_cloud_render_data u_gbuffer_data u_new_sky_data u_samp_noise u_samp_scatter u_samp_str u_shadow_data u_tex_cloud_shadow_cascades u_tex_in_scatter u_tex_noise_cirrus u_tex_noise_hi u_tex_noise_lo u_tex_out_scatter u_tex_previous_cloud_depth u_tex_sky_ambient u_tex_trans u_tex_weather_map_coverage u_tex_weather_map_hiz u_tex_weather_map_properties u_transform_block_fat
- `cloud_shadows` (perms=3): img_cloud_shadow img_cloud_shadow_previous u_cloud_render_data u_cloud_shadow_data u_tex_cloud_shadow_previous u_tex_noise_lo u_tex_weather_map_coverage u_tex_weather_map_properties
- `cloud_update` (perms=2): cloud_dst_weather_data cloud_new_weather_data img_current_weather_data_coverage img_current_weather_data_properties u_cloud_update_data
- `cloud_upscale` (perms=4): img_inout_cloud_t img_inout_depth_t img_out_proxy_cloud_t img_out_proxy_depth_t u_cloud_upscale_data u_tex_cloud_depth_history u_tex_cloud_history u_tex_src_cloud_colour u_tex_src_cloud_depths
- `compute_fft` (perms=24): fft_data_t fft_target_t i_target i_target_array lookup_table_t u_compute_fft_data
- `cube_filter_compute` (perms=4): i_mip u_cube_blur_data u_src_tex
- `cube_filter_raster` (perms=16): u_cube_blur_data u_src_tex
- `debug` (perms=1152): a_texcoord0 a_vertex sparse_image_residency_table_t sparse_image_table_t u_ctex0 u_debug_shader_data u_immediate_data u_sam0 u_tex0 u_transform_block_fat
- `deferred_gbuf` (perms=20480): a_color a_normal a_texcoord0 a_texcoord1 a_vertex deferred_metering_data gbuffer_lit gbuffer_mat gbuffer_nrm gbuffer_pos gbuffer_tex ibl_probes_data index_result_t light_spill_state light_tile_data spill_compact_input ssbo_light_list tex_brdf tex_cubemap_probes tex_smap0 tex_ssr tile_result_t u_deferred_shading_data u_environment_data u_gbuffer_data u_immediate_data u_new_sky_data u_samp_scatter u_shadow_data u_tex_cloud_shadow_cascades u_tex_in_scatter u_tex_out_scatter u_tex_ssao u_tex_trans u_transform_block_fat
- `depth_of_field` (perms=40): DofGlobalVars_t LinearSampler PointSampler r_input_color r_input_depth r_internal_bilat_color r_internal_dilated_radius rw_internal_bilat_color rw_internal_dilated_radius rw_internal_far rw_internal_near rw_internal_radius rw_output_color u_ffx_dof_ubo
- `depth_resolve` (perms=8): u_depth_resolve_data u_depth_tex
- `device_hang` (perms=1): device_hang_data device_hang_sink
- `dome` (perms=32): u_dome_data u_environment_data u_new_sky_data u_samp_scatter u_tex_skydome u_tex_trans u_transform_block_fat
- `exposure_fusion` (perms=16): img_accum_2d img_accum_2d_arr img_luma_2d img_luma_2d_arr img_weights_2d img_weights_2d_arr tex_accum_2d tex_accum_2d_arr tex_luma_2d tex_luma_2d_arr tex_original_image_2d tex_original_image_2d_arr tex_original_image_2d_ms tex_original_image_2d_ms_arr tex_weights_2d tex_weights_2d_arr u_exposure_fusion_data u_tonemap_data
- `fake_terrain` (perms=64): a_normal a_texcoord0 a_vertex instance_data tex_base_texture tex_brdf tex_bump tex_cubemap_ext tex_material tex_nite u_environment_data u_immediate_data u_material_data u_misc_data u_transform_block_fat
- `fix_hdr` (perms=2): hdr_fix_data u_hdr
- `font` (perms=24): a_texcoord0 a_vertex u_environment_data u_immediate_data u_tex0 u_transform_block_fat
- `fsr` (perms=6): i_output_texture u_fsr_data u_input_sampler u_input_texture
- `ground_lights` (perms=4): a_texcoord0 a_vertex u_environment_data u_immediate_data u_tex u_transform_block_fat
- `gstack` (perms=144): i_output s_input u_blur_data2
- `hdr` (perms=960): tex_lumi_guide u_environment_data u_fxaa_stencil_mask u_hdr_data u_immediate_data u_tex1 u_tonemap_data u_transform_block_fat
- `histo_debug` (perms=4): a_vertex i_tone sbo_histogram u_histo_data u_immediate_data u_transform_block_fat
- `in_place_blur` (perms=2): img_image u_in_place_data
- `legacy_depth` (perms=144): a_color a_texcoord0 a_vertex u_environment_data u_immediate_data u_tex0 u_transform_block_fat
- `legacy_flat` (perms=288): a_color a_normal a_texcoord0 a_vertex u_environment_data u_immediate_data u_legacy_data u_tex0 u_tex1 u_transform_block_fat
- `light` (perms=24): ssbo_light_vertex u_environment_data u_lights_tex0 u_new_sky_data u_ogl_light_data u_samp_scatter u_tex_cloud_colour u_tex_cloud_depth u_tex_out_scatter u_tex_volumetric_fog u_transform_block_fat
- `light_spill_compact` (perms=2): light_list_input light_spill_state spill_compact_data spill_compact_exterior_output spill_compact_interior_output spill_indirect_output
- `light_tile` (perms=6): index_result_t light_spill_state light_tile_data light_tile_debug_data ssbo_light_list tile_result_t u_gbuffer_data u_tex_z_minmax
- `light_transform` (perms=2): input_light_list light_transform_data light_transform_debug_data output_light_list u_gbuffer_data u_tex_z_minmax
- `light_vis` (perms=8): light_tile_data light_vis_data ssbo_light_list tile_result_t u_transform_block_fat
- `line3d` (perms=12): a_color a_texcoord0 a_texcoord1 a_vertex u_environment_data u_immediate_data u_line3d_data u_samp_scatter u_tex_cloud_colour u_tex_cloud_depth u_tex_volumetric_fog u_transform_block_fat
- `lumi_histo` (perms=6): image_input_hdr io_output sbo_histogram sbo_result_data u_histo_data
- `manip3d` (perms=8): a_normal a_texcoord0 a_vertex u_environment_data u_immediate_data u_transform_block_fat
- `minify` (perms=240): SpdGlobalAtomicBuffer img_dst_12 img_dst_6 img_dst_8 img_src u_minify_data
- `mixed_reality` (perms=48): a_normal a_texcoord0 a_vertex instance_data u_alpha_image u_environment_data u_immediate_data u_transform_block_fat
- `msaa_categorize` (perms=4): gbuffer_nrm u_transform_block_fat
- `ocean_bake_textures` (perms=2): fft_data i_previous i_target u_ocean_bake_textures_data
- `ocean_mesh_bake` (perms=2): index_data mesh_data result_data u_depth_map u_depth_map_sampler u_ocean_mesh_data u_parameter_map u_water_data
- `ocean_mesh_data` (perms=2): command_data result_data u_ocean_mesh_command_data
- `ocean_meta_data` (perms=1024): a_normal a_texcoord0 a_texcoord1 a_vertex gbuffer_pos smp_deca tex_deca tex_decb tex_noise u_color u_environment_data u_material_data u_noise u_ocean_meta_data u_parameter_map u_transform_block_fat
- `ocean_readback` (perms=8): a_normal a_texcoord0 a_texcoord1 a_vertex u_color u_ocean_readback_data u_parameter_map u_transform_block_fat
- `ocean_shading` (perms=256): a_normal a_texcoord0 a_texcoord1 a_vertex line_index_data_t line_vertex_data_t tex_ssr u_environment_data u_new_sky_data u_ocean_shading_data u_parameter_map u_samp_scatter u_shadow_data u_skydome u_tex_cloud_shadow_cascades u_tex_in_scatter u_tex_out_scatter u_tex_trans u_transform_block_fat u_variance u_water_color_tex u_water_meta_tex u_water_mrt_sampler u_water_nrm_tex
- `ocean_spectrum` (perms=3): fft_data h0_data h0_temp_data i_h0 i_wave_table u_noise u_ocean_spectrum_data wave_table_data
- `ocean_variance` (perms=2): dot_table_data i_result i_table u_ocean_variance
- `particle` (perms=432): a_color a_normal a_texcoord0 a_texcoord1 a_vertex gbuffer_pos u_blur u_environment_data u_new_sky_data u_particle_data u_samp_scatter u_tex u_tex_cloud_colour u_tex_cloud_depth u_tex_cubemap u_tex_in_scatter u_tex_out_scatter u_tex_trans u_tex_volumetric_fog u_transform_block_fat
- `planet` (perms=8): a_vertex tex_mie_phase tex_planet_albedo tex_planet_elevation tex_planet_normal tex_water u_environment_data u_material_data u_multi_tex u_planet_params u_samp_scatter u_scattering_data u_tex_in_scatter u_tex_out_scatter u_trans_tex u_transform_block_fat
- `profile_view` (perms=4): a_texcoord0 a_vertex u_profile_view_data u_tex_elevation u_tex_sectional u_transform_block_fat
- `rain` (perms=4): a_texcoord0 a_vertex u_environment_data u_immediate_data u_rain_data u_transform_block_fat
- `rain_draw` (perms=8): droplet_data droplet_draw_data trail_draw_data
- `rain_droplets_compute` (perms=12): droplet_data droplet_draw_data droplet_trail_data indirect_draw_t input_droplet_grid_t tex_glass_forces tex_glass_surface u_rain_force_data u_rain_info u_rain_surface_info
- `rain_droplets_wipers` (perms=2): droplet_data img_debug_output input_droplet_grid_t tex_glass_wiper u_rain_info u_rain_surface_info
- `rain_forces` (perms=8): a_normal a_texcoord0 a_vertex force_data_t instance_data u_rain_force_data u_rain_force_meta u_transform_block_fat
- `rain_forces_visualize` (perms=1): force_data_t line_index_data_t line_vertex_data_t u_rain_force_data u_rain_visualize
- `rain_ice_accumulation` (perms=4): input_ice_readback_t u_force_data u_glass_surface u_ice_accumulation u_rain_info u_rain_surface_info u_thermal_tex
- `rain_normals` (perms=4): u_depth_tex u_ice_tex u_rain_info u_rain_surface_info
- `rain_surface` (perms=8): a_normal a_texcoord0 a_vertex instance_data screenshot_tex u_depth_tex u_ice_tex u_normal_tex u_rain_draw_info u_rain_info u_samp_scatter u_tex_cloud_colour u_tex_cloud_depth u_tex_volumetric_fog u_transform_block_fat
- `rain_visualize` (perms=1): index_data_t line_index_data_t line_vertex_data_t u_bitangent_data u_force_data u_normal_data u_rain_visualize u_tangent_data vertex_data_t
- `resolve` (perms=48): a_color a_texcoord0 a_vertex u_environment_data u_immediate_data u_resolve_shader_data u_tex u_tonemap_data u_transform_block_fat
- `ripple` (perms=1): output_tex ripple_seed_tex u_ripple_data
- `scatter_compute_multi` (perms=1): img_output_t u_scattering_data u_trans_tex
- `scatter_compute_scattering` (perms=4): img_output_in_scatter img_output_out_scatter tex_gbuffer_z tex_mie_phase u_gbuffer_data u_multi_tex u_scattering_data u_scattering_view u_shadow_data u_tex_cloud_shadow_cascades u_trans_tex
- `scatter_compute_skydome` (perms=8): img_output_t tex_mie_phase u_multi_tex u_scattering_data u_scattering_view u_shadow_data u_tex_cloud_shadow_cascades u_trans_tex
- `scatter_compute_t` (perms=1): img_output_t u_scattering_data
- `scatter_render_atmosphere` (perms=48): gbuffer_pos tex_mie_phase u_environment_data u_gbuffer_data u_multi_tex u_new_sky_data u_samp_scatter u_scattering_data u_scattering_view u_tex_in_scatter u_tex_out_scatter u_trans_tex u_transform_block_fat
- `sectional2` (perms=64): a_texcoord0 a_vertex u_terrain_layer_data u_terrain_runway_data u_tex_egpws u_tex_elevation u_tex_normals u_tex_sectional u_tex_watermask u_transform_block_fat
- `shading_rate` (perms=2): image_output shading_rata_shape_data shading_rate_data
- `shading_rate_vis` (perms=4): u_shading_rate_attachment u_transform_block_fat
- `shadow_rect` (perms=4): a_vertex u_immediate_data u_shadow_rect u_transform_block_fat
- `single_pass_blur` (perms=2): img_out_image tex_in_image u_single_pass_blur_data
- `ssr_deferred` (perms=24): gbuffer_mat gbuffer_nrm gbuffer_pos gbuffer_vel i_output s_water_nrm tex_brdf tex_hiZ tex_ssr u_environment_data u_gbuffer_data u_ssr_data water_pos
- `ssr_mesh` (perms=8): a_normal a_texcoord0 a_vertex instance_data tex_brdf tex_hiZ tex_material_map tex_ssr u_environment_data u_gbuffer_data u_ssr_data u_ssr_mesh_material_data u_transform_block_fat
- `svt` (perms=4): synthetic_vision_data synthetic_vision_terrain_data u_tex_elevation u_tex_noise u_tex_sectional u_tex_terrain u_tex_watermask u_transform_block_fat
- `tchotchke` (perms=8): a_texcoord0 a_vertex u_environment_data u_immediate_data u_tex0 u_transform_block_fat
- `terrain` (perms=829440): a_normal a_texcoord0 a_texcoord1 a_vertex ibl_probes_data instance_data tex_base_sampler tex_base_texture tex_bord tex_brdf tex_bump tex_comp tex_comp1_seasonal_texture tex_comp2 tex_comp2_seasonal_texture tex_cubemap_probes tex_dec2_texture tex_deca_sampler tex_deca_texture tex_material tex_nite tex_noise tex_norm_dec2_texture tex_norm_deca_texture tex_seasonal_texture tex_smap0 tex_weather_texture u_environment_data u_immediate_data u_light_data u_material_data u_misc_data u_new_sky_data u_samp_scatter u_shadow_data u_tex_cloud_colour u_tex_cloud_depth u_tex_cloud_shadow_cascades u_tex_in_scatter u_tex_out_scatter u_tex_trans u_tex_volumetric_fog u_tonemap_data u_transform_block_fat vegetation_info_t vegetation_mesh_t vegetation_points_t vegetation_result_t vegetation_source_points_t
- `vegetation_cull` (perms=2): input_points_t object_data_t result_data_t result_points_t u_environment_data u_vegetation_cull_data
- `vegetation_data` (perms=12): command_t object_t result_t swedish_buffer_t u_swedish_meta_data u_vegetation_cull_data
- `volumetric_apply` (perms=48): gbuffer_pos u_cloud_sampler u_gbuffer_data u_samp_scatter u_tex_cloud_colour0 u_tex_cloud_colour1 u_tex_cloud_depths0 u_tex_cloud_depths1 u_tex_volumetric_fog u_transform_block_fat u_volumetric_apply_data
- `volumetric_fog` (perms=4): gbuffer_pos img_output_volumetric_density img_output_volumetric_fog u_gbuffer_data u_new_sky_data u_samp_noise u_samp_scatter u_samp_str u_shadow_data u_tex_cloud_precipitation u_tex_cloud_shadow_cascades u_tex_in_scatter u_tex_noise_cirrus u_tex_noise_hi u_tex_noise_lo u_tex_out_scatter u_tex_sky_ambient u_tex_trans u_tex_volumetric_density u_tex_volumetric_density_history u_tex_weather_map_coverage u_tex_weather_map_properties u_volumetric_fog_data
- `vr_depth_copy` (perms=8): u_depth_source u_transform_block_fat vr_depth_copy_data
- `weather_apply_compute` (perms=16): img_gbuffer_albedo_2d img_gbuffer_albedo_2d_array img_gbuffer_albedo_2d_ms_array img_gbuffer_material_2d img_gbuffer_material_2d_array img_gbuffer_material_2d_ms_array img_gbuffer_normal_2d img_gbuffer_normal_2d_array img_gbuffer_normal_2d_ms_array tex_droplets tex_gbuffer_z_2d tex_gbuffer_z_2d_array tex_gbuffer_z_2d_ms_array tex_ice_alb tex_ice_dcl tex_ice_nml tex_rain_area_noise tex_snow_alb tex_snow_coverage tex_snow_dcl tex_snow_nml tex_weather_2d tex_weather_2d_array tex_weather_2d_ms_array u_gbuffer_data u_weather_apply_data
- `weather_apply_raster` (perms=64): tex_droplets tex_gbuffer_albedo_2d tex_gbuffer_albedo_2d_array tex_gbuffer_albedo_2d_ms_array tex_gbuffer_material_2d tex_gbuffer_material_2d_array tex_gbuffer_material_2d_ms_array tex_gbuffer_normal_2d tex_gbuffer_normal_2d_array tex_gbuffer_normal_2d_ms_array tex_gbuffer_z_2d tex_gbuffer_z_2d_array tex_gbuffer_z_2d_ms_array tex_ice_alb tex_ice_dcl tex_ice_nml tex_rain_area_noise tex_snow_alb tex_snow_coverage tex_snow_dcl tex_snow_nml tex_weather_2d tex_weather_2d_array tex_weather_2d_ms_array u_gbuffer_data u_transform_block_fat u_weather_apply_data
- `wxr_mask` (perms=4): a_vertex u_transform_block_fat u_wxr_mask_data
- `z_minmax` (perms=9): image_input_mip_z image_input_raw_z image_output_z u_z_minmax_data
