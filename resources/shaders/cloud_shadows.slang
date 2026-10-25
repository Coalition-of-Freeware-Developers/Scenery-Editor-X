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
 * cloud_shadows.slang
 * -------------------------------------------------------
 * Created: 18/04/2026
 * -------------------------------------------------------
 *
 * Cloud shadows (plan TASK-055; XP12: cloud_shadows, clouds/shadow_size 1024, cloud/shadow_cascades 2, shadow_steps 5):
 * the share of the shadow light (the sun, or the moon at night) that reaches the ground through the environment's
 * three cloud layers, in world-space cascades centered on the camera (one layer of tex_uav_sss, the renderer's
 * RWTexture2DArray, per cascade; buffer_environment.cloud_shadow_cascades). Each texel is a point on the ground (y = 0,
 * the airport's elevation); its ray toward the light is marched through every layer in shadow_steps samples.
 *
 * Until the volumetric clouds (plan Phase 15) exist, a layer's cloud cover is a value-noise field thresholded so that
 * the covered share of the sky is the layer's coverage (8/8 covers everything); the light is attenuated by the cover
 * along the ray at cloud/shadow_alpha per meter (ASSUMPTION about that control's meaning). Phase 15 must draw its clouds
 * from the same cover so the shadows sit under them.
 */
import common;
import resources;
import constants;

static const uint LAYERS = 3;
static const float CELL_M = 1800.0f;		// Size of the noise's largest cells (ASSUMPTION until Phase 15's cloud shapes)
static const float MIN_THICKNESS_M = 50.0f;

float hash(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031f);
    p3 += dot(p3, p3.yzx + 33.33f);
    return frac((p3.x + p3.y) * p3.z);
}

float value_noise(float2 p)
{
    const float2 i = floor(p);
    const float2 f = frac(p);
    const float2 u = f * f * (3.0f - 2.0f * f);
    return lerp(lerp(hash(i), hash(i + float2(1.0f, 0.0f)), u.x), lerp(hash(i + float2(0.0f, 1.0f)), hash(i + float2(1.0f, 1.0f)), u.x), u.y);
}

// Four octaves, normalized: mean 0.5, roughly normal with a standard deviation near 0.12
float fbm(float2 p)
{
    float sum = 0.0f, amplitude = 0.5f;
    [unroll]
    for (uint octave = 0; octave < 4; ++octave)
    {
        sum += amplitude * value_noise(p);
        p = float2(p.x * 1.6f + p.y * 1.2f, p.y * 1.6f - p.x * 1.2f) + 17.0f;	// rotate and scale by 2
        amplitude *= 0.5f;
    }
    return sum / 0.9375f;
}

// Standard normal CDF (Abramowitz-Stegun 26.2.17 through erf's tanh approximation, good to 1e-3 here)
float normal_cdf(float x)
{
    return 0.5f * (1.0f + tanh(0.7978846f * (x + 0.044715f * x * x * x)));
}

/* @brief Cloud cover (0 .. 1) of a layer at a world xz: about `coverage` of the area is covered, with soft edges. */
float layer_cover(float2 xz, uint layer, float coverage, float type)
{
    if (coverage >= 0.999f)
        return 1.0f;
    if (coverage <= 0.001f)
        return 0.0f;

    // stratus spreads its cells wider than cumulus (type 1 vs 2); each layer its own field
    const float cell = CELL_M * lerp(1.6f, 1.0f, saturate(type - 1.0f));
    const float n = fbm(xz / cell + float2(37.0f, 11.0f) * float(layer + 1));
    const float uniform = normal_cdf((n - 0.5f) / 0.12f);	// ~uniform 0..1
    return smoothstep(1.0f - coverage - 0.06f, 1.0f - coverage + 0.06f, uniform);
}

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id : SV_DispatchThreadID)
{
    uint3 size;
    tex_uav_sss.GetDimensions(size.x, size.y, size.z);
    const uint count = min(uint(buffer_environment.cloud_shadow.x + 0.5f), size.z);
    if (any(thread_id.xy >= size.xy))
        return;

    for (uint cascade = 0; cascade < count; ++cascade)
    {
        const float4 c = buffer_environment.cloud_shadow_cascades[cascade];
        const float2 ground = c.xy + ((float2(thread_id.xy) + 0.5f) / float2(size.xy) - 0.5f) * 2.0f * c.z;

        // the light the map is for, kept off the horizon so the rays stay within reach
        const bool moon = buffer_environment.csm_params.w > 1.5f;
        float3 l = moon ? buffer_environment.moon_direction.xyz : buffer_environment.sun_direction.xyz;
        l.y = max(l.y, 0.05f);
        l = normalize(l);

        const float extinction = buffer_environment.cloud_shadow.z;
        const uint steps = max(uint(buffer_environment.cloud_shadow.w + 0.5f), 1u);
        const float ground_msl = buffer_environment.camera.y;
        float optical_depth = 0.0f;
        for (uint layer = 0; layer < LAYERS; ++layer)
        {
            const float4 cloud = buffer_environment.clouds[layer];	// base MSL, top MSL, coverage, type
            if (cloud.z <= 0.0f)
                continue;
            const float base = cloud.x - ground_msl;
            const float top = max(cloud.y - ground_msl, base + MIN_THICKNESS_M);
            if (top <= 0.0f)
                continue;

            // the ray from the ground point up through the layer, sampled at the middle of each step
            const float t0 = max(base, 0.0f) / l.y;
            const float t1 = top / l.y;
            const float step = (t1 - t0) / float(steps);
            for (uint i = 0; i < steps; ++i)
            {
                const float2 xz = ground + l.xz * (t0 + (float(i) + 0.5f) * step);
                optical_depth += layer_cover(xz, layer, cloud.z, cloud.w) * extinction * step;
            }
        }
        tex_uav_sss[uint3(thread_id.xy, cascade)] = exp(-optical_depth).xxxx;
    }
}
