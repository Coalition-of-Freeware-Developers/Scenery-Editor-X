/*
-------------------------------------------------------
Scenery Editor X
-------------------------------------------------------
Copyright (c) 2025 Thomas Ray
Copyright (c) 2025 Coalition of Freeware Developers
-------------------------------------------------------
fxaa.slang
-------------------------------------------------------
Created: 18/04/2026
-------------------------------------------------------
*/
import common;
#define FXAA_PC 1
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 39
#define FXAA_GREEN_AS_LUMA 1

static const float g_fxaa_subPix           = 0.9f;    // the amount of sub-pixel aliasing removal. This can effect sharpness.
static const float g_fxaa_edgeThreshold    = 0.063f;  // the minimum amount of local contrast required to apply algorithm.
static const float g_fxaa_edgeThresholdMin = 0.0312f; // trims the algorithm from processing darks

[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
[shader("compute")]
void main_comp(uint3 thread_id : SV_DispatchThreadID)
{
    float2 resolution_out;
    tex_uav.GetDimensions(resolution_out.x, resolution_out.y);
    if (any(int2(thread_id.xy) >= resolution_out))
        return;

    const float2 pos = (thread_id.xy + 0.5f) / resolution_out;
    FxaaTex fxaa_tex = { samplers[sampler_bilinear_clamp], tex };
    float2 texl_size = 1.0f / resolution_out;

    float3 color = FxaaPixelShader
    (
        pos, 0, fxaa_tex, fxaa_tex, fxaa_tex,
        texl_size, 0, 0, 0,
        g_fxaa_subPix,
        g_fxaa_edgeThreshold,
        g_fxaa_edgeThresholdMin,
        0, 0, 0, 0
    ).rgb;

    tex_uav[thread_id.xy] = float4(color, tex[thread_id.xy].a);
}
