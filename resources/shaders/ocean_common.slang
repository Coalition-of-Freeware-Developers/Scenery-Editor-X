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
 * ocean_common.slang
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 *
 * The FFT ocean's shared half (plan Phase 11, TASK-072; XP12: ocean_spectrum, compute_fft, ocean_bake_textures): the
 * sea state and cascades from buffer_environment (Renderer::UpdateOcean, renderer/ocean.h), the wave spectrum (the GPU twin
 * of renderer/ocean.cpp: keep the two alike), and how the water samples the baked cascades. Imported by ocean.slang (the
 * compute passes) and water.slang (the surface).
 *
 * Cascade c of four is a 256 x 256 tile of waves sizeM meters across, holding the wave numbers kMin <= |k| < kMax
 * (renderer/ocean.h ComputeOceanCascades). The baked textures, one array layer per cascade, with mips:
 *   tex_ocean_displacement  rgb displacement (m: choppy x, height, choppy z), a foam (0-1, kept and fading over time)
 *   tex_ocean_slopes        x, z slopes of the height and their squares: mips average them, so a mip's variance is
 *                           E[s^2] - E[s]^2 (LEAN mapping), the waves too small to see as normals, read as roughness
 * Every shader that imports this declares the textures (Slang keeps unused globals): passes bind them (Renderer::BindOcean).
 */
module ocean_common;

import resources;

public [[vk::binding(71)]] Texture2DArray tex_ocean_displacement : register(t71);
public [[vk::binding(72)]] Texture2DArray tex_ocean_slopes : register(t72);
// Typed (the targets' formats, renderer_resources.cpp): reads and writes take the fast path, not the format-less one
public [[vk::binding(113)]] [[vk::image_format("rgba32f")]] RWTexture2DArray<float4> ocean_uav_a : register(u113);
public [[vk::binding(114)]] [[vk::image_format("rgba32f")]] RWTexture2DArray<float4> ocean_uav_b : register(u114);
public [[vk::binding(115)]] [[vk::image_format("rgba16f")]] RWTexture2DArray<float4> ocean_uav_c : register(u115);
public [[vk::binding(116)]] [[vk::image_format("rgba16f")]] RWTexture2DArray<float4> ocean_uav_d : register(u116);

public static const uint OCEAN_SIZE = 256;			// renderer/ocean.h OCEAN_FFT_SIZE
public static const uint OCEAN_CASCADES = 4;
public static const uint OCEAN_FOAM_CASCADES = 2;	// Cascades whose breaking makes foam (the long waves' whitecaps)
public static const float OCEAN_GRAVITY = 9.81f;
public static const float OCEAN_PI = 3.14159265358979f;

// RendererFeatures bit (renderer_features.cpp table order)
public static const uint FEATURE_WATER = 14;

/* @brief Cascade c: tile size (m), first and past-last wave number (rad/m), water/cascade_N_scale (in the spectrum's amplitudes). */
public float4 ocean_cascade(uint c) { return buffer_environment.ocean_cascade[c]; }
public float ocean_time() { return buffer_environment.ocean.x; }
public float ocean_choppiness() { return buffer_environment.ocean.y; }

// -------------------------------------------------------
// The spectrum (renderer/ocean.cpp)

/* @brief w^2 = g k tanh(k d). */
public float ocean_dispersion(float k, float depth)
{
    return sqrt(OCEAN_GRAVITY * k * tanh(min(k * depth, 20.0f)));
}

/* @brief dw / dk. */
public float ocean_dispersion_derivative(float k, float depth)
{
    const float kd = min(k * depth, 20.0f);
    const float th = tanh(kd);
    const float ch = cosh(kd);
    const float omega = sqrt(OCEAN_GRAVITY * k * th);
    return omega > 0.0f ? OCEAN_GRAVITY * (th + kd / (ch * ch)) / (2.0f * omega) : 0.0f;
}

// Sea state (buffer_environment): wind speed, swell, spread blend, short waves fade; alpha, peak omega, gamma, depth
float sea_wind() { return buffer_environment.ocean_sea.x; }
float sea_swell() { return buffer_environment.ocean_sea.y; }
float sea_spread_blend() { return buffer_environment.ocean_sea.z; }
float sea_short_waves_fade() { return buffer_environment.ocean_sea.w; }
float sea_alpha() { return buffer_environment.ocean_jonswap.x; }
float sea_peak_omega() { return buffer_environment.ocean_jonswap.y; }
float sea_gamma() { return buffer_environment.ocean_jonswap.z; }
public float sea_depth() { return buffer_environment.ocean_jonswap.w; }

float jonswap(float omega)
{
    const float wp = sea_peak_omega();
    if (omega <= 0.0f || wp <= 0.0f)
        return 0.0f;
    const float sigma = omega <= wp ? 0.07f : 0.09f;
    const float d = omega - wp;
    const float r = exp(-(d * d) / (2.0f * sigma * sigma * wp * wp));
    const float ratio = wp / omega;
    return sea_alpha() * OCEAN_GRAVITY * OCEAN_GRAVITY / pow(omega, 5.0f) * exp(-1.25f * ratio * ratio * ratio * ratio) * pow(sea_gamma(), r);
}

float tma(float omega, float depth)
{
    const float wh = omega * sqrt(depth / OCEAN_GRAVITY);
    if (wh <= 1.0f)
        return 0.5f * wh * wh;
    if (wh < 2.0f)
        return 1.0f - 0.5f * (2.0f - wh) * (2.0f - wh);
    return 1.0f;
}

// ln Gamma(x), x >= 1: shifted up by 6, then Stirling's series
float log_gamma(float x)
{
    const float shift = log(x * (x + 1.0f) * (x + 2.0f) * (x + 3.0f) * (x + 4.0f) * (x + 5.0f));
    const float z = x + 6.0f;
    const float inv = 1.0f / z;
    const float inv2 = inv * inv;
    const float series = (z - 0.5f) * log(z) - z + 0.918938533f + inv * (1.0f / 12.0f - inv2 * (1.0f / 360.0f - inv2 / 1260.0f));
    return series - shift;
}

/* @brief Hasselmann's cos-2s spreading with Horvath's swell term, blended toward uniform (renderer/ocean.cpp). */
float directional_spread(float omega, float theta)
{
    const float uniform = 1.0f / (2.0f * OCEAN_PI);
    const float wp = sea_peak_omega();
    const float blend = sea_spread_blend();
    if (blend <= 0.0f || omega <= 0.0f || wp <= 0.0f)
        return uniform;

    const float ratio = omega / wp;
    float s = omega <= wp ? 6.97f * pow(ratio, 4.06f) : 9.77f * pow(ratio, -2.33f - 1.45f * (sea_wind() * wp / OCEAN_GRAVITY - 1.17f));
    s += 16.0f * tanh(wp / omega) * sea_swell() * sea_swell();

    const float log_q = (2.0f * s - 1.0f) * 0.693147181f - 1.144729886f + 2.0f * log_gamma(s + 1.0f) - log_gamma(2.0f * s + 1.0f);
    const float c = abs(cos(0.5f * theta));
    const float directional = c > 0.0f ? exp(log_q + 2.0f * s * log(c)) : 0.0f;
    return lerp(uniform, directional, blend);
}

/* @brief E(kx, kz): height variance per unit of k-space area, without the height scale (renderer/ocean.cpp OceanSpectrumEnergy). */
public float ocean_spectrum_energy(float2 k_vector)
{
    const float k = length(k_vector);
    if (k <= 1e-6f)
        return 0.0f;
    const float depth = sea_depth();
    const float omega = ocean_dispersion(k, depth);
    const float theta = atan2(k_vector.y, k_vector.x) - buffer_environment.ocean.w;
    const float fade = exp(-k * k * sea_short_waves_fade() * sea_short_waves_fade());
    return jonswap(omega) * tma(omega, depth) * directional_spread(omega, theta) * ocean_dispersion_derivative(k, depth) / k * fade;
}

/* @brief Wave number (rad/m) of grid cell n of a cascade, in the FFT's order (n >= size / 2 negative). */
public float ocean_grid_wave_number(uint n, float size_m)
{
    const int wrapped = n < OCEAN_SIZE / 2 ? int(n) : int(n) - int(OCEAN_SIZE);
    return 2.0f * OCEAN_PI * float(wrapped) / size_m;
}

/* @brief Two independent standard normal numbers for a cell (deterministic: the same sea every run). */
public float2 ocean_gaussian(uint2 cell, uint cascade)
{
    uint state = cell.x * 1973u + cell.y * 9277u + cascade * 26699u + 0x9e3779b9u;
    const uint a = pcg(state);
    const uint b = pcg(a);
    const float u1 = (float(a >> 8) + 1.0f) / 16777217.0f;	// (0, 1]: log is finite
    const float u2 = float(b >> 8) / 16777216.0f;
    const float r = sqrt(-2.0f * log(u1));
    return float2(r * cos(2.0f * OCEAN_PI * u2), r * sin(2.0f * OCEAN_PI * u2));
}

uint pcg(uint value)
{
    const uint state = value * 747796405u + 2891336453u;
    const uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// -------------------------------------------------------
// Sampling the baked cascades (water.slang)

/* @brief Displacement (m) and foam at a ground point: the cascades summed, each at a mip for footprint_m (0: full detail). */
public float4 ocean_displacement(float2 xz, float footprint_m)
{
    float4 sum = 0.0f;
    for (uint c = 0; c < OCEAN_CASCADES; ++c)
    {
        const float4 cascade = ocean_cascade(c);
        const float texel = cascade.x / float(OCEAN_SIZE);
        const float mip = max(log2(max(footprint_m, 1e-4f) / texel), 0.0f);
        const float4 sample = tex_ocean_displacement.SampleLevel(samplers[sampler_bilinear_wrap], float3(xz / cascade.x, float(c)), mip);
        sum.xyz += sample.xyz;
        sum.w = max(sum.w, sample.w);
    }
    return sum;
}

/**
 * @brief The slopes at a ground point, filtered over the pixel's footprint (dx, dy: how far the ground point moves per
 * pixel across and down the screen, m): xy the mean x / z slope of every cascade, zw the slope variance the footprint
 * averages away (LEAN: the mips' mean square minus the mean squared). The footprint comes from the caller, not the
 * rasterizer: tessellated triangles of different shapes meet at every cell edge, where hardware derivatives jump.
 */
public float4 ocean_slopes(float2 xz, float2 dx, float2 dy)
{
    return ocean_slopes(xz, dx, dy, OCEAN_CASCADES);
}

/* @brief ocean_slopes over the first cascades only (the longer waves). */
public float4 ocean_slopes(float2 xz, float2 dx, float2 dy, uint cascades)
{
    float2 mean = 0.0f;
    float2 variance = 0.0f;
    for (uint c = 0; c < min(cascades, OCEAN_CASCADES); ++c)
    {
        const float4 cascade = ocean_cascade(c);
        const float4 sample = tex_ocean_slopes.SampleGrad(samplers[sampler_anisotropic_wrap], float3(xz / cascade.x, float(c)), dx / cascade.x, dy / cascade.x);
        mean += sample.xy;
        variance += max(sample.zw - sample.xy * sample.xy, 0.0f);
    }
    return float4(mean, variance);
}
