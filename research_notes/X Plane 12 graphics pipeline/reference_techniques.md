# Reference techniques and open-source implementations matching X-Plane 12's rendering passes (for a Vulkan 1.3 / Slang renderer)

Scope note: this file covers the published techniques and reference code that match the pass names in X-Plane 12's shader binding files (`scatter_compute_*`, `exposure_fusion`, `lumi_histo`, `cacao`, `ssr_*`, `cube_filter_compute`, `shadow_rect`, `volumetric_fog`, `fsr`, `ocean_*`, `rain_*` / `weather_apply_*`, `cloud_*`, `light_tile`, `bloom`, `depth_of_field`, `svt`). Linking a pass name to a technique is an inference from the name. Laminar Research has not documented its internals, and these notes do not cover X-Plane's own binaries. Research date: 2026-10-19.

---

## Atmosphere: Hillaire 2020 vs Bruneton 2008/2017 (LUT sizes and formats, mapping ozone/aerosol/haze parameters, open implementations)

### Takeaway
X-Plane's split into `scatter_compute_t` (transmittance), `scatter_compute_multi` (multi-scattering), `scatter_compute_skydome` (sky-view) and `scatter_render_atmosphere` lines up with the four small LUTs in Hillaire 2020. Bruneton's 4D scattering texture would need one large precompute instead. Hillaire's MIT-licensed HLSL demo is the closest reference to port to Slang. Bruneton's BSD code is the accuracy reference, and its transmittance LUT (256x64) is the one Hillaire reuses.

### Cited Findings
- **Hillaire 2020 paper:** "A Scalable and Production Ready Sky and Atmosphere Rendering Technique", EGSR 2020 / Computer Graphics Forum 39(4). Per the abstract, the method needs no high-dimensional LUT: a few simple LUTs and parameterizations cover both the sky and aerial perspective. — [EG Digital Library](https://diglib.eg.org/handle/10.1111/cgf14050)
- **Reference repo:** `sebh/UnrealEngineSkyAtmosphere`, MIT license, HLSL shaders (`RenderSkyRayMarching.hlsl` builds the LUTs, `RenderSkyPathTracing.hlsl` is a path-traced ground truth) with a DX11 C++ host. It is Windows-only and unsupported, and it compares the new method against Bruneton and against a path tracer. — [GitHub sebh/UnrealEngineSkyAtmosphere](https://github.com/sebh/UnrealEngineSkyAtmosphere)
- **Transmittance LUT:** stores colored transmittance from a point to outer space, the same LUT Bruneton 2008 proposed. By default one top-of-atmosphere-to-ground transmittance is applied to all entities. An option applies it per pixel for space views and the terminator. — [Hillaire SIGGRAPH 2020 PBS course slides](https://blog.selfshadow.com/publications/s2020-shading-course/hillaire/s2020_pbs_hillaire_slides.pdf)
  - In the demo, the transmittance LUT uses 40 fixed ray-march samples. — [RenderSkyRayMarching.hlsl](https://raw.githubusercontent.com/sebh/UnrealEngineSkyAtmosphere/master/Resources/RenderSkyRayMarching.hlsl)
  - Bruneton's reference transmittance texture is 256 x 64. — [Bruneton constants.h](https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/constants.h.html)
- **Multi-scattering LUT:**
  - A 2D table of isotropic multiple-scattering contribution, indexed by sample altitude and sun zenith angle. It approximates infinitely many scattering orders. — [Hillaire slides](https://blog.selfshadow.com/publications/s2020-shading-course/hillaire/s2020_pbs_hillaire_slides.pdf)
  - The demo integrates 64 directions on an 8x8 grid (`[numthreads(1,1,64)]`, `SQRTSAMPLECOUNT 8`) with 20 samples per direction. It computes `L = L_2ndOrder * 1/(1 - f_ms)`, a geometric series (`MULTI_SCATTERING_POWER_SERIE 1`). — [RenderSkyRayMarching.hlsl](https://raw.githubusercontent.com/sebh/UnrealEngineSkyAtmosphere/master/Resources/RenderSkyRayMarching.hlsl)
  - Low-end option: sample only two directions (top and bottom) and multiply by 2.
  - Caveats: the LUT diverges slowly for anisotropic phase functions, and the hue drifts for very high, colored scattering coefficients. — [Hillaire slides](https://blog.selfshadow.com/publications/s2020-shading-course/hillaire/s2020_pbs_hillaire_slides.pdf)
- **Sky-view LUT:**
  - A lat/long map of distant-sky luminance. The latitude mapping is non-linear to keep horizon detail, and the LUT can hold any number of suns at once. The sun disk is composited afterwards. — [Hillaire slides](https://blog.selfshadow.com/publications/s2020-shading-course/hillaire/s2020_pbs_hillaire_slides.pdf)
  - The demo hard-codes 192 x 108. With `NONLINEARSKYVIEWLUT 1`, the zenith coordinate is squared on decode and square-rooted on encode. Mie uses the Cornette-Shanks phase function (`USE_CornetteShanks`). — [RenderSkyCommon.hlsl](https://github.com/sebh/UnrealEngineSkyAtmosphere/blob/master/Resources/RenderSkyCommon.hlsl)
  - The sky-view pass starts from 30 samples, varied by `RayMarchMinMaxSPP`. — [RenderSkyRayMarching.hlsl](https://raw.githubusercontent.com/sebh/UnrealEngineSkyAtmosphere/master/Resources/RenderSkyRayMarching.hlsl)
- **Aerial-perspective volume:**
  - A froxel volume mapped onto the camera frustum (citing Wronski 14 and Hillaire 16). RGB holds luminance reaching the camera from the froxel center, A holds greyscale transmittance. It applies to opaque and translucent surfaces. — [Hillaire slides](https://blog.selfshadow.com/publications/s2020-shading-course/hillaire/s2020_pbs_hillaire_slides.pdf)
  - Demo values: `AP_SLICE_COUNT 32`, `AP_KM_PER_SLICE 4` (32 slices x 4 km = 128 km). Samples per slice are `max(1, (slice+1)*2)`. — [RenderSkyRayMarching.hlsl](https://raw.githubusercontent.com/sebh/UnrealEngineSkyAtmosphere/master/Resources/RenderSkyRayMarching.hlsl)
  - Unreal's `r.SkyAtmosphere.AerialPerspectiveLUT.Depth` defaults to 96 km, and anything farther uses the last slice. — [UE Sky Atmosphere docs](https://dev.epicgames.com/documentation/unreal-engine/sky-atmosphere-component-properties-in-unreal-engine)
  - A community note lists UE's AP froxel volume as 32x32x32. This is informal and secondary. — [jojo-lyu notes via search](https://jojo-lyu.notion.site/s2020-Scalable-Atmospheres-in-Unreal-Engine-6cbf50aae15f4701a4c01b589f63c2c1)
- **Cost:**
  - Slides (NVIDIA GTX 1080): "Render sky & atmosphere @720p = 0.14ms", "Total cost = 0.31ms". A separate PC-vs-mobile comparison lists "Lut cost = 1.03ms".
  - The slides say the transmittance LUT can drop in resolution and sample count on mobile, and that LUT resolution plus sample count are the scalability knobs, independent of screen resolution. — [Hillaire slides](https://blog.selfshadow.com/publications/s2020-shading-course/hillaire/s2020_pbs_hillaire_slides.pdf)
- **Volumetric shadows with clouds:** Hillaire proposes a "Beer Shadow Map" stored as R11G11B10f (R = front depth, G = mean extinction behind the front). Volumetric shadows can be traced per pixel with jitter and TAA, or baked into the sky-view and AP LUTs for speed. — [Hillaire slides](https://blog.selfshadow.com/publications/s2020-shading-course/hillaire/s2020_pbs_hillaire_slides.pdf)
- **Bruneton reference code:** `ebruneton/precomputed_atmospheric_scattering`, BSD license. It reimplements the EGSR 2008 paper with descriptive names, extensive comments, documentation, unit tests and a WebGL demo (OpenGL/WebGL topics). — [GitHub ebruneton](https://github.com/ebruneton/precomputed_atmospheric_scattering)
- **Bruneton texture sizes:**

| Texture | Size |
|---|---|
| Transmittance | 256 x 64 |
| Scattering (4D packed into 3D) | R 32, MU 128, MU_S 32, NU 8, stored as 256 x 128 x 32 |
| Irradiance | 64 x 16 |

  The code also includes CIE 1931 colour-matching tables and XYZ-to-sRGB conversion for spectral rendering. — [Bruneton constants.h](https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/constants.h.html)
- **Other implementations of Hillaire 2020:**
  - `hoffstadt/pl-sky` builds all four LUTs (transmittance, multi-scattering, sky-view with non-linear horizon mapping, 3D aerial perspective) on Pilot Light. It needs the Vulkan SDK and builds on Windows, macOS and Linux, but states no license. — [GitHub pl-sky](https://github.com/hoffstadt/pl-sky)
  - FlightGear's HDR pipeline implements the paper with 40-step transmittance, 32 aerial-perspective slices over 128 km, and 20 samples per ray, stored in a tiled 2D texture rather than a 3D texture. — [FlightGear fgdata atmos shaders (search result)](https://git.nayala.duckdns.org/fly/fgdata/src/commit/37f43bd671d192753b2f4527a2b66ab8e696786d/Shaders/HDR/atmos-aerial-perspective.frag)

### Inferences
- **Pass mapping:** X-Plane's naming (`_t`, `_multi`, `_scattering`, `_skydome`) matches Hillaire's set of separate LUTs better than Bruneton's precompute. A plausible mapping:

| X-Plane pass | Hillaire stage |
|---|---|
| `scatter_compute_t` | transmittance LUT |
| `scatter_compute_multi` | multi-scattering LUT |
| `scatter_compute_skydome` | sky-view LUT |
| `scatter_compute_scattering` | aerial-perspective volume, or a Bruneton-style inscatter volume |
| `scatter_render_atmosphere` | sky / aerial-perspective composite |

  This is inference from names only.
- **Proposed formats (common practice, not confirmed for the demo):**

| LUT | Size | Format |
|---|---|---|
| Transmittance | 256x64 | RGBA16F |
| Multi-scattering | 32x32 | RGBA16F |
| Sky-view | 192x108 | R11G11B10F or RGBA16F |
| Aerial perspective | 32x32x32 | RGBA16F (RGB inscatter, A transmittance) |

  The 256x64, 192x108 and 32 slices are confirmed above. 32x32 multi-scattering and the formats are not.
- **Slang port:** the HLSL from `sebh` ports almost one-to-one, since Slang accepts HLSL syntax. Swap the DX11 pixel-shader LUT passes for `[shader("compute")]` kernels writing `RWTexture2D` / `RWTexture3D`. Recompute transmittance and multi-scattering only when atmosphere parameters change, and sky-view and aerial perspective every frame (both are tiny).
- **Flight-sim altitude:** Hillaire's planet-scale parameterization supports space and high-altitude views directly, which suits a flight sim at FL400+. Bruneton also supports any altitude, but changing its parameters means re-running a precompute that is expensive at full scattering order.
- **Mapping weather parameters to media:**
  - Ozone maps to an absorption-only layer (a tent profile in Hillaire and Bruneton 2017).
  - Aerosols or haze map to Mie scattering and absorption density with a scale height of about 1.2 km.
  - Visibility can map to a Mie or fog extinction coefficient using the Koschmieder relation, extinction ≈ 3.912 / visibility. This is a standard meteorological relation from general knowledge and was not sourced in this session.
  - X-Plane's METAR visibility could drive this extinction directly.

### Gaps
- I could not confirm the exact multi-scattering LUT resolution (commonly cited as 32x32) or the LUT texture formats from a primary source. The paper PDF was too large to fetch (more than 10 MB, 403 on the mirror), and the LUT table in the slides is an image.
- I did not verify the default atmosphere coefficients (Rayleigh β, Mie β/σa, ozone σa, scale heights, ozone tent profile) from `SkyAtmosphereCommon.hlsl` / `GameScene.cpp` or the paper. The Bruneton 2017 demo's ozone layer is likewise not confirmed here.
- pl-sky shows no license. FlightGear's fgdata license was not checked; it is probably GPL, which would make it reference reading only for a permissively licensed project.

---

## Tone mapping: exposure fusion local tone mapping, histogram auto exposure, filmic curves (which gives a "photographic" look; parameters to expose)

### Takeaway
X-Plane's `exposure_fusion` + `lumi_histo` pairing matches Bart Wronski's 2022 real-time adaptation of Mertens et al.'s exposure fusion (public domain demo code) driven by a log-luminance histogram auto-exposure. Wronski's method was explicitly framed as bringing a smartphone-camera (HDR+) "photographic" local tone mapping to games. A neutral global curve (Khronos PBR Neutral, Apache-2.0, or an AgX/ACES-fit curve) then follows.

### Cited Findings
- **Mertens, Kautz, Van Reeth 2007, "Exposure Fusion":** blends a bracketed exposure sequence directly into a high-quality LDR image without HDR assembly. Weights come from contrast, saturation and well-exposedness, blended multi-resolution with Laplacian pyramids. — [Mertens et al. PDF](https://www.jankautz.com/publications/exposure_fusion.pdf); summary in [Wronski 2022](https://bartwronski.com/2022/02/28/exposure-fusion-local-tonemapping-for-real-time-rendering/)
- **Wronski 2022, "Exposure Fusion – local tonemapping for real-time rendering":**
  - Synthetic exposures: three is "a pretty good starting choice" ("the more – the better"), packable into one texture.
  - Weights: use only the well-exposedness term, a Gaussian around 0.5 lightness controlled by an "exposure preference sigma". Drop the contrast and saturation terms.
  - Pyramids: Laplacians are differences of Gaussian mips, so no extra storage. The coarsest blended level is around mip 5-6.
  - Resolution: everything is computed at 1/4 x 1/4 resolution, then guided-filter upsampled (joint bilateral as an alternative) to full resolution.
  - Cost: unprofiled, estimated "under 1ms on even previous generation consoles". The dominant cost is tonemapping the synthetic exposures.
  - Exposed parameters: exposure (midtones), shadows and highlights (brightness of the synthetic exposures relative to the middle one), coarsest mip level (compression vs washed-out look), exposure preference sigma (strength).
  - Artifacts discussed: halos (his earlier God of War approach), gradient reversals with bilateral filters, a "toxic HDR" look, and flicker or diamond artifacts from naive bilinear down/upsampling.
  - Demo code: public domain (JavaScript/WebGL).
  - Lineage: he presents this as the core of the local tone mapping in Google HDR+ (camera pipeline). — [Wronski 2022](https://bartwronski.com/2022/02/28/exposure-fusion-local-tonemapping-for-real-time-rendering/)
- **Histogram auto exposure (Opsenica):**
  - 256 bins: bin 0 for luminance below 0.005, bins 1-255 spanning a configurable log2 range.
  - Pass 1: 16x16 thread groups accumulate in shared memory with atomics, then merge into the global histogram.
  - Pass 2: a 256-thread reduction computes the weighted mean bin, excluding bin 0, and resets the histogram.
  - Temporal smoothing: `adapted = last + (new - last) * timeCoeff`.
  - Exposure: EV100 = log2(Lavg · S/K) with S = 100, K = 12.5. Lmax = 78/(q·S) · 2^EV100 with q = 0.65, which simplifies to Lmax = 9.6 · Lavg and H = 1/Lmax.
  - Code is a bgfx-based sample; license not stated. — [bruop.github.io/exposure](https://bruop.github.io/exposure/)
- **ACES fitted (Narkowicz 2015):** `saturate((x*(2.51x+0.03))/(x*(2.43x+0.59)+0.14))` per channel, an approximation sampled from the ACES output transform without the full ACES colour pipeline. — [ACES Filmic Tonemapping notes](https://scrapbox.io/0b5vr/ACES_Filmic_Tonemapping)
  - Stephen Hill's (BakingLab) rational fit is cited as more faithful, with a +0.25 EV (×1.189) compensation commonly applied. — [pixls.us discussion](https://discuss.pixls.us/t/base-curve-module-hybrid-aces-like-proof-of-concept/55796/86)
- **Khronos PBR Neutral:**
  - Repo `KhronosGroup/ToneMapping`, Apache-2.0. The goal is output sRGB that matches input baseColor "as faithfully as possible" under grey-scale lighting, for well-exposed scenes where HDR values are mostly small specular highlights. — [GitHub KhronosGroup/ToneMapping](https://github.com/KhronosGroup/ToneMapping)
  - Design: 1:1 colour up to a chosen maximum, with remaining headroom used to compress highlights. The sample is about 13 lines of GLSL and outputs sRGB only.
  - Adopters reported: Blender, Babylon.js, Three.js, Filament, model-viewer and others. — [Khronos news](https://www.khronos.org/news/permalink/khronos-pbr-neutral-tone-mapper-released-for-true-to-life-color-rendering-of-3d-products)
- **AgX (Troy Sobotka):**
  - Pipeline: inset gamut matrix, log2 encoding, sigmoid tonescale, outset matrix, display EOTF.
  - `MrLixm/AgXc` ports it to many languages and notes that Blender 4.x's variant differs from the original. — [GitHub MrLixm/AgXc](https://www.github.com/MrLixm/AgXc)
  - darktable's AgX module derives from Blender's display transform. — [darktable AgX docs](https://docs.darktable.org/usermanual/development/en/module-reference/processing-modules/agx/)
- **Uchimura "GT" tonemapper (CEDEC 2017, "HDR theory and practice", Polyphony Digital):** a toe / linear / shoulder curve. A secondary source lists controls Max Brightness, Contrast, Linear Start, Linear Length, Black Tightness and Pedestal; this is marketing copy, unverified. — [Fab listing (secondary)](https://www.fab.com/listings/8ac93617-31b7-42f7-ad80-b917b828083c)
- **Hable 2010 filmic (comparison):** default parameters shoulder 0.22, linear strength 0.3, linear angle 0.1, toe strength 0.2, toe numerator 0.01, toe denominator 0.3. — [colour-hdri docs](https://colour-hdri.readthedocs.io/en/develop/generated/colour_hdri.tonemapping_operator_filmic.html)

### Inferences
- **X-Plane chain:** a plausible reading of the pass names is:
  1. `lumi_histo`: histogram, giving average luminance and exposure.
  2. `hdr` / `fix_hdr`: HDR resolve, plus NaN/Inf or firefly clamping (guess).
  3. `exposure_fusion`: Wronski LTM at 1/4 resolution with guided upsampling.
  4. A global curve.

  X-Plane 12's look of bright skies with lifted cockpit shadows is consistent with exposure fusion, but that is subjective.
- **"Photographic" curve:** exposure fusion handles the camera-like local adaptation. The global curve should then be mild and hue-preserving. Khronos PBR Neutral (Apache-2.0, tiny) or AgX gives a neutral, photographic look. Narkowicz ACES shifts hues and oversaturates brights, as is widely reported, but that was not verified here.
- **Parameters to expose in an editor:**
  - Exposure compensation (EV)
  - Auto-exposure min/max EV and adaptation speed (separate up and down rates)
  - Histogram low/high percentile clip
  - LTM shadows boost, highlights compression and strength (sigma)
  - Coarsest mip
  - Curve choice (Neutral / AgX / ACES-fit)
  - White point
- **Slang/Vulkan port:**
  - Histogram: `InterlockedAdd` on `groupshared uint[256]`. Slang supports HLSL atomics and wave intrinsics.
  - Exposure fusion: needs a mip chain of three-exposure weights and colours. Use a single-pass downsampler (FidelityFX SPD-style) or `vkCmdBlitImage` mips.
  - Keep exposure in a 1x1 R32F buffer so FSR's exposure input can share it.

### Gaps
- I did not fetch Uchimura's primary curve formula and default constants (CEDEC 2017 slides). Use only after verifying against the slides or a known reference port.
- I did not fetch AgX's exact matrices and polynomial from Blender's OCIO config.
- I found no published evidence of what X-Plane actually uses for its global curve.

---

## PBR: GGX/Smith/Schlick split-sum IBL with DFG LUT, multi-scattering energy compensation, roughness from gloss, cube map prefiltering

### Takeaway
The standard reference stack has three parts:
- Karis 2013 split-sum.
- Filament's DFG formulation: an RG16F LUT where the multi-scatter variant gives energy compensation `1 + f0*(1/dfg.y - 1)`.
- GGX importance-sampled prefiltered cube mips, with LOD linear in perceptual roughness.

Khronos's Apache-2.0 Vulkan `glTF-IBL-Sampler` is a ready reference for X-Plane's `cube_filter_compute`.

### Cited Findings
- **Karis 2013:** "Real Shading in Unreal Engine 4" (SIGGRAPH 2013 PBS course) introduced the UE4 model and the split-sum IBL approximation. — [Karis course notes (copy)](https://cgvr.informatik.uni-bremen.de/teaching/cg_literatur/Brian%20Karis%20-%20Real%20Shading%20in%20Unreal%20Engine%204,%202013%20Siggraph%20Course.pdf)
- **Filament DFG LUT:**
  - A 2D texture indexed by (NoV, perceptual roughness) storing DFG1 and DFG2, the f0 and f90 scale terms. 8-bit is too imprecise; RG16F is recommended.
  - Multiscatter variant (f90 = 1): DFG1 = (4/N) Σ Fc(VoH) · V · (VoH/NoH) · NoL and DFG2 = (4/N) Σ V · (VoH/NoH) · NoL. Reconstruction: `L ≈ [(1-f0)·DFG1 + f0·DFG2] · LD(n, α)`.
  - Cloth adds a third channel (DG term). — [Filament docs](https://google.github.io/filament/Filament.md.html)
- **Multiple-scattering energy compensation (Filament):** `energyCompensation = 1.0 + f0 * (1.0/dfg.y - 1.0); Fr *= energyCompensation;`, derived from f_ms = F0 · (1 - E)/E · f_ss. — [Filament docs](https://google.github.io/filament/Filament.md.html)
- **Prefiltered environment (Filament):**
  - α = perceptualRoughness². Example: a 256² cube with 9 mips sampled at `lod = 8.0 * perceptualRoughness`.
  - Prefiltering uses "pre-filtered importance sampling" with pdf 4·VoH/(D·NoH), assuming v = n, F = 1, V = 1.
  - Diffuse irradiance uses 3-band SH (9 coefficients; 2 bands / 4 coefficients is acceptable), pre-convolved with the cosine lobe. — [Filament docs](https://google.github.io/filament/Filament.md.html)
- **Roughness clamp:** Filament clamps roughness to 0.089 so 1/roughness⁴ stays representable in fp16; Frostbite uses 0.045 for analytic lights with fp32. — [Filament docs](https://google.github.io/filament/Filament.md.html)
- **Specular occlusion from AO (Lagarde formula, as given by Filament):**
  - `saturate(pow(NoV + ao, exp2(-16*roughness - 1)) - 1 + ao)`, applied to indirect specular only.
  - Horizon occlusion: `horizon = min(1 + dot(r, n), 1); spec *= horizon²`.
  - Micro-occlusion from f0: `f90 = saturate(50 * f0.g)`. — [Filament docs](https://google.github.io/filament/Filament.md.html)
- **Gloss conventions:** Lagarde's wet-surface work uses gloss with 1 = smooth and derives porosity as `saturate(-2.5*Gloss + 1.25)`, treating smooth surfaces as non-porous. — [Lagarde, Water drop 3b](https://seblagarde.wordpress.com/2013/04/14/water-drop-3b-physically-based-wet-surfaces/)
- **glTF-IBL-Sampler (Khronos):**
  - Apache-2.0, Vulkan with Glslang (C++ / GLSL `filter.frag`).
  - Prefilters an HDR panorama into a diffuse cube map and a specular cube map with one mip per roughness. Distributions: Lambertian, GGX or Charlie (sheen).
  - Also outputs a BRDF LUT.
  - Options: `-sampleCount` (default 1024), `-lodBias`, `-mipLevelCount`, `-cubeMapResolution`, formats R8G8B8A8_UNORM / R16G16B16A16_SFLOAT / R32G32B32A32_SFLOAT, output KTX1/KTX2. — [GitHub KhronosGroup/glTF-IBL-Sampler](https://github.com/KhronosGroup/glTF-IBL-Sampler)

### Inferences
- **`cube_filter_compute`:** most likely a per-frame (or amortized) GGX prefilter of a dynamically rendered sky/scene cube map, which X-Plane needs because time of day and weather change continuously.
- **Real-time variant:** filtered importance sampling (sample the source cube's mips by pdf, so 16-64 samples per texel suffice) instead of the 1024-sample offline default. Amortize one face or mip per frame.
- **Roughness from X-Plane gloss:** X-Plane material gloss probably maps as `perceptualRoughness = 1 - gloss`, then α = perceptualRoughness². This convention is not sourced here; the authoritative mapping is in X-Plane's own docs and shaders. Flag it for verification against X-Plane's normal-map alpha conventions.
- **DFG LUT:** generate once at startup in a compute shader (for example 128² or 256² RG16F) rather than shipping a PNG, which avoids sRGB and precision mistakes.

### Gaps
- Not fetched here: Lagarde & de Rousiers "Moving Frostbite to PBR" (2014) specifics, Kulla-Conty 2017, and Fdez-Agüera 2019 multiple-scattering IBL.
- Filament's exact DFG LUT resolution is not stated in the fetched text.
- Filament `cmgen` (Apache-2.0, as generally known) was not verified in this session.

---

## AO: FidelityFX CACAO vs GTAO / XeGTAO (license, cost, bent normals) and how AO is applied

### Takeaway
X-Plane names its AO pass `cacao`, so it uses AMD's ASSAO-derived CACAO. CACAO is MIT and has Vulkan/GLSL support in FidelityFX SDK 1.1.4, but AMD now marks it deprecated (SDK 1 effects). XeGTAO (MIT, HLSL, archived) is the stronger choice for quality and for bent normals at modest cost. Either way, apply AO only to indirect diffuse and use a specular-occlusion term for indirect specular.

### Cited Findings
- **CACAO 1.4:**
  - Algorithm: "a highly optimised adaptation of the Intel ASSAO".
  - Presets: Lowest to Highest, where Highest is adaptive with an importance map. Medium and above build a depth mip chain with FidelityFX SPD.
  - Resolution: native or downscaled with a final bilateral upsample.
  - Inputs: depth (normal or reverse-Z), projection and normals-to-view matrices; normals optional (derived from depth otherwise).
  - Output: single-channel AO. The SSAO intermediate is RG8 (obscurance plus edges).
  - Parameters: `EffectRadius`, `EffectShadowStrength`, `EffectShadowPow`, `EffectShadowClamp`, `InvSharpness`, `DetailAOStrength`, `EffectHorizonAngleThreshold`, fade-out, `AdaptiveSampleCountLimit`, `BlurNumPasses` (default 4; 2 at Lowest), bilateral sigmas.
  - Shaders: HLSL and GLSL (CS 6.0; GLSL needs `GL_EXT_samplerless_texture_functions` and `GL_GOOGLE_include_directive`). — [GPUOpen CACAO manual](https://gpuopen.com/manuals/fidelityfx_sdk/techniques/combined-adaptive-compute-ambient-occlusion/)
- **CACAO Vulkan support:** added in a later standalone release, v1.1 "Vulkan support!". FidelityFX SDK (MIT) 1.1.4 bundles CACAO 1.4 with a sample. — [FidelityFX-CACAO releases](https://github.com/GPUOpen-Effects/FidelityFX-CACAO/releases); [FidelityFX SDK 1.1.4 manual](https://gpuopen.com/manuals/fidelityfx_sdk/)
- **SDK status:**
  - The current GPUOpen SDK repo is now the "AMD FSR SDK" (v2.3.0 "Redstone"). Its release notes for 2.1.0 / 2.3.0 say "All SDK version 1 effects are now deprecated" and point to FidelityFX SDK 1.1.4 for legacy effects.
  - SDK 1.1.0-1.1.4 list Vulkan SDK 1.3.250 as a prerequisite; 2.x lists only VS2022 and the Windows SDK. — [FidelityFX-SDK releases](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/releases)
- **XeGTAO:**
  - MIT (Intel 2021); HLSL plus a C++ header (`XeGTAO.h` / `XeGTAO.hlsli`), DX12 SM 6.3. Archived: Intel has stopped development.
  - Cost (High preset): about 1.4 ms at 4K on an RTX 3070, about 0.56 ms at 1080p on an RTX 2060, about 2.39 ms at 1080p on an i7-1195G7 iGPU. Medium is about 2/3 of High.
  - Bent normals: optional since v1.30, adding about 25% cost, packed with AO into RGBA8 (RGB = bent normal, A = AO).
  - Denoiser: 5x5 depth-aware spatial filter, with TAA for temporal accumulation.
  - Inputs: depth (normals optional, with a built-in depth-to-normal generator).
  - Based on Jimenez et al. 2016, "Practical Realtime Strategies for Accurate Indirect Occlusion" (GTAO).
  - Its sample applies AO to diffuse and specular probe lighting using GTAO's multi-bounce and GTSO, plus micro-shadowing on direct light. The authors call this usage "ad hoc". — [GitHub GameTechDev/XeGTAO](https://github.com/GameTechDev/XeGTAO)
- **Applying AO to specular:** the Lagarde specular-occlusion formula and horizon occlusion are given in the PBR section above. — [Filament docs](https://google.github.io/filament/Filament.md.html)

### Inferences
- **Choice for this editor:**
  - CACAO matches X-Plane's look and is MIT with GLSL available, the most direct Vulkan port.
  - XeGTAO gives better-grounded AO plus bent normals (useful for specular occlusion and better ambient lighting) and is easy to port from HLSL to Slang.
  - Both are MIT, so either is safe for a FOSS editor.
- **Slang port notes:**
  - CACAO's de-interleaved 2x2 passes and SPD depth mips map cleanly onto Vulkan compute.
  - XeGTAO uses SM 6.x features (16-bit types optional). Slang supports `half` / `min16float` and wave operations, but check that the `R16_SFLOAT` view-space depth path matches the built-in normal generator; the XeGTAO README says they are incompatible.

### Gaps
- No public GPU timings were extracted for CACAO; that part of the manual page was not reached.
- I did not compare CACAO with XeGTAO quality from a single source.

---

## SSR: FidelityFX SSSR / stochastic SSR, Hi-Z tracing, cube map fallback, water reflections

### Takeaway
FidelityFX SSSR 1.5 (MIT, HLSL and GLSL/Vulkan, in SDK 1.1.4) is the canonical open reference. It covers:
- a SPD-built 7-mip Hi-Z hierarchy;
- 8x8 tile classification with a roughness threshold;
- blue-noise jittered GGX rays;
- a spatio-temporal denoiser;
- an environment cube map plus BRDF LUT fallback.

X-Plane's split into `ssr_deferred` and `ssr_mesh` suggests one SSR pass for the G-buffer and another for forward-shaded meshes such as water or glass.

### Cited Findings
- **SSSR 1.5 pipeline:**
  1. SPD depth hierarchy (7 mips).
  2. Tile classification: rough pixels skip tracing and use the environment map; outputs a ray list and 8x8 denoise tiles.
  3. Per-frame 128x128 blue noise from a 256x256 Sobol buffer.
  4. Indirect arguments.
  5. Hierarchical intersection.
  6. Denoise.
- **Inputs:** HDR colour (direct lighting only), depth (`FFX_SSSR_ENABLE_DEPTH_INVERTED` for reverse-Z), normals, roughness (perceptual-squared by default, configurable), motion vectors, environment cube map, BRDF LUT, R16F variance history.
- **Sampling:** traversal runs full rate for mirrors and quarter rate for glossy. `samplesPerQuad` sets the minimum rays per 2x2 quad, and variance-guided tracing raises it up to 4 rays per quad. The cut-off is `roughnessThreshold`.
- **API requirements:** DX12 CS 6.2 (6.6 for wave64); Vulkan GLSL 4.50 with subgroup basic/ballot/shuffle. — [GPUOpen SSSR manual](https://gpuopen.com/manuals/fidelityfx_sdk/techniques/stochastic-screen-space-reflections/)
- **Wet-world reflections:** Lagarde's "Water drop 4a – Reflecting wet world" covers reflection approaches for wet scenes; content not fetched. — [Lagarde 4a](https://seblagarde.wordpress.com/2013/09/08/water-drop-4a-reflecting-wet-world/)
- **Water roughness:** in Lagarde's model, puddles and flooded surfaces go to gloss 1, specular 0.02 (IOR 1.33) and an up-facing normal, which are the near-mirror inputs that SSR must handle. — [Lagarde 2b](https://seblagarde.wordpress.com/2013/01/03/water-drop-2b-dynamic-rain-and-its-effects/)

### Inferences
- **Two SSR passes:** `ssr_mesh` likely traces per-pixel SSR from forward-shaded water, glass and wet surfaces using the opaque Hi-Z, while `ssr_deferred` handles G-buffer surfaces.
- **Fallback chain:** SSR, then the per-frame `cube_filter_compute` probe, then sky. For an ocean, also fade SSR at grazing angles and screen edges and blend toward the sky/cube (planar reflections are too costly at sim scale).
- **SPD reuse:** CACAO, SSSR and (optionally) luminance mips all use FidelityFX SPD. Porting SPD once to Slang (it's HLSL/GLSL, wave-intrinsic based) unlocks all three.

### Gaps
- SSSR GPU timings were not in the fetched portion of the manual.
- The denoiser internals (reprojection, prefilter, temporal) were not captured.
- I did not research hybrid or ray-traced reflections, since they are out of scope for a raster editor.

---

## Shadows: stable cascades, PCF/PCSS, cascade blending, cloud shadow maps, contact shadows

### Takeaway
Matt Pettineo's "A Sampling of Shadow Techniques" and its `TheRealMJP/Shadows` sample cover the core practice:
- stable cascades (Valient's sphere fit plus texel snapping);
- GatherCmp-optimized PCF;
- EVSM / MSM;
- normal-offset and receiver-plane bias;
- cascade blending;
- SDSM depth bounds.

For clouds, a top-down Beer shadow map (Hillaire) or transmittance map works. Contact shadows come from Bend Studio's screen-space shadows, whose example code is public; license unverified.

### Cited Findings
- **Stable cascades:** Valient's ShaderX6 method fits a bounding sphere per split so the projection does not change with camera rotation, and snaps the projection to texel increments. The trade-off is lower effective resolution, and FOV or near/far changes can cause issues. — [MJP, A Sampling of Shadow Techniques](https://therealmjp.github.io/posts/shadow-maps/)
- **Cascade selection:** the sample uses one global shadow matrix plus a per-cascade scale/offset. Partitioning can be logarithmic, PSSM (λ) or manual. "Auto-Compute Depth Bounds" uses GPU min/max depth (SDSM). "Filter Across Cascades" blends at boundaries. — [MJP](https://therealmjp.github.io/posts/shadow-maps/)
- **Filtering cost (AMD 7950, 1080p):**
  - OptimizedPCF (GatherCmp, from Ignacio Castaño / The Witness): going from 2x2 to 7x7 costs about +0.4 ms, against +2-3 ms for grid sampling.
  - EVSM4 (4×FP32 per texel): about +11.5 ms over 7x7 PCF at 2048² cascades with 4xMSAA, mips and 8xAF; about +3 ms at 1024² with 4xMSAA.
  - VSM suffers light bleeding. — [MJP](https://therealmjp.github.io/posts/shadow-maps/)
- **Bias:**
  - Manual depth bias differs in world size per cascade.
  - Normal offset is scaled by the cascade depth range; suggested starting point is bias ≈ 0.0015 and offset scale 1-2.
  - Receiver plane bias uses screen derivatives but can fail in degenerate cases. — [MJP](https://therealmjp.github.io/posts/shadow-maps/)
- **Sample code:** `https://github.com/TheRealMJP/Shadows`. The article states no license. — [MJP](https://therealmjp.github.io/posts/shadow-maps/)
- **Bend Studio screen-space shadows:** Days Gone (PS4, 2019) was the first title to use them for sun shadows, and other Sony titles followed. Bend publishes the example code and a SIGGRAPH 2023 presentation as downloads. — [Inside Bend: Screen Space Shadows](https://www.bendstudio.com/blog/inside-bend-screen-space-shadows/)
  - Community ports exist (UE5 write-up; "Open Shaders" uses the Bend SSS algorithm). — [winter-crown-works](https://winter-crown-works.com/en/tech/003); [open-shaders deepwiki](https://deepwiki.com/alandtse/open-shaders/4.2-shadow-features)
- **Cloud and volumetric shadows:**
  - Hillaire's Beer Shadow Map stores front depth and mean extinction (R11G11B10f), avoiding ESM's convergence to zero transmittance over long distances. — [Hillaire 2020 slides](https://blog.selfshadow.com/publications/s2020-shading-course/hillaire/s2020_pbs_hillaire_slides.pdf)
  - Company of Heroes 2 uses a top-down "snow shadow" map so only topmost surfaces get snow, the same top-down idea as a cloud shadow or coverage map. — [CoH2 rendering tech slides](https://www.slideshare.net/proyZ/daniel-barrero-coh2renderingtech)

### Inferences
- **`shadow_rect`:** likely renders shadow cascades or tiles as rectangles inside an atlas (one shadow texture with sub-rects per cascade or spot light). This is a guess from the name.
- **Recommended stack:**
  - 4 stable cascades (sphere-fit, texel-snapped) in a depth array or atlas, with reverse-Z and D32F.
  - 5x5 / 7x7 GatherCmp PCF, normal-offset bias, cross-cascade dithered blend.
  - PCSS (blocker search, then variable-kernel PCF) only for close cascades.
  - Cloud shadows: sample a top-down cloud transmittance (or Beer) map in world XZ and multiply it into sun visibility.
  - Contact shadows: Bend SSS for the sun.
- **Slang port:** use `SamplerComparisonState` + `GatherCmp` (Slang maps it to `OpImageDrefGather`). Bend SSS is a compute pass dispatched in wavefronts outward from the light's screen-space projection (as documented in its presentation). Verify its license before vendoring.

### Gaps
- The Bend SSS license and its parameter list were not readable; the blog doesn't state them, and the archive was not downloaded.
- The license of MJP's `Shadows` repo was not verified.
- I did not fetch an authoritative PCSS reference (Fernando 2005, NVIDIA).

---

## Fog: froxel volumetric fog (Wronski 2014, Hillaire 2015) and how it coexists with aerial perspective

### Takeaway
Froxel fog voxelizes participating media and lights into a low-resolution camera-frustum 3D texture, integrates front-to-back into a scatter/transmittance volume, and applies it as a lookup while shading. Hillaire 2020's aerial-perspective volume uses exactly that froxel mapping (and cites Wronski 14 / Hillaire 16), so the two share a parameterization. Composite them as two transmittance-multiplied in-scatter terms, or fold atmosphere in-scatter into the fog volume.

### Cited Findings
- **Wronski GDC 2014 / SIGGRAPH 2014:**
  - Talks: "Assassin's Creed IV: Black Flag – Road to Next-Gen Graphics" (GDC 2014) and "Volumetric Fog: Unified Compute Shader Based Solution to Atmospheric Scattering" (SIGGRAPH 2014 Advances).
  - Method: compute shaders accumulate density and in-scattered light into small 3D textures, then build a lookup applied during shading.
  - Reported fixed cost: about 1.1 ms on PS4 and Xbox One. — [Wronski GDC 2014 post](https://bartwronski.com/2014/03/09/my-upcoming-gdc-2014-presentation/); [AC4 GDC slides](https://bartwronski.com/wp-content/uploads/2014/03/ac4_gdc.pdf)
- **Hillaire 2015, "Physically Based and Unified Volumetric Rendering in Frostbite" (SIGGRAPH 2015 Advances):**
  - A cascaded volume for extinction.
  - Particle voxelization into that volume.
  - Volumetric shadow maps letting any light cast volumetric shadows.
  - Planned as the framework for future Frostbite games. — [EA/Frostbite page](https://www.ea.com/news/physically-based-unified-volumetric-rendering-in-frostbite); [SIGGRAPH 2015 Advances course](https://advances.realtimerendering.com/s2015/index.html)
- **Aerial perspective:** Hillaire 2020 stores it in a frustum-mapped froxel volume (RGB luminance, A transmittance) applied to opaque and translucent surfaces. — [Hillaire 2020 slides](https://blog.selfshadow.com/publications/s2020-shading-course/hillaire/s2020_pbs_hillaire_slides.pdf)
- **Rain fog:** Lagarde's rain implementation raises fog density with rain intensity and desaturates fog colour over time, with presets fog 0.15 / 0.4 / 1.0 for light / moderate / heavy rain. — [Lagarde 2b](https://seblagarde.wordpress.com/2013/01/03/water-drop-2b-dynamic-rain-and-its-effects/)

### Inferences
- **X-Plane mapping:** `volumetric_fog` probably fills and integrates the froxel grid (local fog, rain shafts, light scattering), and `volumetric_apply` composites it, matching Wronski's split.
- **Coexistence with aerial perspective:**
  - Option A: keep the atmosphere AP volume (tens of km, coarse) and the local fog volume (hundreds of metres to a few km, finer near-camera slices) separate. Apply `C = C·T_fog·T_ap + S_fog + S_ap·T_fog`, ordered correctly; fog is nearer the camera.
  - Option B: inject atmosphere extinction and in-scatter into the near fog froxels and start the AP integration where the fog volume ends.
  - Both rely on the shared froxel parameterization.
- **Typical froxel sizes:** about 160x90x64 or 128 slices with exponential depth distribution, RGBA16F (in-scatter RGB plus extinction or transmittance), with temporal reprojection to hide jitter. This is common practice from memory; see Gaps.
- **Weather tie-in:** METAR visibility or haze can set the near-ground fog extinction (see the Koschmieder note in the Atmosphere section). Rain intensity adds extinction and desaturation (Lagarde).

### Gaps
- I could not verify Wronski's exact froxel resolution (often quoted as 160x90x64) or Hillaire 2015's froxel sizes and cost; the slide decks were not fetched (AC4 slides PDF, about 158 MB for the 2015 deck).
- I did not identify a canonical permissively licensed froxel-fog repo. Godot 4's volumetric fog (MIT engine) is a candidate reference but was not verified here.

---

## Weather surface effects: Lagarde's "Water drop" series (wet surfaces, puddles, ripples, streaks) and snow accumulation; seasonal foliage

### Takeaway
Lagarde's series gives concrete, portable formulas:
- wet darkening via porosity: `lerp(1, 0.2, porosity)`;
- gloss pushed toward water;
- puddle and flood levels from heightmaps and vertex colour, with water F0 0.02 and up normals;
- a 4-layer procedural ripple normal map with a specified RGBA layout;
- rain-intensity presets.

Snow accumulation in shipped games blends by the world-up component of the normal, masked by height/noise and a top-down occlusion map, with RNM normal blending. Together these map closely to X-Plane's `rain_surface`, `ripple`, `rain_normals`, `rain_ice_accumulation` and `weather_apply_*`.

### Cited Findings
- **Series index (Lagarde):** 1 "Observe rainy world", 2a/2b "Dynamic rain and its effects", 3a/3b "Physically based wet surfaces", 4a/4b "Reflecting wet world". — URLs in [Lagarde 3b](https://seblagarde.wordpress.com/2013/04/14/water-drop-3b-physically-based-wet-surfaces/)
- **Physically based wet surfaces (3b):**
  - Diffuse: `factor = lerp(1, 0.2, Porosity); Diffuse *= lerp(1, factor, WetLevel)`, with porosity multiplied by (1 - metalness).
  - Porosity without a texture: `saturate(-2.5*Gloss + 1.25)`.
  - Gloss: `Gloss = lerp(1, Gloss, lerp(1, factor, 0.5*WetLevel))`, where the 0.5 is empirical.
  - Water F0 (0.02) is noted but left unmodelled in that version.
  - `AccumulatedWater` drives a thin layer (normal smoothed, specular boosted) and puddles (flat normal, full water properties). Normals blend toward vertex normals as water deepens.
  - Drying takes about a minute (sigmoid or lerp) with a noise mask; puddles dry slower. — [Lagarde 3b](https://seblagarde.wordpress.com/2013/04/14/water-drop-3b-physically-based-wet-surfaces/)
- **Dynamic rain (2b): ripples:**
  - One tiling procedural ripple texture. R = inverse normalized distance from the circle centre, GB = direction (×2-1), A = random per-circle time offset.
  - Animation:

    ```
    DropFrac   = frac(A + t)
    TimeFrac   = DropFrac - 1 + R
    DropFactor = saturate(0.2 + Weight*0.8 - DropFrac)
    Final      = DropFactor * R * sin(clamp(TimeFrac*9, 0, 3) * PI)
    NormalXY   = GB * Final * 0.35
    ```
  - Four time-shifted layers are weighted by `saturate((RainIntensity - (0, .25, .5, .75)) * 4)`.
  - Application: `N = lerp(N, WaterNormal, AccumulatedWater * VertexColor.r)`.
  - Mipmapping the ripple texture made ripples vanish. — [Lagarde 2b](https://seblagarde.wordpress.com/2013/01/03/water-drop-2b-dynamic-rain-and-its-effects/)
- **Dynamic rain (2b): wetness and flooding:**
  - Wet: `Diffuse *= lerp(1, 0.3, WetLevel); Gloss = min(Gloss * lerp(1, 2.5, WetLevel), 1)`.
  - Accumulated water = max of `min(FloodLevel.x, 1 - Heightmap)` (holes) and `saturate((FloodLevel.y - VertexColor.g)/0.4)` (puddles).
  - Underwater blend: gloss to 1, specular to 0.02, normal to up.
  - Rain-intensity presets:

| Rain | Intensity | Splashes | Puddle flood inc/dec | Heightmap flood inc/dec | Wet inc/dec | Fog |
|---|---|---|---|---|---|---|
| Light | 0.33 | 20 | 0.1 / 0.05 | 0.2 / 0.1 | 0.6 / 0.2 | 0.15 |
| Moderate | 0.66 | 40 | 0.3 / 0.05 | 0.5 / 0.1 | 0.75 / 0.2 | 0.4 |
| Heavy | 1.0 | 60 | 0.7 / 0.05 | 0.85 / 0.1 | 1.0 / 0.2 | 1.0 |

  - Splash exposure uses a depth map rendered from the rain direction (like a shadow map). Vertical and occluded surfaces stay dry.
  - Splash cost about 0.25-0.33 ms on X360/PS3; camera droplets about 0.32 ms (PS3) / 0.54 ms (X360).
  - Water streaming down walls is deliberately not modelled. — [Lagarde 2b](https://seblagarde.wordpress.com/2013/01/03/water-drop-2b-dynamic-rain-and-its-effects/)
- **Snow:**
  - Company of Heroes 2 blends object material with snow by the normal's up component ("fast and looks good") and uses a top-down snow-shadow map so only topmost surfaces receive snow. — [CoH2 rendering tech](https://www.slideshare.net/proyZ/daniel-barrero-coh2renderingtech)
  - Batman: Arkham Origins (GDC 2014) blends snow stages with a heightmap and relief mapping, and blends normals with Reoriented Normal Mapping ("normals are not colors"). — [GDC 2014 deformable snow](https://www.slideshare.net/colinbb/gdc2014-deformable-snow-rendering-in-batman-arkham-origins)
  - A UE4 shader guide uses a world-up mask, coverage multiplier, vertex-paint control, a contrast step and a breakup noise mask. — [80.lv Shader Guide for UE4](https://80.lv/articles/shader-guide-for-ue4)

### Inferences
- **X-Plane mapping (from names):**
  - `rain_surface`: wet-level and flood-level state, probably a world-space or top-down texture updated over time.
  - `ripple`: generates the animated ripple normal texture (Lagarde-style procedural circles).
  - `rain_normals`: blends ripple and flow normals into the G-buffer.
  - `rain_ice_accumulation`: integrates wetness, ice and snow coverage over time, likely a persistent texture driven by precipitation and temperature.
  - `weather_apply_*`: modifies G-buffer albedo, roughness and normal in place (a deferred "weather decal" pass).
- **Implementation for the editor:**
  1. A persistent top-down R16G16 "surface water / snow" texture around the camera, driven by `wet`, `flood` and `snow` rates per Lagarde's table.
  2. A top-down occlusion depth (rain / snow shadow) to keep covered or vertical areas dry.
  3. A compute pass modifying the G-buffer: porosity darkening, gloss boost, puddle flattening with F0 0.02, ripple normals masked by `AccumulatedWater`.
  4. Snow coverage = `saturate((N.y - threshold) * contrast) * noise * accumulation * topDownVisible`, blended into albedo and roughness, with RNM for normals.
- **Ice:** a thin clear-coat-like layer (higher gloss, F0 about 0.018-0.02) over darkened albedo is a reasonable approximation. This is not sourced.
- **Seasonal foliage:** no sources found. A practical approach is a per-region season parameter that shifts albedo hue/value in a LUT or swaps texture sets, with snow coverage applied to canopy tops.

### Gaps
- I found no sources for seasonal foliage tinting or swapping in shipped real-time renderers.
- The Horizon Zero Dawn / Forbidden West, Assassin's Creed and Frostbite snow-accumulation talks were not located or verified.
- I found no sourced ice-accumulation technique.
- Lagarde parts 3a and 4a/4b were not fetched.
- Lagarde's blog code snippets carry no explicit license; they are formulas, generally re-implementable.

---

## Ocean: Tessendorf FFT, JONSWAP/Phillips/TMA spectra, cascades, foam, and "ocean variance" / LEAN-style specular filtering

### Takeaway
X-Plane's `ocean_spectrum` → `compute_fft` → `ocean_variance` maps to:
1. A Tessendorf FFT ocean with an empirical spectrum (JONSWAP/TMA with directional spreading per Horvath 2015).
2. A GPU FFT (Stockham).
3. A slope-variance computation used to filter specular at distance, as in Bruneton, Neyret & Holzschuch 2010's "seamless transitions from geometry to BRDF", which avoids sparkle aliasing.

GodotOceanWaves (MIT) is the most complete permissive reference for steps 1 and 2.

### Cited Findings
- **Tessendorf, "Simulating Ocean Water" (SIGGRAPH course notes):** FFT synthesis of height fields from a statistical spectrum. Foam appears where the Jacobian of the horizontal displacement goes negative (crests curling over). — [Tessendorf 2001 slides](https://jtessen.people.clemson.edu/reports/papers_files/waterslides2001.pdf); foam criterion summarized in [GodotOceanWaves](https://github.com/2Retr0/GodotOceanWaves)
- **Horvath 2015, "Empirical directional wave spectra for computer graphics" (DigiPro):** uses the TMA spectrum (JONSWAP with finite-depth correction), compares empirical directional spreading functions, and adds a normalized "swell" parameter that elongates long waves into parallel trains. — [paper DOI via Unpaywall](https://unpaywall.org/10.1145%2F2791261.2791267)
- **TMA and JONSWAP:** TMA is JONSWAP with a Kitaigorodskii depth correction and reduces to JONSWAP in deep water; the usual JONSWAP peak enhancement γ is 3.3. — [wavespectra docs](https://wavespectra.readthedocs.io/en/v4.8.0/construction.html); [SWAN manual](https://swanmodel.sourceforge.io/online_doc/swanuse/node27.html)
- **GodotOceanWaves (MIT):**
  - Spectrum: TMA from wind speed, depth and fetch, with small-wave suppression.
  - Spreading: flat-to-Hasselmann blend plus swell.
  - Cascades: multiple, sharing one spectrum resolution, with optional one-cascade-per-frame updates.
  - FFT: Stockham with precomputed butterfly, row pass, transpose, column pass.
  - Foam: Jacobian-based, accumulating with exponential decay (grow and decay rates).
  - Lighting: microfacet BSDF after the Atlas GDC 2019 talk, using GGX instead of Beckmann.
  - Filtering: bicubic/bilinear blend by pixel density.
  - References: Tessendorf, Horvath 2015, Flügge 2017, Mihelich GDC 2019, Gunnell 2023, Pensionerov FFT-Ocean 2020. — [GitHub 2Retr0/GodotOceanWaves](https://github.com/2Retr0/GodotOceanWaves)
- **Other repos:**
  - `gasgiant/FFT-Ocean`: MIT, Unity, prototype; points to the newer Ocean-URP. — [GitHub gasgiant/FFT-Ocean](https://github.com/gasgiant/FFT-Ocean)
  - `deiss/fftocean`: small C++ Tessendorf/Phillips implementation (OpenGL/GLUT). — [GitHub deiss/fftocean](https://github.com/deiss/fftocean)
- **Bruneton, Neyret, Holzschuch 2010, "Real-time Realistic Ocean Lighting using Seamless Transitions from Geometry to BRDF" (CGF 29(2)):**
  - Models sub-pixel waves as a Gaussian slope distribution with computed variance, used in a microfacet BRDF so distant ocean shading stays consistent with near geometry. — [INRIA project page](https://evasion.inrialpes.fr/Publications/2010/BNH10); [course slides](https://cgg.mff.cuni.cz/%7Ejaroslav/teaching/2012-npgr031/prezentace/Realtime%20Realistic%20Ocean%20Lighting.pdf)
  - Its source ships within INRIA's Proland ocean examples. — [Proland ocean docs](https://proland.inrialpes.fr/doc/proland-4.0/ocean/html/page-examples.html)
- **Arc Blanc (arXiv 2025):** a real-time ocean framework built on Tessendorf IFFT height and derivative expressions. — [arXiv 2503.03326](https://arxiv.org/pdf/2503.03326)

### Inferences
- **`ocean_variance`:** most likely computes per-texel or per-mip slope variance (σx², σy², and possibly σxy) of the FFT normal field. Shading then widens GGX roughness with distance and mip, using `α² ≈ α_base² + 2σ²`, a LEAN/Toksvig-style relation. This is the Bruneton 2010 / LEAN-mapping idea; the exact X-Plane formula is unknown.
- **Recommended Vulkan/Slang layout:**
  - 3-4 cascades (for example 256² each, patch sizes about 250 m / 50 m / 10 m) using RGBA16F displacement (xyz) plus a derivatives texture (∂h/∂x, ∂h/∂z, ∂Dx/∂x, ∂Dz/∂z) for normals and the Jacobian.
  - Storage images in a 2D array.
  - A Stockham FFT in one compute dispatch per axis using `groupshared`.
  - Mipmapped normals/derivatives with a variance channel (store E[s] and E[s²] so variance survives mipmapping, the LEAN idea).
- **Spectrum parameters to expose:** wind speed and direction, fetch, depth (TMA), γ (3.3), spreading / swell, choppiness (λ), foam threshold and decay, cascade sizes.
- **Licensing:** GodotOceanWaves (MIT) is safe to port. Proland's license was not checked.

### Gaps
- I found no standalone permissively licensed C++/Vulkan FFT ocean repo with JONSWAP, cascades and foam together.
- The Proland license and Bruneton 2010 code availability (BSD?) were not verified.
- LEAN mapping (Olano & Baker 2010) was not fetched.
- The Atlas GDC 2019 (Mihelich) and Gunnell 2023 references were not fetched.
- X-Plane's actual spectrum (Phillips vs JONSWAP) is unknown.

---

## Remaining passes: volumetric clouds and cloud shadows, FSR, tiled light culling, bloom, depth of field, sparse virtual texturing

### Takeaway
- Clouds: Guerrilla's Nubis talks (Schneider 2015 / 2017 / 2022 / 2023), with Vulkan student ports available.
- FSR: FSR 2.2.1 (MIT, HLSL, ships a Vulkan backend) is the most practical open upscaler. FSR 3.1's API uses an AMD-signed prebuilt DLL, and the current FSR SDK 2.x / FSR 4 releases list Vulkan as unsupported.
- Tiled lights: Forward+ (Harada 2012) or clustered (Olsson 2012).
- Bloom and DoF: Jimenez 2014 (CoD: AW).
- SVT: Barrett 2008 and van Waveren 2012.

### Cited Findings
- **Volumetric clouds:**
  - Guerrilla publishes the Nubis talks ("Nubis Evolved" SIGGRAPH 2022, "Nubis Cubed" 2023). — [Nubis Evolved](https://www.guerrilla-games.com/read/nubis-evolved); [Nubis Cubed](https://www.guerrilla-games.com/read/nubis-cubed); [SIGGRAPH 2022 slides](https://advances.realtimerendering.com/s2022/SIGGRAPH2022-Advances-NubisEvolved-NoVideos.pdf)
  - `AmanSachan1/Meteoros` (UPenn CIS 565) is a Vulkan Nubis-style cloudscape renderer, under 3 ms at 1080p on a notebook GTX 1070. — [GitHub Meteoros](https://github.com/AmanSachan1/Meteoros)
  - `YueZhang1027/CIS5650-Final-Project-Frostnova` is a Vulkan implementation of Nubis3 (voxel-based). — [GitHub Frostnova](https://github.com/YueZhang1027/CIS5650-Final-Project-Frostnova)
- **Cloud update strategies (from Hillaire's survey slide):** Schneider 15/17 update 1 pixel per 4x4 tile (16-frame latency, looks full-res). Bauer 19 (RDR2) traces at quarter resolution with TAA upsampling (instant, but blurrier). — [Hillaire 2020 slides](https://blog.selfshadow.com/publications/s2020-shading-course/hillaire/s2020_pbs_hillaire_slides.pdf)
- **FSR 2:**
  - `GPUOpen-Effects/FidelityFX-FSR2` v2.2.1: MIT-style license, DX12 and Vulkan backend libraries, HLSL (CS 6.2 / 6.6), with custom backends allowed.
  - Inputs: colour, depth, motion vectors, optional reactive mask (R8), optional exposure (1x1 R32F), transparency/composition mask.
  - Modes:

| Mode | Scale | Jitter phases | Mip bias |
|---|---|---|---|
| Quality | 1.5x | 18 | -1.58 |
| Balanced | 1.7x | 23 | -1.76 |
| Performance | 2.0x | 32 | -2.0 |
| Ultra Performance | 3.0x | 72 | -2.58 |

  - Cost: 4K Quality about 0.7 ms (RX 7900 XTX) to 5.4 ms (RX 590). — [GitHub FidelityFX-FSR2](https://github.com/GPUOpen-Effects/FidelityFX-FSR2)
- **FSR 3.1:** FidelityFX SDK 1.1 (July 2024) added FSR 3.1 with native DX12 and Vulkan backends. — [GamingOnLinux](https://gamingonlinux.com/2024/07/amd-releases-fidelityfx-sdk-110-with-fsr-31-and-vulkan-support)
  - AMD's integration overview says the new API talks to a single prebuilt, AMD-signed DLL, and custom backends are no longer supported through it. — [AMD FSR 3.1 overview](https://gpuopen.com/presentations/2024/FidelityFX_Super_Resolution_3-1_Release-Overview_and_Integration.pdf)
- **FSR SDK 2.x / FSR 4:**
  - Latest is AMD FSR SDK 2.3.0 "Redstone". Component versions: FSR Upscaler 3.1.5, ML upscaler (FSR 4) 4.1.1, Frame Generation 3.1.6 and ML 4.0.1, Ray Regeneration 1.2.0, Radiance Caching (preview).
  - The known-issues table says Vulkan "is currently not supported in SDK". — [GitHub FidelityFX-SDK](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK)
  - Release history: 2.0.0 (20 Aug) shipped FSR 4.0.2; 2.3.0 was released 24 Jun; SDK 1 effects are deprecated. — [FidelityFX-SDK releases](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/releases)
- **Tiled and clustered light culling:**
  - Harada, McKee, Yang 2012, "Forward+": compute-shader light culling produces per-tile light lists read by forward shading. — [EG DL Forward+](https://diglib.eg.org/items/1db2c4c6-dcab-42ea-8c0a-6805d781759e/full)
  - Olsson, Billeter, Assarsson 2012, "Clustered Deferred and Forward Shading" (HPG): 3D clusters handle up to about a million lights. — [Chalmers](https://research.chalmers.se/en/publication/161725)
  - The Vulkan tutorial has a Forward+ chapter (compute pass writes compact per-tile lists to SSBOs). — [Vulkan tutorial Forward+](https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Advanced_Topics/ForwardPlus_Rendering.html)
  - Wicked Engine's write-up covers tile depth min/max via atomics and culling optimizations. — [Wicked Engine](https://wickedengine.net/2018/01/optimizing-tile-based-light-culling)
- **Bloom and DoF (Jimenez 2014, "Next Generation Post Processing in Call of Duty: Advanced Warfare", SIGGRAPH 2014):**
  - Bloom uses a pyramidal filter hierarchy for temporal stability. — [iryoku.com](https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/)
  - The widely used 13-tap downsample is attributed to this talk by community implementations (for example FlightGear's `bloom_downsample.frag`, which is secondary). — [FlightGear bloom_downsample](https://git.nayala.duckdns.org/fly/fgdata/src/branch/next/Shaders/HDR/bloom_downsample.frag)
  - DoF: a secondary summary says the team rejected separable hex/octagon kernels for a 49-tap circular kernel with foreground/background separation. — [scrapbox note](https://scrapbox.io/0b5vr/Next_Generation_Post_Processing_in_Call_of_Duty:_Advanced_Warfare)
- **Sparse virtual texturing:**
  - Most implementations derive from Sean Barrett's GDC 2008 talk.
  - Structure: an indirection (page-table) texture lookup followed by a filtered lookup into a physical tile cache. A feedback pre-pass writes the needed tile and mip, read back by the CPU.
  - References: [Gaia Sky virtual textures docs](https://gaia.ari.uni-heidelberg.de/gaiasky/docs/3.7.0/Virtual-textures.html); [TU Wien thesis (Mayer 2010)](https://www.cg.tuwien.ac.at/research/publications/2010/MAYER-2010-MTX/MAYER-2010-MTX-doc.pdf); [van Waveren, "Virtual Texturing in Software and Hardware", SIGGRAPH 2012](https://www.quaddicted.com/webarchive/mrelusive.com_20170131/publications/presentations/2012_siggraph/Virtual_Texturing_in_Software_and_Hardware_final.pdf); [Sagristà 2023 blog](https://tonisagrista.com/blog/2023/sparse-virtual-textures/)

### Inferences
- **X-Plane mapping:**
  - `light_tile` + `light_spill_compact`: tiled culling of X-Plane's many "spill" lights (runway, airport and city lights), with a compaction pass that builds dense visible-light lists. This fits a Forward+/clustered design.
  - `svt`: probably streams orthophoto or terrain textures through a page table plus feedback buffer.
  - `fsr`: likely FSR 1 (spatial EASU/RCAS) or FSR 2. The name alone doesn't distinguish them.
- **Upscaler for a FOSS Vulkan editor:** vendor FSR 2.2.1 (MIT, source, Vulkan backend). It is HLSL and could compile through Slang or DXC to SPIR-V, but FSR2's own Vulkan backend uses precompiled permutations. Avoid the FSR 3.1+ DLL path (signed binary, Windows-centric) and FSR 4 (Vulkan unsupported per the current SDK page).
- **SVT on Vulkan 1.3:** prefer a software page table (indirection texture plus physical atlas) over `sparseResidencyImage2D`. Sparse residency support and performance vary by vendor; this is general knowledge, unverified here.
- **Bloom:** the 13-tap downsample plus 3x3 tent upsample chain (about 6-8 mips, R11G11B10F) is the de facto standard and sits right after `exposure_fusion` / exposure. Apply bloom in linear HDR before tonemapping.

### Gaps
- I did not verify FSR 1 (EASU/RCAS) availability and license in SDK 1.1.4, or FidelityFX DOF / Blur / LPM.
- The Nubis cloud shadow-map method and the licenses of the student cloud repos were not checked.
- The year of SDK v2.3.0 is not shown on the releases page; June 2026 is inferred from the 2022-2026 copyright.
- I did not fetch exact primary-source bloom tap weights or DoF kernel details from the Jimenez slides.

---

## For each feature: concrete reference repos (URL, license, language) suitable for porting to Slang/Vulkan

### Takeaway
Most of X-Plane 12's techniques have permissively licensed references: MIT (Hillaire sky, XeGTAO, FidelityFX 1.1.4 effects, FSR2, GodotOceanWaves), BSD (Bruneton) or Apache-2.0 (Khronos IBL Sampler, Khronos PBR Neutral). Several key references state no license (Bend SSS, MJP Shadows, pl-sky, bruop) and need checking before vendoring. Use them as reading references until verified.

### Cited Findings

| Feature (X-Plane pass) | Reference | License | Language / API | Source |
|---|---|---|---|---|
| Sky/atmosphere LUTs (`scatter_*`) | sebh/UnrealEngineSkyAtmosphere | MIT | HLSL + C++ (DX11) | [repo](https://github.com/sebh/UnrealEngineSkyAtmosphere) |
| Precomputed scattering (accuracy reference) | ebruneton/precomputed_atmospheric_scattering | BSD | C++ / GLSL / WebGL | [repo](https://github.com/ebruneton/precomputed_atmospheric_scattering) |
| Hillaire 2020 on Vulkan toolchain | hoffstadt/pl-sky | not stated | C/C++, needs Vulkan SDK | [repo](https://github.com/hoffstadt/pl-sky) |
| Local tone mapping (`exposure_fusion`) | Wronski 2022 demo | public domain | JS / WebGL | [post](https://bartwronski.com/2022/02/28/exposure-fusion-local-tonemapping-for-real-time-rendering/) |
| Histogram exposure (`lumi_histo`) | bruop (bgfx) | not stated | C++ / bgfx shaders | [post](https://bruop.github.io/exposure/) |
| Neutral tone curve | KhronosGroup/ToneMapping (PBR Neutral) | Apache-2.0 | GLSL sample | [repo](https://github.com/KhronosGroup/ToneMapping) |
| AgX curve | MrLixm/AgXc | not verified | multi-language ports | [repo](https://www.github.com/MrLixm/AgXc) |
| IBL prefilter (`cube_filter_compute`) | KhronosGroup/glTF-IBL-Sampler | Apache-2.0 | C++ / GLSL, Vulkan | [repo](https://github.com/KhronosGroup/glTF-IBL-Sampler) |
| BRDF / DFG / specular occlusion math | Filament docs | (Apache-2.0 engine, not verified here) | GLSL | [docs](https://google.github.io/filament/Filament.md.html) |
| AO (`cacao`) | FidelityFX CACAO 1.4 (SDK 1.1.4) | MIT | HLSL + GLSL, DX12 / Vulkan | [manual](https://gpuopen.com/manuals/fidelityfx_sdk/techniques/combined-adaptive-compute-ambient-occlusion/), [releases](https://github.com/GPUOpen-Effects/FidelityFX-CACAO/releases) |
| AO alternative (+ bent normals) | GameTechDev/XeGTAO | MIT (archived) | HLSL + C++ header, DX12 | [repo](https://github.com/GameTechDev/XeGTAO) |
| SSR (`ssr_*`) | FidelityFX SSSR 1.5 (SDK 1.1.4) | MIT | HLSL + GLSL, DX12 / Vulkan | [manual](https://gpuopen.com/manuals/fidelityfx_sdk/techniques/stochastic-screen-space-reflections/) |
| Shadows (`shadow_rect`) | TheRealMJP/Shadows | not stated | C++ / HLSL (DX11) | [article](https://therealmjp.github.io/posts/shadow-maps/) |
| Contact shadows | Bend Studio SSS example code | not stated on blog | (download) | [blog](https://www.bendstudio.com/blog/inside-bend-screen-space-shadows/) |
| Upscaler (`fsr`) | GPUOpen-Effects/FidelityFX-FSR2 2.2.1 | MIT-style | HLSL, DX12 + Vulkan backends | [repo](https://github.com/GPUOpen-Effects/FidelityFX-FSR2) |
| Ocean (`ocean_spectrum`, `compute_fft`) | 2Retr0/GodotOceanWaves | MIT | Godot compute shaders | [repo](https://github.com/2Retr0/GodotOceanWaves) |
| Ocean (simple reference) | gasgiant/FFT-Ocean | MIT | Unity | [repo](https://github.com/gasgiant/FFT-Ocean) |
| Ocean lighting / variance (`ocean_variance`) | Bruneton et al. 2010 via Proland | not verified | C++ / GLSL | [Proland](https://proland.inrialpes.fr/doc/proland-4.0/ocean/html/page-examples.html) |
| Volumetric clouds (`cloud_*`) | AmanSachan1/Meteoros | not verified | C++ / Vulkan | [repo](https://github.com/AmanSachan1/Meteoros) |
| Volumetric clouds (Nubis3) | YueZhang1027/CIS5650-Final-Project-Frostnova | not verified | Vulkan | [repo](https://github.com/YueZhang1027/CIS5650-Final-Project-Frostnova) |
| Tiled lights (`light_tile`) | Vulkan tutorial Forward+ chapter | Khronos docs | C++ / shaders, Vulkan | [docs](https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Advanced_Topics/ForwardPlus_Rendering.html) |

### Inferences
- **Porting order by value/effort:**
  1. Hillaire sky (HLSL to Slang near-direct; replaces any analytic sky).
  2. Histogram exposure, then Khronos Neutral, then Wronski exposure fusion.
  3. glTF-IBL-Sampler logic as a runtime compute prefilter, plus a DFG LUT.
  4. XeGTAO or CACAO.
  5. Stable CSM with GatherCmp PCF and a cloud transmittance map.
  6. Froxel fog sharing the AP froxel layout.
  7. Lagarde wet/puddle/ripple G-buffer pass.
  8. FFT ocean with a variance mip.
  9. SSSR (port SPD first).
  10. FSR2.
- **Slang notes:**
  - HLSL sources (Hillaire, XeGTAO, FSR2, SSSR/CACAO HLSL variants) are the easiest, since Slang accepts HLSL with minor fixes (resource binding attributes → `[[vk::binding]]` or parameter blocks).
  - GLSL-only sources (glTF-IBL-Sampler, Khronos Neutral, FlightGear) need manual translation, but they are small.
  - Wave-intrinsic code (SPD, SSSR) maps to Slang `WaveActive*` / `WaveReadLaneAt` and needs Vulkan 1.1+ subgroup support (available in Vulkan 1.3).

### Gaps
- Licenses still to confirm before vendoring code:

| Item | Status |
|---|---|
| Bend SSS example | not stated on blog |
| TheRealMJP/Shadows | not stated in article |
| pl-sky | none shown |
| bruop bgfx sample | not stated |
| MrLixm/AgXc | not checked |
| Proland | not checked |
| Meteoros, Frostnova | not checked |
| FlightGear fgdata | likely GPL, not checked |

- I did not verify whether FidelityFX SDK 1.1.4 remains downloadable as a tagged release (the release list shows v1.1.4, 08 May).
