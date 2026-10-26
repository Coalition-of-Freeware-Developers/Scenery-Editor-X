/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * cacao.slang
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 *
 * Ambient occlusion (plan Phase 8, TASK-057; XP12: cacao): AMD FidelityFX CACAO 1.4 ported to Slang from FidelityFX SDK
 * 1.1.4 (sdk/include/FidelityFX/gpu/cacao/ffx_cacao_*.h, the FP32 paths). renderer/cacao.h computes the constants and
 * the dispatches; Renderer::Pass_AmbientOcclusion runs one kernel per define:
 *   CACAO_PREPARE_DEPTHS        g-buffer depth -> deinterleaved view depths (4 layers) and 3 smart-averaged mips
 *   CACAO_PREPARE_NORMALS       g-buffer world normals -> deinterleaved CACAO view-space normals
 *   CACAO_GENERATE              CACAO_QUALITY 1 (medium, sparse), 2 (high), 3 (highest, adaptive) -> ping (AO, packed edges)
 *   CACAO_GENERATE_BASE         highest's 5-tap base pass -> pong (obscurance, weight)
 *   CACAO_IMPORTANCE_MAP(_A/_B) highest's importance map, smoothed twice; B adds to the load counter
 *   CACAO_BLUR                  edge-sensitive blur, ping -> pong
 *   CACAO_APPLY                 full resolution: the 4 layers into the output (smart: edge-aware)
 *   CACAO_UPSCALE               half resolution: bilateral 5 x 5 upscale to the output, CACAO_SMART 0 / 1
 * Like X-Plane 12.4.4's cacao, the output is the visibility raised to 2.2 (SDK 1.1.4), and the obscurance also fades in
 * near the camera (X-Plane's ssao/fade_in_*). SEDX differences: reverse-Z depth (the constants carry it); CACAO's view
 * space is x right, y up, z forward, and the g-buffer's world normals are turned into it; the point-mirror gathers are
 * mirrored loads (SEDX has no mirror sampler); out-of-range writes are skipped and out-of-range reads clamped (Vulkan
 * gives no D3D-style bounds guarantees); the load counter is zeroed by the importance-map kernel instead of a clear.
 * The output is lerp(1, ao ^ exterior_curve, exterior_strength) (ssao/exterior_*, identity at X-Plane's values).
 *
 * AMD FidelityFX CACAO, Copyright (C) 2024 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

import resources;

// The constants (renderer/cacao.h, CacaoConstants): 48 float4s
struct CacaoConstants
{
    float4 depth_unpack;            // view depth = x / (y - depth); zw tan(fov / 2)
    float4 ndc_to_view;             // xy multiply, zw add
    float4 depth_uv_to_view;
    float4 effect;                  // radius, shadow strength, shadow power, shadow clamp
    float4 fade;                    // fade-out multiply, add; fade-in multiply, add
    float4 sampling;                // horizon angle threshold, 1 / sampling radius near limit, depth precision offset, -1 / radius
    float4 adaptive;                // load counter average divisor, adaptive sample count limit, 1 - sharpness, detail AO strength
    float4 bilateral;               // sigma squared, similarity distance sigma, blur passes, exterior curve
    float4 ssao_size;               // width, height, 1 / width, 1 / height
    float4 depth_size;
    float4 output_size;
    float4 importance_size;
    float4 deinterleaved_size;
    float4 output;                  // exterior strength, quality, downsampled, 0
    float4 per_pass_full_res_uv_offset[4];
    float4 pattern_rot_scale[20];   // [layer x 5 + sub-pass]
    float4 normals_to_view[3];      // rows: world -> CACAO view space
    float4 padding[7];
};
[[vk::binding(202)]] cbuffer BufferCacao : register(b202) { CacaoConstants buffer_cacao; };

// CACAO's buffers: what a kernel reads (bound to the image it needs) and writes
[[vk::binding(64)]] Texture2DArray<float> cacao_depth : register(t64);          // deinterleaved view depth, 4 mips
[[vk::binding(65)]] Texture2DArray<float4> cacao_normals : register(t65);       // deinterleaved view-space normals
[[vk::binding(66)]] Texture2DArray<float2> cacao_ssao : register(t66);          // ping or pong: AO + packed edges (or base pass)
[[vk::binding(67)]] Texture2D<float> cacao_importance : register(t67);          // importance map or its pong
[[vk::binding(109)]] [[vk::image_format("unknown")]] RWTexture2DArray<float> cacao_depth_out[4] : register(u109);
[[vk::binding(110)]] [[vk::image_format("unknown")]] RWTexture2DArray<float4> cacao_normals_out : register(u110);
[[vk::binding(111)]] [[vk::image_format("unknown")]] RWTexture2DArray<float2> cacao_ssao_out : register(u111);
[[vk::binding(112)]] [[vk::image_format("unknown")]] RWTexture2D<float> cacao_importance_out : register(u112);
// tex_depth (g-buffer depth), tex_normal (g-buffer normals), tex_uav (output) and tex_uav_uint (the 1 x 1 load counter)
// come from resources.slang

// =====================================================================================================================
// Constants and utilities (ffx_cacao_utils.h)
// =====================================================================================================================

float2 ssao_inverse_size() { return buffer_cacao.ssao_size.zw; }
float2 deinterleaved_inverse_size() { return buffer_cacao.deinterleaved_size.zw; }
float effect_radius() { return buffer_cacao.effect.x; }
float effect_shadow_strength() { return buffer_cacao.effect.y; }
float effect_shadow_pow() { return buffer_cacao.effect.z; }
float effect_shadow_clamp() { return buffer_cacao.effect.w; }
float inv_sharpness() { return buffer_cacao.adaptive.z; }
bool is_downsampled() { return buffer_cacao.output.z > 0.5f; }

// Point-mirror addressing (D3D MIRROR: -1 -> 0, size -> size - 1)
int mirror_coord(int t, int size)
{
    if (t < 0)
        t = -t - 1;
    if (t >= size)
        t = 2 * size - t - 1;
    return clamp(t, 0, size - 1);
}

// Packing / unpacking for edges; 2 bits per edge mean 4 gradient values (0, 0.33, 0.66, 1) for smoother transitions
float pack_edges(float4 edges_lrtb)
{
    edges_lrtb = round(saturate(edges_lrtb) * 3.05f);
    return dot(edges_lrtb, float4(64.0f / 255.0f, 16.0f / 255.0f, 4.0f / 255.0f, 1.0f / 255.0f));
}

float4 unpack_edges(float packed_value)
{
    const uint packed = uint(packed_value * 255.5f);
    float4 edges_lrtb;
    edges_lrtb.x = float((packed >> 6) & 0x03) / 3.0f;
    edges_lrtb.y = float((packed >> 4) & 0x03) / 3.0f;
    edges_lrtb.z = float((packed >> 2) & 0x03) / 3.0f;
    edges_lrtb.w = float((packed >> 0) & 0x03) / 3.0f;
    return saturate(edges_lrtb + inv_sharpness());
}

// ffxInvertSafe: 1 / v, 0 for 0
float invert_safe(float v)
{
    const float s = sign(v);
    const float s2 = s * s;
    return s2 / (v + s2 - 1.0f);
}

// Reverse-Z depth -> view depth (the constants solve SEDX's projection)
float screen_to_view_depth(float screen_depth)
{
    return buffer_cacao.depth_unpack.x * invert_safe(buffer_cacao.depth_unpack.y - screen_depth);
}

float4 calculate_edges(float center_z, float left_z, float right_z, float top_z, float bottom_z)
{
    // slope-sensitive depth-based edge detection
    float4 edges_lrtb = float4(left_z, right_z, top_z, bottom_z) - center_z;
    const float4 edges_lrtb_slope_adjusted = edges_lrtb + edges_lrtb.yxwz;
    edges_lrtb = min(abs(edges_lrtb), abs(edges_lrtb_slope_adjusted));
    return saturate(1.3f - edges_lrtb / (center_z * 0.040f));
}

float3 ndc_to_view_space(float2 position, float view_depth)
{
    return float3((buffer_cacao.ndc_to_view.xy * position + buffer_cacao.ndc_to_view.zw) * view_depth, view_depth);
}

float3 depth_buffer_uv_to_view_space(float2 position, float view_depth)
{
    return float3((buffer_cacao.depth_uv_to_view.xy * position + buffer_cacao.depth_uv_to_view.zw) * view_depth, view_depth);
}

uint pack_f32(float2 v) { return f32tof16(v.x) | (f32tof16(v.y) << 16); }
float2 unpack_f32(uint a) { return float2(f16tof32(a & 0xFFFF), f16tof32(a >> 16)); }

int2 depth_buffer_size() { return int2(buffer_cacao.depth_size.xy); }

// The deinterleaved view depth is R16F (as AMD's): its largest finite value, with some margin
static const float MAX_VIEW_DEPTH = 65000.0f;

float load_screen_depth(int2 coord)
{
    return tex_depth.Load(int3(clamp(coord, int2(0, 0), depth_buffer_size() - 1), 0)).r;
}

// =====================================================================================================================
// Prepare (ffx_cacao_prepare.h): deinterleaved depths and mips, normals
// =====================================================================================================================

#if defined(CACAO_PREPARE_DEPTHS)

groupshared float s_prepare_depths_and_mips[4][8][8];

float mip_smart_average(float4 depths)
{
    const float closest = min(min(depths.x, depths.y), min(depths.z, depths.w));
    // As in AMD's code: -1 / r * r (it evaluates to -1)
    const float falloff_calc_mul_sq = -1.0f / effect_radius() * effect_radius();
    const float4 dists = depths - closest.xxxx;
    const float4 weights = saturate(dists * dists * falloff_calc_mul_sq + 1.0f);
    return dot(weights, depths) / dot(weights, float4(1.0f, 1.0f, 1.0f, 1.0f));
}

// MIP is a compile-time constant so the image array is never indexed dynamically
void store_depth_mip<let MIP : int>(uint2 coord, uint layer, float value)
{
    uint width, height, layers;
    cacao_depth_out[MIP].GetDimensions(width, height, layers);
    if (coord.x < width && coord.y < height)
        cacao_depth_out[MIP][uint3(coord, layer)] = value;
}

// samples: x (0, 1), y (1, 1), z (1, 0), w (0, 0) of the 2 x 2 texels the layers take (GatherRed's order)
void prepare_depths_and_mips(float4 samples, uint2 output_coord, uint2 gtid)
{
    samples = float4(screen_to_view_depth(samples.x), screen_to_view_depth(samples.y), screen_to_view_depth(samples.z), screen_to_view_depth(samples.w));
    samples = min(samples, MAX_VIEW_DEPTH);	// a far plane past R16F's range would store infinity

    s_prepare_depths_and_mips[0][gtid.x][gtid.y] = samples.w;
    s_prepare_depths_and_mips[1][gtid.x][gtid.y] = samples.z;
    s_prepare_depths_and_mips[2][gtid.x][gtid.y] = samples.x;
    s_prepare_depths_and_mips[3][gtid.x][gtid.y] = samples.y;

    store_depth_mip<0>(output_coord, 0, samples.w);
    store_depth_mip<0>(output_coord, 1, samples.z);
    store_depth_mip<0>(output_coord, 2, samples.x);
    store_depth_mip<0>(output_coord, 3, samples.y);

    const uint depth_array_index = 2 * (gtid.y % 2) + (gtid.x % 2);
    const uint2 depth_array_offset = uint2(gtid.x % 2, gtid.y % 2);
    const int2 buffer_coord = int2(gtid) - int2(depth_array_offset);

    output_coord /= 2;
    GroupMemoryBarrierWithGroupSync();

    // every thread is alive here
    {
        const float sample_00 = s_prepare_depths_and_mips[depth_array_index][buffer_coord.x + 0][buffer_coord.y + 0];
        const float sample_01 = s_prepare_depths_and_mips[depth_array_index][buffer_coord.x + 0][buffer_coord.y + 1];
        const float sample_10 = s_prepare_depths_and_mips[depth_array_index][buffer_coord.x + 1][buffer_coord.y + 0];
        const float sample_11 = s_prepare_depths_and_mips[depth_array_index][buffer_coord.x + 1][buffer_coord.y + 1];
        const float average = mip_smart_average(float4(sample_00, sample_01, sample_10, sample_11));
        store_depth_mip<1>(output_coord, depth_array_index, average);
        s_prepare_depths_and_mips[depth_array_index][buffer_coord.x][buffer_coord.y] = average;
    }

    bool still_alive = gtid.x % 4 == depth_array_offset.x && gtid.y % 4 == depth_array_offset.y;

    output_coord /= 2;
    GroupMemoryBarrierWithGroupSync();

    if (still_alive)
    {
        const float sample_00 = s_prepare_depths_and_mips[depth_array_index][buffer_coord.x + 0][buffer_coord.y + 0];
        const float sample_01 = s_prepare_depths_and_mips[depth_array_index][buffer_coord.x + 0][buffer_coord.y + 2];
        const float sample_10 = s_prepare_depths_and_mips[depth_array_index][buffer_coord.x + 2][buffer_coord.y + 0];
        const float sample_11 = s_prepare_depths_and_mips[depth_array_index][buffer_coord.x + 2][buffer_coord.y + 2];
        const float average = mip_smart_average(float4(sample_00, sample_01, sample_10, sample_11));
        store_depth_mip<2>(output_coord, depth_array_index, average);
        s_prepare_depths_and_mips[depth_array_index][buffer_coord.x][buffer_coord.y] = average;
    }

    still_alive = gtid.x % 8 == depth_array_offset.x && gtid.y % 8 == depth_array_offset.y;

    output_coord /= 2;
    GroupMemoryBarrierWithGroupSync();

    if (still_alive)
    {
        const float sample_00 = s_prepare_depths_and_mips[depth_array_index][buffer_coord.x + 0][buffer_coord.y + 0];
        const float sample_01 = s_prepare_depths_and_mips[depth_array_index][buffer_coord.x + 0][buffer_coord.y + 4];
        const float sample_10 = s_prepare_depths_and_mips[depth_array_index][buffer_coord.x + 4][buffer_coord.y + 0];
        const float sample_11 = s_prepare_depths_and_mips[depth_array_index][buffer_coord.x + 4][buffer_coord.y + 4];
        const float average = mip_smart_average(float4(sample_00, sample_01, sample_10, sample_11));
        store_depth_mip<3>(output_coord, depth_array_index, average);
    }
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main_comp(uint3 tid: SV_DispatchThreadID, uint3 gtid: SV_GroupThreadID)
{
    // Native: the 2 x 2 texels at 2 x tid (GatherRed, point clamp); downsampled: the corners of the 4 x 4 at 4 x tid
    const int step = is_downsampled() ? 2 : 1;
    const int2 base = int2(tid.xy) * 2 * step;
    float4 samples;
    samples.x = load_screen_depth(base + int2(0, step));
    samples.y = load_screen_depth(base + int2(step, step));
    samples.z = load_screen_depth(base + int2(step, 0));
    samples.w = load_screen_depth(base);
    prepare_depths_and_mips(samples, tid.xy, gtid.xy);
}

#endif

#if defined(CACAO_PREPARE_NORMALS)

// The g-buffer's world normal (no normal where nothing was drawn: faces the camera) in CACAO's view space
float3 load_normal(int2 coord)
{
    const float3 world = tex_normal.Load(int3(clamp(coord, int2(0, 0), depth_buffer_size() - 1), 0)).xyz;
    const float length_sq = dot(world, world);
    if (length_sq < 1e-8f)
        return float3(0.0f, 0.0f, -1.0f);
    const float3 n = world * rsqrt(length_sq);
    return float3(dot(buffer_cacao.normals_to_view[0].xyz, n), dot(buffer_cacao.normals_to_view[1].xyz, n), dot(buffer_cacao.normals_to_view[2].xyz, n));
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main_comp(uint3 tid: SV_DispatchThreadID)
{
    uint width, height, layers;
    cacao_normals_out.GetDimensions(width, height, layers);
    if (tid.x >= width || tid.y >= height)
        return;

    // Native: the 2 x 2 at 2 x tid; downsampled: every other texel of the 4 x 4 at 4 x tid
    const int step = is_downsampled() ? 2 : 1;
    const int2 base = int2(tid.xy) * 2 * step;
    cacao_normals_out[uint3(tid.xy, 0)] = float4(load_normal(base), 1.0f);
    cacao_normals_out[uint3(tid.xy, 1)] = float4(load_normal(base + int2(step, 0)), 1.0f);
    cacao_normals_out[uint3(tid.xy, 2)] = float4(load_normal(base + int2(0, step)), 1.0f);
    cacao_normals_out[uint3(tid.xy, 3)] = float4(load_normal(base + int2(step, step)), 1.0f);
}

#endif

// =====================================================================================================================
// SSAO generation (ffx_cacao_ssao_generation.h)
// =====================================================================================================================

#if defined(CACAO_GENERATE) || defined(CACAO_GENERATE_BASE)

#if defined(CACAO_GENERATE_BASE)
#define CACAO_QUALITY 3
static const int QUALITY = 3;
static const bool ADAPTIVE_BASE = true;
#else
static const int QUALITY = CACAO_QUALITY;
static const bool ADAPTIVE_BASE = false;
#endif

static const float4 SAMPLE_PATTERN[32] =
{
    float4( 0.78488064f,  0.56661671f,  1.500000f, -0.126083f), float4( 0.26022232f, -0.29575172f,  1.500000f, -1.064030f),
    float4( 0.10459357f,  0.08372527f,  1.110000f, -2.730563f), float4(-0.68286800f,  0.04963045f,  1.090000f, -0.498827f),
    float4(-0.13570161f, -0.64190155f,  1.250000f, -0.532765f), float4(-0.26193795f, -0.08205118f,  0.670000f, -1.783245f),
    float4(-0.61177456f,  0.66664219f,  0.710000f, -0.044234f), float4( 0.43675563f,  0.25119025f,  0.610000f, -1.167283f),
    float4( 0.07884444f,  0.86618668f,  0.640000f, -0.459002f), float4(-0.12790935f, -0.29869005f,  0.600000f, -1.729424f),
    float4(-0.04031125f,  0.02413622f,  0.600000f, -4.792042f), float4( 0.16201244f, -0.52851415f,  0.790000f, -1.067055f),
    float4(-0.70991218f,  0.47301072f,  0.640000f, -0.335236f), float4( 0.03277707f, -0.22349690f,  0.600000f, -1.982384f),
    float4( 0.68921727f,  0.36800742f,  0.630000f, -0.266718f), float4( 0.29251814f,  0.37775412f,  0.610000f, -1.422520f),
    float4(-0.12224089f,  0.96582592f,  0.600000f, -0.426142f), float4( 0.11071457f, -0.16131058f,  0.600000f, -2.165947f),
    float4( 0.46562141f, -0.59747696f,  0.600000f, -0.189760f), float4(-0.51548797f,  0.11804193f,  0.600000f, -1.246800f),
    float4( 0.89141309f, -0.42090443f,  0.600000f,  0.028192f), float4(-0.32402530f, -0.01591529f,  0.600000f, -1.543018f),
    float4( 0.60771245f,  0.41635221f,  0.600000f, -0.605411f), float4( 0.02379565f, -0.08239821f,  0.600000f, -3.809046f),
    float4( 0.48951152f, -0.23657045f,  0.600000f, -1.189011f), float4(-0.17611565f, -0.81696892f,  0.600000f, -0.513724f),
    float4(-0.33930185f, -0.20732205f,  0.600000f, -1.698047f), float4(-0.91974425f,  0.05403209f,  0.600000f,  0.062246f),
    float4(-0.15064627f, -0.14949332f,  0.600000f, -1.896062f), float4( 0.53180975f, -0.35210401f,  0.600000f, -0.758838f),
    float4( 0.41487166f,  0.81442589f,  0.600000f, -0.505648f), float4(-0.24106961f, -0.32721516f,  0.600000f, -1.665244f)
};

static const int MAX_TAPS = 32;
static const int ADAPTIVE_TAP_BASE_COUNT = 5;
static const int ADAPTIVE_TAP_FLEXIBLE_COUNT = MAX_TAPS - ADAPTIVE_TAP_BASE_COUNT;
// Taps of the non-adaptive loop (AMD's g_FFX_CACAO_numTaps: 3, 5, 12 for quality 0-2; the adaptive base pass uses 5).
// The actual number of depth samples is twice this (each tap has two symmetrical samples).
#if defined(CACAO_GENERATE_BASE) || CACAO_QUALITY == 1
static const int NUMBER_OF_TAPS = 5;
#elif CACAO_QUALITY == 2
static const int NUMBER_OF_TAPS = 12;
#else
static const int NUMBER_OF_TAPS = 0;
#endif

// Optional parts enabled from a quality preset up (0 low, 1 medium, 2 high, 3 highest / adaptive; 99 disabled)
static const int TILT_SAMPLES_ENABLE_AT_QUALITY_PRESET = 99;
static const float TILT_SAMPLES_AMOUNT = 0.4f;
static const int HALOING_REDUCTION_ENABLE_AT_QUALITY_PRESET = 1;
static const float HALOING_REDUCTION_AMOUNT = 0.6f;
static const int NORMAL_BASED_EDGES_ENABLE_AT_QUALITY_PRESET = 2;
static const float NORMAL_BASED_EDGES_DOT_THRESHOLD = 0.5f;
static const int DETAIL_AO_ENABLE_AT_QUALITY_PRESET = 1;
static const int DEPTH_MIPS_ENABLE_AT_QUALITY_PRESET = 2;
static const float DEPTH_MIPS_GLOBAL_OFFSET = -4.3f;
static const int DEPTH_BASED_EDGES_ENABLE_AT_QUALITY_PRESET = 1;
static const int REDUCE_RADIUS_NEAR_SCREEN_BORDER_ENABLE_AT_QUALITY_PRESET = 99;

float sample_view_depth_mip(float2 uv, float mip, uint layer)
{
    return cacao_depth.SampleLevel(samplers[sampler_point_clamp], float3(uv, float(layer)), mip);
}

float load_view_depth_mirrored(int2 coord, uint layer)
{
    const int2 size = int2(buffer_cacao.deinterleaved_size.xy);
    return cacao_depth.Load(int4(mirror_coord(coord.x, size.x), mirror_coord(coord.y, size.y), int(layer), 0));
}

float3 load_normal_pass(int2 coord, uint layer)
{
    const int2 size = int2(buffer_cacao.ssao_size.xy);
    return cacao_normals.Load(int4(clamp(coord, int2(0, 0), size - 1), int(layer), 0)).xyz;
}

// calculate effect radius and fit our screen sampling pattern inside it
void calculate_radius_parameters(float pix_center_length, float2 pixel_dir_rb_viewspace_size_at_center_z, out float pix_lookup_radius_mod,
                                 out float radius, out float falloff_calc_mul_sq)
{
    radius = effect_radius();

    // when too close, on-screen sampling disk will grow beyond screen size; limit this to avoid closeup temporal artifacts
    const float too_close_limit_mod = saturate(pix_center_length * buffer_cacao.sampling.y) * 0.8f + 0.2f;
    radius *= too_close_limit_mod;

    // 0.85 is to reduce the radius to allow for more samples on a slope to still stay within influence
    pix_lookup_radius_mod = (0.85f * radius) / pixel_dir_rb_viewspace_size_at_center_z.x;

    // used to calculate falloff (both for AO samples and per-sample weights)
    falloff_calc_mul_sq = -1.0f / (radius * radius);
}

// all vectors in viewspace
float calculate_pixel_obscurance(float3 pixel_normal, float3 hit_delta, float falloff_calc_mul_sq)
{
    const float length_sq = dot(hit_delta, hit_delta);
    const float n_dot_d = dot(pixel_normal, hit_delta) / sqrt(length_sq);
    const float falloff_mult = max(0.0f, length_sq * falloff_calc_mul_sq + 1.0f);
    return max(0.0f, n_dot_d - buffer_cacao.sampling.x) * falloff_mult;
}

float haloing_weight(float3 hit_delta)
{
    // max(0, dot(hit_delta, neg_viewspace_dir)) in its cheaper, less correct form
    float reduct = max(0.0f, -hit_delta.z);
    reduct = saturate(reduct * buffer_cacao.sampling.w + 2.0f);	// saturate(2 - reduct / radius)
    return HALOING_REDUCTION_AMOUNT * reduct + (1.0f - HALOING_REDUCTION_AMOUNT);
}

void ssao_tap_inner(inout float obscurance_sum, inout float weight_sum, float2 sampling_uv, float mip_level, float3 pix_center_pos, float3 pixel_normal,
                    float falloff_calc_mul_sq, float weight_mod, uint layer)
{
    // get depth at sample, convert to viewspace
    const float viewspace_sample_z = sample_view_depth_mip(sampling_uv, mip_level, layer);
    const float3 hit_pos = depth_buffer_uv_to_view_space(sampling_uv.xy, viewspace_sample_z);
    const float3 hit_delta = hit_pos - pix_center_pos;

    const float obscurance = calculate_pixel_obscurance(pixel_normal, hit_delta, falloff_calc_mul_sq);
    float weight = 1.0f;
    if (QUALITY >= HALOING_REDUCTION_ENABLE_AT_QUALITY_PRESET)
        weight = haloing_weight(hit_delta);
    weight *= weight_mod;
    obscurance_sum += obscurance * weight;
    weight_sum += weight;
}

// rot_scale: the 2 x 2 matrix's rows (x, y) and (z, w), applied as a column-vector product
float2 transform_sample(float4 rot_scale, float2 v)
{
    return float2(rot_scale.x * v.x + rot_scale.y * v.y, rot_scale.z * v.x + rot_scale.w * v.y);
}

void ssao_tap(inout float obscurance_sum, inout float weight_sum, int tap_index, float4 rot_scale, float3 pix_center_pos, float3 pixel_normal,
              float2 depth_buffer_uv, float mip_offset, float falloff_calc_mul_sq, float weight_mod, float2 norm_xy, float norm_xy_length, uint layer)
{
    float2 sample_offset;
    float sample_pow2_len;

    // patterns
    {
        const float4 new_sample = SAMPLE_PATTERN[tap_index];
        sample_offset = transform_sample(rot_scale, new_sample.xy);
        sample_pow2_len = new_sample.w;	// precalculated, same as: log2(length(new_sample.xy))
        weight_mod *= new_sample.z;
    }

    // snap to pixel center (more correct obscurance math, avoids artifacts)
    sample_offset = round(sample_offset);

    // calculate MIP based on the sample distance from the centre, similar to as described in
    // http://graphics.cs.williams.edu/papers/SAOHPG12/
    const float mip_level = (QUALITY < DEPTH_MIPS_ENABLE_AT_QUALITY_PRESET) ? 0.0f : (sample_pow2_len + mip_offset);

    const float2 sampling_uv = sample_offset * deinterleaved_inverse_size() + depth_buffer_uv;
    ssao_tap_inner(obscurance_sum, weight_sum, sampling_uv, mip_level, pix_center_pos, pixel_normal, falloff_calc_mul_sq, weight_mod, layer);

    // for the second tap, just use the mirrored offset
    float2 sample_offset_mirrored_uv = -sample_offset;

    // tilt the second set of samples so that the disk is effectively rotated by the normal; effective at removing one
    // set of artifacts, but too expensive for lower quality settings
    if (QUALITY >= TILT_SAMPLES_ENABLE_AT_QUALITY_PRESET)
    {
        const float dot_norm = dot(sample_offset_mirrored_uv, norm_xy);
        sample_offset_mirrored_uv -= dot_norm * norm_xy_length * norm_xy;
        sample_offset_mirrored_uv = round(sample_offset_mirrored_uv);
    }

    // snap to pixel center (more correct obscurance math, avoids artifacts)
    const float2 sampling_mirrored_uv = sample_offset_mirrored_uv * deinterleaved_inverse_size() + depth_buffer_uv;
    ssao_tap_inner(obscurance_sum, weight_sum, sampling_mirrored_uv, mip_level, pix_center_pos, pixel_normal, falloff_calc_mul_sq, weight_mod, layer);
}

struct SsaoHits
{
    float3 hits[2];
    float weight_mod;
};

struct SsaoSampleData
{
    float2 uv_offset;
    float mip_level;
    float weight_mod;
};

SsaoSampleData get_sample_data(float4 rot_scale, float4 new_sample, float mip_offset)
{
    float2 sample_offset = transform_sample(rot_scale, new_sample.xy);
    sample_offset = round(sample_offset) * deinterleaved_inverse_size();

    const float sample_pow2_len = new_sample.w;
    SsaoSampleData result;
    result.uv_offset = sample_offset;
    result.mip_level = (QUALITY < DEPTH_MIPS_ENABLE_AT_QUALITY_PRESET) ? 0.0f : (sample_pow2_len + mip_offset);
    result.weight_mod = new_sample.z;
    return result;
}

SsaoHits get_hits2(SsaoSampleData data, float2 depth_buffer_uv, uint layer)
{
    SsaoHits result;
    result.weight_mod = data.weight_mod;
    float2 sample_uv = depth_buffer_uv + data.uv_offset;
    result.hits[0] = float3(sample_uv, sample_view_depth_mip(sample_uv, data.mip_level, layer));
    sample_uv = depth_buffer_uv - data.uv_offset;
    result.hits[1] = float3(sample_uv, sample_view_depth_mip(sample_uv, data.mip_level, layer));
    return result;
}

void add_hits(float3 pix_center_pos, float3 pixel_normal, float falloff_calc_mul_sq, inout float weight_sum, inout float obscurance_sum, SsaoHits hits)
{
    float weight = hits.weight_mod;
    [unroll]
    for (int hit_index = 0; hit_index < 2; ++hit_index)
    {
        const float3 hit = hits.hits[hit_index];
        const float3 hit_pos = depth_buffer_uv_to_view_space(hit.xy, hit.z);
        const float3 hit_delta = hit_pos - pix_center_pos;

        const float obscurance = calculate_pixel_obscurance(pixel_normal, hit_delta, falloff_calc_mul_sq);
        if (QUALITY >= HALOING_REDUCTION_ENABLE_AT_QUALITY_PRESET)
            weight = haloing_weight(hit_delta);
        obscurance_sum += obscurance * weight;
        weight_sum += weight;
    }
}

void generate_ssao_shadows_internal(out float out_shadow_term, out float4 out_edges, out float out_weight, float2 sv_pos, uint layer)
{
    const float2 sv_pos_rounded = trunc(sv_pos);
    const int2 sv_pos_i = int2(sv_pos_rounded);

    // the pixel's view depth and its left, right, top, bottom neighbours (two point-mirror gathers in AMD's code)
    const float2 depth_buffer_uv = (sv_pos + 0.5f) * deinterleaved_inverse_size();
    const float pix_z = load_view_depth_mirrored(sv_pos_i, layer);
    const float pix_lz = load_view_depth_mirrored(sv_pos_i + int2(-1, 0), layer);
    const float pix_tz = load_view_depth_mirrored(sv_pos_i + int2(0, -1), layer);
    const float pix_rz = load_view_depth_mirrored(sv_pos_i + int2(1, 0), layer);
    const float pix_bz = load_view_depth_mirrored(sv_pos_i + int2(0, 1), layer);

    const float2 normalized_screen_pos = (sv_pos_rounded + 0.5f) * ssao_inverse_size();
    float3 pix_center_pos = ndc_to_view_space(normalized_screen_pos, pix_z);

    // load this pixel's viewspace normal
    float3 pixel_normal = load_normal_pass(sv_pos_i, layer);

    // optimized approximation of: ndc_to_view_space(normalized_screen_pos + pixel size, z).xy - pix_center_pos.xy
    const float2 pixel_dir_rb_viewspace_size_at_center_z = pix_center_pos.z * buffer_cacao.ndc_to_view.xy * ssao_inverse_size();

    float pix_lookup_radius_mod;
    float falloff_calc_mul_sq;

    // calculate effect radius and fit our screen sampling pattern inside it
    float effect_viewspace_radius;
    calculate_radius_parameters(length(pix_center_pos), pixel_dir_rb_viewspace_size_at_center_z, pix_lookup_radius_mod, effect_viewspace_radius, falloff_calc_mul_sq);

    // calculate samples rotation/scaling
    float4 rot_scale;
    {
        // reduce effect radius near the screen edges slightly; ideally, one would render a larger depth buffer (5% on
        // each side) instead
        if (!ADAPTIVE_BASE && (QUALITY >= REDUCE_RADIUS_NEAR_SCREEN_BORDER_ENABLE_AT_QUALITY_PRESET))
        {
            float near_screen_border = min(min(depth_buffer_uv.x, 1.0f - depth_buffer_uv.x), min(depth_buffer_uv.y, 1.0f - depth_buffer_uv.y));
            near_screen_border = saturate(10.0f * near_screen_border + 0.6f);
            pix_lookup_radius_mod *= near_screen_border;
        }

        // load & update pseudo-random rotation matrix
        const uint pseudo_random_index = uint(sv_pos_rounded.y * 2.0f + sv_pos_rounded.x) % 5;
        rot_scale = buffer_cacao.pattern_rot_scale[layer * 5 + pseudo_random_index] * pix_lookup_radius_mod;
    }

    // the main obscurance & sample weight storage
    float obscurance_sum = 0.0f;
    float weight_sum = 0.0f;

    // edge mask for between this and left/right/top/bottom neighbour pixels - not used in quality level 0 so initialize
    // to "no edge" (1 is no edge, 0 is edge)
    float4 edges_lrtb = float4(1.0f, 1.0f, 1.0f, 1.0f);

    // move center pixel slightly towards camera to avoid imprecision artifacts due to using of 16bit depth buffer
    pix_center_pos *= buffer_cacao.sampling.z;

    if (!ADAPTIVE_BASE && (QUALITY >= DEPTH_BASED_EDGES_ENABLE_AT_QUALITY_PRESET))
        edges_lrtb = calculate_edges(pix_z, pix_lz, pix_rz, pix_tz, pix_bz);

    // adds a more high definition sharp effect, which gets blurred out (reuses left/right/top/bottom samples that we
    // used for edge detection)
    if (!ADAPTIVE_BASE && (QUALITY >= DETAIL_AO_ENABLE_AT_QUALITY_PRESET))
    {
        // disable in case of quality level 4 (reference)
        if (QUALITY != 4)
        {
            // approximate neighbouring pixels positions (actually just deltas or "positions - pix_center_pos")
            const float3 viewspace_dir_z_normalized = float3(pix_center_pos.xy / pix_center_pos.zz, 1.0f);

            const float3 pix_l_delta = float3(-pixel_dir_rb_viewspace_size_at_center_z.x, 0.0f, 0.0f) + viewspace_dir_z_normalized * (pix_lz - pix_center_pos.z);
            const float3 pix_r_delta = float3(+pixel_dir_rb_viewspace_size_at_center_z.x, 0.0f, 0.0f) + viewspace_dir_z_normalized * (pix_rz - pix_center_pos.z);
            const float3 pix_t_delta = float3(0.0f, -pixel_dir_rb_viewspace_size_at_center_z.y, 0.0f) + viewspace_dir_z_normalized * (pix_tz - pix_center_pos.z);
            const float3 pix_b_delta = float3(0.0f, +pixel_dir_rb_viewspace_size_at_center_z.y, 0.0f) + viewspace_dir_z_normalized * (pix_bz - pix_center_pos.z);

            const float range_reduction_const = 4.0f;	// this is to avoid various artifacts
            const float modified_falloff_calc_mul_sq = range_reduction_const * falloff_calc_mul_sq;

            float4 additional_obscurance;
            additional_obscurance.x = calculate_pixel_obscurance(pixel_normal, pix_l_delta, modified_falloff_calc_mul_sq);
            additional_obscurance.y = calculate_pixel_obscurance(pixel_normal, pix_r_delta, modified_falloff_calc_mul_sq);
            additional_obscurance.z = calculate_pixel_obscurance(pixel_normal, pix_t_delta, modified_falloff_calc_mul_sq);
            additional_obscurance.w = calculate_pixel_obscurance(pixel_normal, pix_b_delta, modified_falloff_calc_mul_sq);

            obscurance_sum += buffer_cacao.adaptive.w * dot(additional_obscurance, edges_lrtb);
        }
    }

    // sharp normals also create edges - but this adds to the cost as well
    if (!ADAPTIVE_BASE && (QUALITY >= NORMAL_BASED_EDGES_ENABLE_AT_QUALITY_PRESET))
    {
        const float3 neighbour_normal_l = load_normal_pass(sv_pos_i + int2(-1, +0), layer);
        const float3 neighbour_normal_r = load_normal_pass(sv_pos_i + int2(+1, +0), layer);
        const float3 neighbour_normal_t = load_normal_pass(sv_pos_i + int2(+0, -1), layer);
        const float3 neighbour_normal_b = load_normal_pass(sv_pos_i + int2(+0, +1), layer);

        const float dot_threshold = NORMAL_BASED_EDGES_DOT_THRESHOLD;

        float4 normal_edges_lrtb;
        normal_edges_lrtb.x = saturate(dot(pixel_normal, neighbour_normal_l) + dot_threshold);
        normal_edges_lrtb.y = saturate(dot(pixel_normal, neighbour_normal_r) + dot_threshold);
        normal_edges_lrtb.z = saturate(dot(pixel_normal, neighbour_normal_t) + dot_threshold);
        normal_edges_lrtb.w = saturate(dot(pixel_normal, neighbour_normal_b) + dot_threshold);

        edges_lrtb *= normal_edges_lrtb;
    }

    const float global_mip_offset = DEPTH_MIPS_GLOBAL_OFFSET;
    const float mip_offset = (QUALITY < DEPTH_MIPS_ENABLE_AT_QUALITY_PRESET) ? 0.0f : (log2(pix_lookup_radius_mod) + global_mip_offset);

    // used to tilt the second set of samples so that the disk is effectively rotated by the normal; effective at
    // removing one set of artifacts, but too expensive for lower quality settings
    float2 norm_xy = float2(pixel_normal.x, pixel_normal.y);
    float norm_xy_length = length(norm_xy);
    norm_xy /= float2(norm_xy_length, -norm_xy_length);
    norm_xy_length *= TILT_SAMPLES_AMOUNT;

#if !(defined(CACAO_GENERATE) && CACAO_QUALITY == 3)
    // standard, non-adaptive approach (quality 1, 2 and the adaptive base pass)
    [unroll]
    for (int i = 0; i < NUMBER_OF_TAPS; i++)
    {
        ssao_tap(obscurance_sum, weight_sum, i, rot_scale, pix_center_pos, pixel_normal, depth_buffer_uv, mip_offset, falloff_calc_mul_sq, 1.0f,
            norm_xy, norm_xy_length, layer);
    }
#else
    {
        // adaptive approach: add new samples where the importance map asks for them
        const float2 full_res_uv = normalized_screen_pos + buffer_cacao.per_pass_full_res_uv_offset[layer].xy;
        float importance = cacao_importance.SampleLevel(samplers[sampler_bilinear_clamp], full_res_uv, 0.0f);

        // this is to normalize DETAIL_AO_AMOUNT across all pixel regardless of importance
        obscurance_sum *= (ADAPTIVE_TAP_BASE_COUNT / float(MAX_TAPS)) + (importance * ADAPTIVE_TAP_FLEXIBLE_COUNT / float(MAX_TAPS));

        // load existing base values
        const int2 ssao_size = int2(buffer_cacao.ssao_size.xy);
        const float2 base_values = cacao_ssao.Load(int4(clamp(sv_pos_i, int2(0, 0), ssao_size - 1), int(layer), 0)).xy;
        weight_sum += base_values.y * float(ADAPTIVE_TAP_BASE_COUNT * 4.0f);
        obscurance_sum += base_values.x * weight_sum;

        // increase importance around edges
        const float avg_total_importance = float(tex_uav_uint[uint2(0, 0)]) * buffer_cacao.adaptive.x;
        const float importance_limiter = saturate(buffer_cacao.adaptive.y / avg_total_importance);
        importance *= importance_limiter;

        float additional_sample_count_flt = ADAPTIVE_TAP_FLEXIBLE_COUNT * importance;
        additional_sample_count_flt += 1.5f;
        const uint additional_samples = uint(additional_sample_count_flt);
        const uint additional_samples_to = min(uint(MAX_TAPS - 1), additional_samples + uint(ADAPTIVE_TAP_BASE_COUNT));

        // sample loop
        {
            float4 new_sample = SAMPLE_PATTERN[ADAPTIVE_TAP_BASE_COUNT];
            SsaoSampleData data = get_sample_data(rot_scale, new_sample, mip_offset);
            SsaoHits hits = get_hits2(data, depth_buffer_uv, layer);
            new_sample = SAMPLE_PATTERN[ADAPTIVE_TAP_BASE_COUNT + 1];

            for (uint i = ADAPTIVE_TAP_BASE_COUNT; i < additional_samples_to - 1; i++)
            {
                data = get_sample_data(rot_scale, new_sample, mip_offset);
                new_sample = SAMPLE_PATTERN[i + 2];
                const SsaoHits next_hits = get_hits2(data, depth_buffer_uv, layer);

                add_hits(pix_center_pos, pixel_normal, falloff_calc_mul_sq, weight_sum, obscurance_sum, hits);
                hits = next_hits;
            }

            // last loop iteration
            add_hits(pix_center_pos, pixel_normal, falloff_calc_mul_sq, weight_sum, obscurance_sum, hits);
        }
    }
#endif

    // early out for adaptive base - just output weight (used for the next pass)
    if (ADAPTIVE_BASE)
    {
        out_shadow_term = obscurance_sum / weight_sum;
        out_edges = float4(0.0f, 0.0f, 0.0f, 0.0f);
        out_weight = weight_sum;
        return;
    }

    // calculate weighted average
    float obscurance = obscurance_sum / weight_sum;

    // fade-out (1 close, gradient, 0 far) and X-Plane's fade-in close to the camera
    const float fade_out = saturate(pix_center_pos.z * buffer_cacao.fade.x + buffer_cacao.fade.y);
    const float fade_in = 1.0f - saturate(pix_center_pos.z * buffer_cacao.fade.z + buffer_cacao.fade.w);
    float fade = min(fade_out, fade_in);

    // reduce the SSAO shadowing if we're on the edge to remove artifacts on edges (we don't care for the lower quality one)
    if (!ADAPTIVE_BASE && (QUALITY >= DEPTH_BASED_EDGES_ENABLE_AT_QUALITY_PRESET))
    {
        // when there's more than 2 opposite edges, start fading out the occlusion to reduce aliasing artifacts
        const float edge_fadeout_factor = saturate((1.0f - edges_lrtb.x - edges_lrtb.y) * 0.35f) + saturate((1.0f - edges_lrtb.z - edges_lrtb.w) * 0.35f);
        fade *= saturate(1.0f - edge_fadeout_factor);
    }

    // strength, clamp, fade
    obscurance = effect_shadow_strength() * obscurance;
    obscurance = min(obscurance, effect_shadow_clamp());
    obscurance *= fade;

    // conceptually switch to occlusion with the meaning being visibility (grows with visibility, occlusion == 1
    // implies full visibility), to be in line with what is more commonly used
    float occlusion = 1.0f - obscurance;

    // modify the gradient; note: this cannot be moved to a later pass because of loss of precision after storing in
    // the render target
    occlusion = pow(saturate(occlusion), effect_shadow_pow());

    out_shadow_term = occlusion;	// 0 means fully occluded, 1 means fully lit
    out_edges = edges_lrtb;			// used to prevent blurring across edges, 1 means no edge, 0 means edge
    out_weight = weight_sum;
}

void store_ssao(uint2 coord, uint layer, float2 value)
{
    if (all(coord < uint2(buffer_cacao.ssao_size.xy)))
        cacao_ssao_out[uint3(coord, layer)] = value;
}

#if defined(CACAO_GENERATE) && CACAO_QUALITY == 1
// Medium: each thread of a 4 x 16 group does every fifth pixel of a row; z is 5 slices x 4 layers
[shader("compute")]
[numthreads(4, 16, 1)]
void main_comp(uint3 tid: SV_DispatchThreadID)
{
    const uint tid_corrected_z = tid.z % 5;
    const uint layer = tid.z / 5;
    const uint x_offset = (tid.y * 3 + tid_corrected_z) % 5;
    const uint2 coord = uint2(5 * tid.x + x_offset, tid.y);

    float out_shadow_term, out_weight;
    float4 out_edges;
    generate_ssao_shadows_internal(out_shadow_term, out_edges, out_weight, float2(coord), layer);
    store_ssao(coord, layer, float2(out_shadow_term, pack_edges(out_edges)));
}
#else
[shader("compute")]
[numthreads(8, 8, 1)]
void main_comp(uint3 coord: SV_DispatchThreadID)
{
    float out_shadow_term, out_weight;
    float4 out_edges;
    generate_ssao_shadows_internal(out_shadow_term, out_edges, out_weight, float2(coord.xy), coord.z);
#if defined(CACAO_GENERATE_BASE)
    store_ssao(coord.xy, coord.z, float2(out_shadow_term, out_weight / (float(ADAPTIVE_TAP_BASE_COUNT) * 4.0f)));
#else
    store_ssao(coord.xy, coord.z, float2(out_shadow_term, pack_edges(out_edges)));
#endif
}
#endif

#endif

// =====================================================================================================================
// Importance map (ffx_cacao_importance_map.h)
// =====================================================================================================================

#if defined(CACAO_IMPORTANCE_MAP)

[shader("compute")]
[numthreads(8, 8, 1)]
void main_comp(uint3 tid: SV_DispatchThreadID)
{
    // the load counter is zeroed here, two dispatches before ImportanceMapB adds to it
    if (all(tid == uint3(0, 0, 0)))
        tex_uav_uint[uint2(0, 0)] = 0;

    // the 2 x 2 texels at 2 x tid of each layer of the base pass (point-clamp GatherRed in AMD's code)
    const int2 base_pos = int2(tid.xy) * 2;
    const int2 ssao_max = int2(buffer_cacao.ssao_size.xy) - 1;
    float avg = 0.0f;
    float min_v = 1.0f;
    float max_v = 0.0f;
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        float4 vals;
        vals.x = cacao_ssao.Load(int4(clamp(base_pos + int2(0, 1), int2(0, 0), ssao_max), i, 0)).x;
        vals.y = cacao_ssao.Load(int4(clamp(base_pos + int2(1, 1), int2(0, 0), ssao_max), i, 0)).x;
        vals.z = cacao_ssao.Load(int4(clamp(base_pos + int2(1, 0), int2(0, 0), ssao_max), i, 0)).x;
        vals.w = cacao_ssao.Load(int4(clamp(base_pos, int2(0, 0), ssao_max), i, 0)).x;

        // apply the same modifications that would have been applied in the main shader
        const float shadow_pow = effect_shadow_pow();
        vals = effect_shadow_strength() * vals;
        vals = 1.0f - vals;
        vals = pow(saturate(vals), float4(shadow_pow, shadow_pow, shadow_pow, shadow_pow));

        avg += dot(vals, float4(1.0f / 16.0f, 1.0f / 16.0f, 1.0f / 16.0f, 1.0f / 16.0f));
        max_v = max(max_v, max(max(vals.x, vals.y), max(vals.z, vals.w)));
        min_v = min(min_v, min(min(vals.x, vals.y), min(vals.z, vals.w)));
    }

    const float min_max_diff = max_v - min_v;
    if (all(tid.xy < uint2(buffer_cacao.importance_size.xy)))
        cacao_importance_out[tid.xy] = pow(saturate(min_max_diff * 2.0f), 0.8f);
}

#endif

#if defined(CACAO_IMPORTANCE_MAP_A) || defined(CACAO_IMPORTANCE_MAP_B)

static const float SMOOTHEN_IMPORTANCE = 1.0f;

float sample_importance(float2 uv)
{
    return cacao_importance.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0.0f);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main_comp(uint3 tid: SV_DispatchThreadID)
{
    const float2 uv = (float2(tid.xy) + 0.5f) * buffer_cacao.importance_size.zw;
    const float centre = sample_importance(uv);
    const float2 half_pixel = 0.5f * buffer_cacao.importance_size.zw;

    float4 vals;
#if defined(CACAO_IMPORTANCE_MAP_A)
    vals.x = sample_importance(uv + float2(-half_pixel.x * 3, -half_pixel.y));
    vals.y = sample_importance(uv + float2(+half_pixel.x, -half_pixel.y * 3));
    vals.z = sample_importance(uv + float2(+half_pixel.x * 3, +half_pixel.y));
    vals.w = sample_importance(uv + float2(-half_pixel.x, +half_pixel.y * 3));
#else
    vals.x = sample_importance(uv + float2(-half_pixel.x, -half_pixel.y * 3));
    vals.y = sample_importance(uv + float2(+half_pixel.x * 3, -half_pixel.y));
    vals.z = sample_importance(uv + float2(+half_pixel.x, +half_pixel.y * 3));
    vals.w = sample_importance(uv + float2(-half_pixel.x * 3, +half_pixel.y));
#endif

    const float avg_val = dot(vals, float4(0.25f, 0.25f, 0.25f, 0.25f));
    vals.xy = max(vals.xy, vals.zw);
    const float max_val = max(centre, max(vals.x, vals.y));
    const float ret_val = lerp(max_val, avg_val, SMOOTHEN_IMPORTANCE);
    if (all(tid.xy < uint2(buffer_cacao.importance_size.xy)))
        cacao_importance_out[tid.xy] = ret_val;

#if defined(CACAO_IMPORTANCE_MAP_B)
    // sum the average; to avoid overflowing we assume max AO resolution is not bigger than 16384 x 16384; so quarter res
    // (used here) will be 4096 x 4096, which leaves us with 8 bits per pixel. Save every 9th to avoid InterlockedAdd
    // congestion - since we're blurring, this is good enough; compensated by multiplying the average divisor by 9. Like
    // AMD's, the threads past the map's edge count too.
    const uint sum = uint(saturate(ret_val) * 255.0f + 0.5f);
    if (((tid.x % 3) + (tid.y % 3)) == 0)
        InterlockedAdd(tex_uav_uint[uint2(0, 0)], sum);
#endif
}

#endif

// =====================================================================================================================
// Edge-sensitive blur (ffx_cacao_edge_sensitive_blur.h, FP32 path)
// =====================================================================================================================

#if defined(CACAO_BLUR)

static const int TILE_WIDTH = 4;
static const int TILE_HEIGHT = 3;
static const int HALF_TILE_WIDTH = TILE_WIDTH / 2;
static const int QUARTER_TILE_WIDTH = TILE_WIDTH / 4;
static const int BLUR_WIDTH = 16;
static const int BLUR_HEIGHT = 16;
static const int ARRAY_WIDTH = HALF_TILE_WIDTH * BLUR_WIDTH + 2;
static const int ARRAY_HEIGHT = TILE_HEIGHT * BLUR_HEIGHT + 2;

groupshared uint s_blur_front[ARRAY_WIDTH][ARRAY_HEIGHT];
groupshared uint s_blur_back[ARRAY_WIDTH][ARRAY_HEIGHT];

struct Edges4
{
    float4 left;
    float4 right;
    float4 top;
    float4 bottom;
};

Edges4 unpack_edges_4(float4 packed_value)
{
    const uint4 packed = uint4(packed_value * 255.5f);
    Edges4 result;
    result.left = saturate(float4((packed >> 6) & 0x03) / 3.0f + inv_sharpness());
    result.right = saturate(float4((packed >> 4) & 0x03) / 3.0f + inv_sharpness());
    result.top = saturate(float4((packed >> 2) & 0x03) / 3.0f + inv_sharpness());
    result.bottom = saturate(float4((packed >> 0) & 0x03) / 3.0f + inv_sharpness());
    return result;
}

float4 calc_blurred_sample_4(float4 packed_edges, float4 centre, float4 left, float4 right, float4 top, float4 bottom)
{
    float4 sum = centre * 0.5f;
    float4 weight = float4(0.5f, 0.5f, 0.5f, 0.5f);
    const Edges4 edges = unpack_edges_4(packed_edges);

    sum += left * edges.left;
    weight += edges.left;
    sum += right * edges.right;
    weight += edges.right;
    sum += top * edges.top;
    weight += edges.top;
    sum += bottom * edges.bottom;
    weight += edges.bottom;

    return sum / weight;
}

// The ping buffer through point-mirror sampling at a texel's center: a mirrored load
float2 sample_blur_input(int2 coord, uint layer)
{
    const int2 size = int2(buffer_cacao.ssao_size.xy);
    return cacao_ssao.Load(int4(mirror_coord(coord.x, size.x), mirror_coord(coord.y, size.y), int(layer), 0));
}

void store_blur_output(int2 coord, float2 value, uint layer)
{
    if (all(coord >= int2(0, 0)) && all(coord < int2(buffer_cacao.ssao_size.xy)))
        cacao_ssao_out[uint3(uint2(coord), layer)] = value;
}

[shader("compute")]
[numthreads(16, 16, 1)]
void main_comp(uint3 tid3: SV_GroupThreadID, uint3 gid: SV_GroupID)
{
    const uint2 tid = tid3.xy;
    const uint layer = gid.z;
    const uint blur_passes = min(uint(buffer_cacao.bilateral.z + 0.5f), 8u);

    const int2 image_coord = int2(gid.xy) * (int2(TILE_WIDTH * BLUR_WIDTH, TILE_HEIGHT * BLUR_HEIGHT) - int(2 * blur_passes)) +
                             int2(TILE_WIDTH, TILE_HEIGHT) * int2(tid) - int(blur_passes);
    const int2 buffer_coord = int2(HALF_TILE_WIDTH, TILE_HEIGHT) * int2(tid) + 1;

    float4 packed_edges[QUARTER_TILE_WIDTH][TILE_HEIGHT];
    {
        float2 input_val[TILE_WIDTH][TILE_HEIGHT];
        [unroll]
        for (int y = 0; y < TILE_HEIGHT; ++y)
        {
            [unroll]
            for (int x = 0; x < TILE_WIDTH; ++x)
                input_val[x][y] = sample_blur_input(image_coord + int2(x, y), layer);
        }
        [unroll]
        for (int y = 0; y < TILE_HEIGHT; ++y)
        {
            [unroll]
            for (int x = 0; x < QUARTER_TILE_WIDTH; ++x)
            {
                float2 ssao_vals = float2(input_val[4 * x + 0][y].x, input_val[4 * x + 1][y].x);
                s_blur_front[buffer_coord.x + 2 * x + 0][buffer_coord.y + y] = pack_f32(ssao_vals);
                ssao_vals = float2(input_val[4 * x + 2][y].x, input_val[4 * x + 3][y].x);
                s_blur_front[buffer_coord.x + 2 * x + 1][buffer_coord.y + y] = pack_f32(ssao_vals);
                packed_edges[x][y] = float4(input_val[4 * x + 0][y].y, input_val[4 * x + 1][y].y, input_val[4 * x + 2][y].y, input_val[4 * x + 3][y].y);
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    for (uint i = 0; i < (blur_passes + 1) / 2; ++i)
    {
        [unroll]
        for (int y = 0; y < TILE_HEIGHT; ++y)
        {
            [unroll]
            for (int x = 0; x < QUARTER_TILE_WIDTH; ++x)
            {
                const int2 c = buffer_coord + int2(2 * x, y);
                const float4 centre = float4(unpack_f32(s_blur_front[c.x + 0][c.y + 0]), unpack_f32(s_blur_front[c.x + 1][c.y + 0]));
                const float4 top = float4(unpack_f32(s_blur_front[c.x + 0][c.y - 1]), unpack_f32(s_blur_front[c.x + 1][c.y - 1]));
                const float4 bottom = float4(unpack_f32(s_blur_front[c.x + 0][c.y + 1]), unpack_f32(s_blur_front[c.x + 1][c.y + 1]));

                float2 tmp = unpack_f32(s_blur_front[c.x - 1][c.y + 0]);
                const float4 left = float4(tmp.y, centre.xyz);
                tmp = unpack_f32(s_blur_front[c.x + 2][c.y + 0]);
                const float4 right = float4(centre.yzw, tmp.x);

                const float4 tmp_4 = calc_blurred_sample_4(packed_edges[x][y], centre, left, right, top, bottom);
                s_blur_back[c.x + 0][c.y] = pack_f32(tmp_4.xy);
                s_blur_back[c.x + 1][c.y] = pack_f32(tmp_4.zw);
            }
        }
        GroupMemoryBarrierWithGroupSync();

        if (2 * i + 1 < blur_passes)
        {
            [unroll]
            for (int y = 0; y < TILE_HEIGHT; ++y)
            {
                [unroll]
                for (int x = 0; x < QUARTER_TILE_WIDTH; ++x)
                {
                    const int2 c = buffer_coord + int2(2 * x, y);
                    const float4 centre = float4(unpack_f32(s_blur_back[c.x + 0][c.y + 0]), unpack_f32(s_blur_back[c.x + 1][c.y + 0]));
                    const float4 top = float4(unpack_f32(s_blur_back[c.x + 0][c.y - 1]), unpack_f32(s_blur_back[c.x + 1][c.y - 1]));
                    const float4 bottom = float4(unpack_f32(s_blur_back[c.x + 0][c.y + 1]), unpack_f32(s_blur_back[c.x + 1][c.y + 1]));

                    float2 tmp = unpack_f32(s_blur_back[c.x - 1][c.y + 0]);
                    const float4 left = float4(tmp.y, centre.xyz);
                    tmp = unpack_f32(s_blur_back[c.x + 2][c.y + 0]);
                    const float4 right = float4(centre.yzw, tmp.x);

                    const float4 tmp_4 = calc_blurred_sample_4(packed_edges[x][y], centre, left, right, top, bottom);
                    s_blur_front[c.x + 0][c.y] = pack_f32(tmp_4.xy);
                    s_blur_front[c.x + 1][c.y] = pack_f32(tmp_4.zw);
                }
            }
            GroupMemoryBarrierWithGroupSync();
        }
    }

    [unroll]
    for (uint y = 0; y < TILE_HEIGHT; ++y)
    {
        const uint output_y = TILE_HEIGHT * tid.y + y;
        if (blur_passes <= output_y && output_y < TILE_HEIGHT * BLUR_HEIGHT - blur_passes)
        {
            [unroll]
            for (int x = 0; x < QUARTER_TILE_WIDTH; ++x)
            {
                uint output_x = TILE_WIDTH * tid.x + 4 * x;

                float4 ssao_val;
                if (blur_passes % 2 == 0)
                    ssao_val = float4(unpack_f32(s_blur_front[buffer_coord.x + x][buffer_coord.y + y]), unpack_f32(s_blur_front[buffer_coord.x + x + 1][buffer_coord.y + y]));
                else
                    ssao_val = float4(unpack_f32(s_blur_back[buffer_coord.x + x][buffer_coord.y + y]), unpack_f32(s_blur_back[buffer_coord.x + x + 1][buffer_coord.y + y]));

                if (blur_passes <= output_x && output_x < TILE_WIDTH * BLUR_WIDTH - blur_passes)
                    store_blur_output(image_coord + int2(4 * x, y), float2(ssao_val.x, packed_edges[x][y].x), layer);
                output_x += 1;
                if (blur_passes <= output_x && output_x < TILE_WIDTH * BLUR_WIDTH - blur_passes)
                    store_blur_output(image_coord + int2(4 * x + 1, y), float2(ssao_val.y, packed_edges[x][y].y), layer);
                output_x += 1;
                if (blur_passes <= output_x && output_x < TILE_WIDTH * BLUR_WIDTH - blur_passes)
                    store_blur_output(image_coord + int2(4 * x + 2, y), float2(ssao_val.z, packed_edges[x][y].z), layer);
                output_x += 1;
                if (blur_passes <= output_x && output_x < TILE_WIDTH * BLUR_WIDTH - blur_passes)
                    store_blur_output(image_coord + int2(4 * x + 3, y), float2(ssao_val.w, packed_edges[x][y].w), layer);
            }
        }
    }
}

#endif

// =====================================================================================================================
// Output: apply (ffx_cacao_apply.h) and bilateral upscale (ffx_cacao_upscale.h)
// =====================================================================================================================

#if defined(CACAO_APPLY) || defined(CACAO_UPSCALE)

// ssao/exterior_strength and exterior_curve on the visibility
float finish_ao(float ao)
{
    return lerp(1.0f, pow(saturate(ao), buffer_cacao.bilateral.w), buffer_cacao.output.x);
}

void store_output(int2 coord, float ao)
{
    if (all(coord < int2(buffer_cacao.output_size.xy)))
        tex_uav[uint2(coord)] = float4(finish_ao(ao), 0.0f, 0.0f, 1.0f);
}

float sample_ssao_linear(float2 uv, uint layer)
{
    return cacao_ssao.SampleLevel(samplers[sampler_bilinear_clamp], float3(uv, float(layer)), 0.0f).x;
}

float sample_ssao_point(float2 uv, uint layer)
{
    return cacao_ssao.SampleLevel(samplers[sampler_point_clamp], float3(uv, float(layer)), 0.0f).x;
}

float2 load_ssao(int2 coord, uint layer)
{
    const int2 size = int2(buffer_cacao.ssao_size.xy);
    return cacao_ssao.Load(int4(clamp(coord, int2(0, 0), size - 1), int(layer), 0));
}

// The occlusion at a half-resolution pixel from its own layer and the three others, pushed away from edges
float smart_reinterleave(float2 center_value, int2 pix_pos)
{
    const int mx = pix_pos.x & 1;
    const int my = pix_pos.y & 1;
    const int ih = (1 - mx) + my * 2;		// neighbouring, horizontal
    const int iv = mx + (1 - my) * 2;		// neighbouring, vertical
    const int id = (1 - mx) + (1 - my) * 2;	// diagonal

    float ao = center_value.x;
    const float4 edges_lrtb = unpack_edges(center_value.y);

    // convert index shifts to sampling offsets
    const float fmx = float(mx);
    const float fmy = float(my);

    // in case of an edge, push sampling offsets away from the edge (towards pixel center)
    const float fmxe = edges_lrtb.y - edges_lrtb.x;
    const float fmye = edges_lrtb.w - edges_lrtb.z;

    // calculate final sampling offsets and sample using bilinear filter
    const float2 p = float2(pix_pos);
    const float2 uv_h = (p + float2(fmx + fmxe - 0.5f, 0.5f - fmy)) * 0.5f * ssao_inverse_size();
    const float ao_h = sample_ssao_linear(uv_h, ih);
    const float2 uv_v = (p + float2(0.5f - fmx, fmy - 0.5f + fmye)) * 0.5f * ssao_inverse_size();
    const float ao_v = sample_ssao_linear(uv_v, iv);
    const float2 uv_d = (p + float2(fmx - 0.5f + fmxe, fmy - 0.5f + fmye)) * 0.5f * ssao_inverse_size();
    const float ao_d = sample_ssao_linear(uv_d, id);

    // reduce weight for samples near edge - if the edge is on both sides, weight goes to 0
    float4 blend_weights;
    blend_weights.x = 1.0f;
    blend_weights.y = (edges_lrtb.x + edges_lrtb.y) * 0.5f;
    blend_weights.z = (edges_lrtb.z + edges_lrtb.w) * 0.5f;
    blend_weights.w = (blend_weights.y + blend_weights.z) * 0.5f;

    // calculate weighted average
    const float blend_weights_sum = dot(blend_weights, float4(1.0f, 1.0f, 1.0f, 1.0f));
    return dot(float4(ao, ao_h, ao_v, ao_d), blend_weights) / blend_weights_sum;
}

#endif

#if defined(CACAO_APPLY)

[shader("compute")]
[numthreads(8, 8, 1)]
void main_comp(uint3 tid: SV_DispatchThreadID)
{
    const int2 pix_pos = int2(tid.xy);
    const int ic = (pix_pos.x & 1) + (pix_pos.y & 1) * 2;	// center index
    const float2 center_value = load_ssao(pix_pos / 2, ic);
    const float ao = smart_reinterleave(center_value, pix_pos);
    store_output(pix_pos, pow(ao, 2.2f));
}

#endif

#if defined(CACAO_UPSCALE)

static const int UPSCALE_WIDTH = 8;
static const int UPSCALE_HEIGHT = 8;
static const int UPSCALE_BUFFER_WIDTH = UPSCALE_WIDTH + 4;
static const int UPSCALE_BUFFER_HEIGHT = UPSCALE_HEIGHT + 4 + 4;

struct BilateralBufferVal
{
    uint packed_depths;
    uint packed_ssao_vals;
};

groupshared BilateralBufferVal s_bilateral_upscale_buffer[UPSCALE_BUFFER_WIDTH][UPSCALE_BUFFER_HEIGHT];

float load_downscaled_depth(int2 coord, uint layer)
{
    const int2 size = int2(buffer_cacao.deinterleaved_size.xy);
    return cacao_depth.Load(int4(clamp(coord, int2(0, 0), size - 1), int(layer), 0));
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main_comp(uint3 tid3: SV_DispatchThreadID, uint3 gtid3: SV_GroupThreadID, uint3 gid3: SV_GroupID)
{
    const int width = 2;
    const int height = 2;
    const uint2 tid = tid3.xy;
    const uint2 gtid = gtid3.xy;
    const uint2 gid = gid3.xy;

    // fill in group shared buffer: the half-resolution AO (the 4 layers reinterleaved) and its view depth around the group
    {
        const uint thread_num = (gtid.y * UPSCALE_WIDTH + gtid.x) * 3;
        const uint2 buffer_coord = uint2(thread_num % UPSCALE_BUFFER_WIDTH, thread_num / UPSCALE_BUFFER_WIDTH);
        int2 image_coord = int2(gid * uint2(UPSCALE_WIDTH, UPSCALE_HEIGHT)) + int2(buffer_coord) - 2;

#if CACAO_SMART
        float2 inputs[3];
        for (int j = 0; j < 3; ++j)
        {
            const int2 p = int2(image_coord.x + j, image_coord.y);
            inputs[j] = load_ssao(p >> 1, uint((p.x & 1) + 2 * (p.y & 1)));
        }

        for (int i = 0; i < 3; ++i)
        {
            const int ic = (image_coord.x & 1) + (image_coord.y & 1) * 2;	// center index
            const float ao = smart_reinterleave(inputs[i], image_coord);

            ++image_coord.x;

            // As in AMD's code, the depth comes from the next pixel's coordinate with this one's layer
            BilateralBufferVal buffer_val;
            const float depth = load_downscaled_depth(image_coord >> 1, ic);
            buffer_val.packed_depths = pack_f32(float2(depth, depth));
            buffer_val.packed_ssao_vals = pack_f32(float2(ao, ao));
            s_bilateral_upscale_buffer[buffer_coord.x + i][buffer_coord.y] = buffer_val;
        }
#else
        for (int i = 0; i < 3; ++i)
        {
            const float2 inv = ssao_inverse_size();
            float2 sample_loc0 = (float2(image_coord >> 1) + 0.5f) * inv;
            float2 sample_loc1 = sample_loc0;
            float2 sample_loc2 = sample_loc0;
            float2 sample_loc3 = sample_loc0;
            switch ((image_coord.y & 1) * 2 + (image_coord.x & 1))
            {
            case 0:
                sample_loc1.x -= 0.5f * inv.x;
                sample_loc2.y -= 0.5f * inv.y;
                sample_loc3 -= 0.5f * inv;
                break;
            case 1:
                sample_loc0.x += 0.5f * inv.x;
                sample_loc2 += float2(0.5f, -0.5f) * inv;
                sample_loc3.y -= 0.5f * inv.y;
                break;
            case 2:
                sample_loc0.y += 0.5f * inv.y;
                sample_loc1 += float2(-0.5f, 0.5f) * inv;
                sample_loc3.x -= 0.5f * inv.x;
                break;
            default:
                sample_loc0 += 0.5f * inv;
                sample_loc1.y += 0.5f * inv.y;
                sample_loc2.x += 0.5f * inv.x;
                break;
            }

            const float ssao_val0 = sample_ssao_point(sample_loc0, 0);
            const float ssao_val1 = sample_ssao_point(sample_loc1, 1);
            const float ssao_val2 = sample_ssao_point(sample_loc2, 2);
            const float ssao_val3 = sample_ssao_point(sample_loc3, 3);

            const int2 depth_coord = image_coord >> 1;
            const uint depth_layer = uint(2 * (image_coord.y & 1) + (image_coord.x & 1));
            ++image_coord.x;

            BilateralBufferVal buffer_val;
            const float depth = load_downscaled_depth(depth_coord, depth_layer);
            const float ssao_val = (ssao_val0 + ssao_val1 + ssao_val2 + ssao_val3) * 0.25f;
            buffer_val.packed_depths = pack_f32(float2(depth, depth));
            buffer_val.packed_ssao_vals = pack_f32(float2(ssao_val, ssao_val));
            s_bilateral_upscale_buffer[buffer_coord.x + i][buffer_coord.y] = buffer_val;
        }
#endif
    }

    GroupMemoryBarrierWithGroupSync();

    // the four full-resolution depths this thread writes
    float depths[4];
    {
        const int2 full_buffer_coord = int2(2 * tid);
        depths[0] = screen_to_view_depth(load_screen_depth(full_buffer_coord + int2(0, 0)));
        depths[1] = screen_to_view_depth(load_screen_depth(full_buffer_coord + int2(1, 0)));
        depths[2] = screen_to_view_depth(load_screen_depth(full_buffer_coord + int2(0, 1)));
        depths[3] = screen_to_view_depth(load_screen_depth(full_buffer_coord + int2(1, 1)));
    }

    const float4 packed_depths = float4(depths[0], depths[1], depths[2], depths[3]);

    const int2 base_buffer_coord = int2(gtid) + int2(width, height);

    const float epsilon_weight = 1e-3f;
    const float2 nearest_ssao_vals = unpack_f32(s_bilateral_upscale_buffer[base_buffer_coord.x][base_buffer_coord.y].packed_ssao_vals);
    float4 packed_totals = epsilon_weight * float4(1.0f, 1.0f, 1.0f, 1.0f);
    packed_totals.xy *= nearest_ssao_vals;
    packed_totals.zw *= nearest_ssao_vals;
    float4 packed_total_weights = epsilon_weight * float4(1.0f, 1.0f, 1.0f, 1.0f);

    const float distance_sigma = buffer_cacao.bilateral.y;
    const float2 packed_dist_sigma = float2(1.0f / distance_sigma, 1.0f / distance_sigma);
    const float sigma = buffer_cacao.bilateral.x;
    const float2 packed_sigma = float2(1.0f / sigma, 1.0f / sigma);

    for (int x = -width; x <= width; ++x)
    {
        for (int y = -height; y <= height; ++y)
        {
            const int2 buffer_coord = base_buffer_coord + int2(x, y);
            const BilateralBufferVal buffer_val = s_bilateral_upscale_buffer[buffer_coord.x][buffer_coord.y];

            float2 u = float2(x, x) - float2(0.0f, 0.5f);
            float2 v1 = float2(y, y) - float2(0.0f, 0.0f);
            float2 v2 = float2(y, y) - float2(0.5f, 0.5f);
            u = u * u;
            v1 = v1 * v1;
            v2 = v2 * v2;

            const float2 dist1 = u + v1;
            const float2 dist2 = u + v2;

            const float2 wx1 = exp(-dist1 * packed_sigma);
            const float2 wx2 = exp(-dist2 * packed_sigma);

            const float2 buffer_packed_depths = unpack_f32(buffer_val.packed_depths);

            float2 diff1 = packed_depths.xy - buffer_packed_depths;
            float2 diff2 = packed_depths.zw - buffer_packed_depths;
            diff1 *= diff1;
            diff2 *= diff2;

            const float2 wy1 = exp(-diff1 * packed_dist_sigma);
            const float2 wy2 = exp(-diff2 * packed_dist_sigma);

            const float2 weight1 = wx1 * wy1;
            const float2 weight2 = wx2 * wy2;

            const float2 packed_ssao_vals = unpack_f32(buffer_val.packed_ssao_vals);
            packed_totals.xy += packed_ssao_vals * weight1;
            packed_totals.zw += packed_ssao_vals * weight2;
            packed_total_weights.xy += weight1;
            packed_total_weights.zw += weight2;
        }
    }

    const int2 output_coord = int2(2 * tid);
    float4 output_values = packed_totals / packed_total_weights;
    output_values = pow(output_values, float4(2.2f, 2.2f, 2.2f, 2.2f));

    store_output(output_coord + int2(0, 0), output_values.x);
    store_output(output_coord + int2(1, 0), output_values.y);
    store_output(output_coord + int2(0, 1), output_values.z);
    store_output(output_coord + int2(1, 1), output_values.w);
}

#endif
