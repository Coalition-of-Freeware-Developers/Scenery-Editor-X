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
 * exposure_fusion.slang
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 *
 * Local tone mapping by exposure fusion (plan TASK-040), after Bart Wronski, "Local tone mapping using exposure
 * fusion" (2022), which follows Mertens, Kautz and Van Reeth, "Exposure Fusion" (2007). At half resolution:
 *   EXPOSURES  three synthetic exposures of the exposed luminance (shadows lifted by exposure_fusion.x EV, neutral,
 *              highlights lowered by post.y EV) through a simple display curve, and their well-exposedness weights
 *              (a Gaussian around mid-grey of width exposure_fusion.y), normalized per pixel.
 *              tex frame_render -> tex_uav exposures (rgb, a: neutral exposed luminance), tex_uav2 weights
 *   DOWNSAMPLE the Gaussian pyramids of both, one level: tex, tex2 level k -> tex_uav, tex_uav2 level k + 1
 *   COLLAPSE   the blended Laplacian pyramid, coarsest level first (pass_get_f4_value().x = 1 there):
 *              tex exposures k, tex3 exposures k + 1, tex2 weights k, tex4 result k + 1 -> tex_uav result k
 *   GUIDE      the gain the fused image asks for, log2 of fused / neutral exposed luminance, to the R16F guide
 *              tex result 0, tex2 exposures 0 -> tex_uav exposure_fusion_guide
 * tonemap.slang upsamples the guide, edge-aware, and applies it before the tone curve.
 */

import resources;
import constants;

static const float3 LUMA = float3(0.2126f, 0.7152f, 0.0722f);
static const float CURVE_KEY = 0.18f;	// the display curve maps exposed mid-grey to 0.5

float display_curve(float x) { return x / (x + CURVE_KEY); }
float display_curve_inverse(float y) { y = clamp(y, 0.0f, 0.999f); return CURVE_KEY * y / (1.0f - y); }

// The exposed (residual applied) luminance of frame_render around uv
float exposed_luminance(float2 uv)
{
    const float residual = exposure_scale_from_ev(exposure_state[EXPOSURE_STATE_EV]) / max(get_pre_exposure(), 1e-30f);
    return dot(tex.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0.0f).rgb, LUMA) * residual;
}

// A 4-tap tent: bilinear samples half a texel of the output around its centre (16 input texels at 2:1)
float4 downsample(Texture2D source, float2 uv, float2 texel_out)
{
    const float2 d = texel_out * 0.25f;
    return 0.25f * (source.SampleLevel(samplers[sampler_bilinear_clamp], uv + float2(-d.x, -d.y), 0.0f) +
                    source.SampleLevel(samplers[sampler_bilinear_clamp], uv + float2( d.x, -d.y), 0.0f) +
                    source.SampleLevel(samplers[sampler_bilinear_clamp], uv + float2(-d.x,  d.y), 0.0f) +
                    source.SampleLevel(samplers[sampler_bilinear_clamp], uv + float2( d.x,  d.y), 0.0f));
}

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id: SV_DispatchThreadID)
{
    float2 resolution_out;
    tex_uav.GetDimensions(resolution_out.x, resolution_out.y);
    if (any(thread_id.xy >= uint2(resolution_out)))
        return;
    const float2 uv = (thread_id.xy + 0.5f) / resolution_out;

#if EXPOSURES
    // The 2x2 average of the full-resolution frame (bilinear at the shared corner)
    const float luminance = max(exposed_luminance(uv), 0.0f);
    const float shadows = exp2(max(buffer_environment.exposure_fusion.x, 0.0f));
    const float highlights = exp2(-max(buffer_environment.post.y, 0.0f));
    const float3 exposures = float3(display_curve(luminance * shadows), display_curve(luminance), display_curve(luminance * highlights));

    const float sigma = max(buffer_environment.exposure_fusion.y, 0.01f);
    float3 weights = exp(-(exposures - 0.5f) * (exposures - 0.5f) / (2.0f * sigma * sigma)) + 1e-4f;
    weights /= weights.x + weights.y + weights.z;

    tex_uav[thread_id.xy] = float4(exposures, luminance);
    tex_uav2[thread_id.xy] = float4(weights, 1.0f);
#elif DOWNSAMPLE
    const float2 texel_out = 1.0f / resolution_out;
    tex_uav[thread_id.xy] = downsample(tex, uv, texel_out);
    tex_uav2[thread_id.xy] = downsample(tex2, uv, texel_out);
#elif COLLAPSE
    const float3 gaussian = tex.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0.0f).rgb;
    const float3 weights = tex2.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0.0f).rgb;
    if (pass_get_f4_value().x > 0.5f)
    {
        // The coarsest level: the weighted Gaussians
        tex_uav[thread_id.xy] = float4(dot(gaussian, weights), 0.0f, 0.0f, 1.0f);
        return;
    }
    const float3 coarser = tex3.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0.0f).rgb;
    const float laplacian = dot(gaussian - coarser, weights);
    const float result = tex4.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0.0f).r + laplacian;
    tex_uav[thread_id.xy] = float4(result, 0.0f, 0.0f, 1.0f);
#elif GUIDE
    const float fused = tex.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0.0f).r;
    const float neutral = tex2.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0.0f).a;
    const float limit = max(buffer_environment.exposure_fusion.x, 0.0f) + max(buffer_environment.post.y, 0.0f);
    const float gain = neutral > 1e-6f ? log2(max(display_curve_inverse(fused), 1e-9f) / neutral) : 0.0f;
    tex_uav[thread_id.xy] = float4(clamp(gain, -limit, limit), 0.0f, 0.0f, 1.0f);
#endif
}
