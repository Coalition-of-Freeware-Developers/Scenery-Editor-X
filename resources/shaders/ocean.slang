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
 * ocean.slang
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 *
 * The FFT ocean's compute passes (plan TASK-072; XP12: ocean_spectrum, compute_fft, ocean_bake_textures): every frame the
 * four cascades' waves at the ocean's time, inverse transformed and baked for the water (Renderer::Pass_Ocean). By define:
 *   OCEAN_SPECTRUM  h0 of every cell when the sea state changes: ocean_uav_a (one layer per cascade) = h0(k) in xy, conj(h0(-k))
 *                   in zw, a Gaussian amplitude per cell (renderer/ocean.cpp: 0.5 sqrt(E(k)) dk x the height scale)
 *   OCEAN_EVOLVE    the cells at the ocean's time, h = h0 e^(-iwt) + conj(h0(-k)) e^(iwt), and the seven fields the water needs
 *                   from it, packed two real fields per complex one into ocean_uav_b (layers 2c, 2c + 1):
 *                   (Dx + i Dz, h + i dh/dx) and (dh/dz + i dDx/dx, dDz/dz + i dDx/dz)
 *   OCEAN_FFT       a 256-point inverse FFT of every row (pass x 0) or column (pass x 1) of ocean_uav_b, in place: one
 *                   workgroup per line, the line bit-reversed into shared memory, eight radix-2 stages (renderer/ocean.cpp
 *                   OceanInverseFft is the CPU twin)
 *   OCEAN_BAKE      ocean_uav_c (displacement, foam) and ocean_uav_d (slopes and their squares) from the transformed fields;
 *                   foam where the waves' Jacobian falls below water/foam_bias, kept from the last frame and fading
 *   OCEAN_MIP       one mip of both from the mip above (tex_ocean_displacement / tex_ocean_slopes single-mip views): 2 x 2 means
 */
import resources;
import ocean_common;

static float2 complex_multiply(float2 a, float2 b)
{
    return float2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

// a + i b for complex a and b: the two transform to real fields in the real and imaginary parts
static float2 pack_pair(float2 a, float2 b)
{
    return float2(a.x - b.y, a.y + b.x);
}

#if defined(OCEAN_FFT)

static const uint FFT_THREADS = OCEAN_SIZE / 2;
static const uint FFT_BITS = 8;	// log2(OCEAN_SIZE)
groupshared float4 s_fft_a[OCEAN_SIZE];
groupshared float4 s_fft_b[OCEAN_SIZE];

[shader("compute")]
[numthreads(128, 1, 1)]
void main_comp(uint3 group : SV_GroupID, uint3 thread : SV_GroupThreadID)
{
    const uint line = group.y;
    const uint cascade = group.z;
    const bool columns = pass_get_f4_value().x > 0.5f;
    const uint t = thread.x;

    // The line into shared memory in bit-reversed order, two elements a thread
    for (uint e = 0; e < 2; ++e)
    {
        const uint i = t + e * FFT_THREADS;
        const uint2 texel = columns ? uint2(line, i) : uint2(i, line);
        const uint r = reversebits(i) >> (32u - FFT_BITS);
        s_fft_a[r] = ocean_uav_b[uint3(texel, cascade * 2u)];
        s_fft_b[r] = ocean_uav_b[uint3(texel, cascade * 2u + 1u)];
    }
    GroupMemoryBarrierWithGroupSync();

    // Radix-2 butterflies, one a thread per stage: twiddle e^(+2 pi i j / (2 span)), the inverse transform
    for (uint span = 1; span < OCEAN_SIZE; span *= 2u)
    {
        const uint j = t & (span - 1u);
        const uint i0 = (t / span) * 2u * span + j;
        const uint i1 = i0 + span;
        float s, c;
        sincos(OCEAN_PI * float(j) / float(span), s, c);
        const float2 w = float2(c, s);

        const float4 a0 = s_fft_a[i0];
        const float4 a1 = s_fft_a[i1];
        const float4 b0 = s_fft_b[i0];
        const float4 b1 = s_fft_b[i1];
        const float4 a1w = float4(complex_multiply(a1.xy, w), complex_multiply(a1.zw, w));
        const float4 b1w = float4(complex_multiply(b1.xy, w), complex_multiply(b1.zw, w));
        s_fft_a[i0] = a0 + a1w;
        s_fft_a[i1] = a0 - a1w;
        s_fft_b[i0] = b0 + b1w;
        s_fft_b[i1] = b0 - b1w;
        GroupMemoryBarrierWithGroupSync();
    }

    for (uint e = 0; e < 2; ++e)
    {
        const uint i = t + e * FFT_THREADS;
        const uint2 texel = columns ? uint2(line, i) : uint2(i, line);
        ocean_uav_b[uint3(texel, cascade * 2u)] = s_fft_a[i];
        ocean_uav_b[uint3(texel, cascade * 2u + 1u)] = s_fft_b[i];
    }
}

#else

[shader("compute")]
[numthreads(8, 8, 1)]
void main_comp(uint3 id : SV_DispatchThreadID)
{
    const uint cascade = id.z;
    const float4 tile = ocean_cascade(cascade);

#if defined(OCEAN_SPECTRUM) || defined(OCEAN_EVOLVE)
    if (any(id.xy >= OCEAN_SIZE))
        return;
    const float2 k = float2(ocean_grid_wave_number(id.x, tile.x), ocean_grid_wave_number(id.y, tile.x));
    const float k_length = length(k);
#endif

#if defined(OCEAN_SPECTRUM)
    float4 h0 = 0.0f;
    if (k_length > 0.0f && k_length >= tile.y && k_length < tile.z)
    {
        // The cell's variance E(k) dk^2 (and -k's) as a complex Gaussian amplitude: E|h0|^2 = E(k) dk^2 / 2
        const float dk = 2.0f * OCEAN_PI / tile.x;
        const float scale = 0.5f * dk * buffer_environment.ocean.z * tile.w;
        const uint2 minus = (OCEAN_SIZE - id.xy) % OCEAN_SIZE;
        const float2 h = ocean_gaussian(id.xy, cascade) * sqrt(ocean_spectrum_energy(k)) * scale;
        const float2 h_minus = ocean_gaussian(minus, cascade) * sqrt(ocean_spectrum_energy(-k)) * scale;
        h0 = float4(h, h_minus.x, -h_minus.y);
    }
    ocean_uav_a[uint3(id.xy, cascade)] = h0;

#elif defined(OCEAN_EVOLVE)
    float2 h = 0.0f;
    if (k_length > 0.0f)
    {
        const float4 h0 = ocean_uav_a[uint3(id.xy, cascade)];
        float s, c;
        sincos(ocean_dispersion(k_length, sea_depth()) * ocean_time(), s, c);
        h = complex_multiply(h0.xy, float2(c, -s)) + complex_multiply(h0.zw, float2(c, s));
    }
    const float2 direction = k_length > 0.0f ? k / k_length : 0.0f;
    const float inv_k = k_length > 0.0f ? 1.0f / k_length : 0.0f;
    const float2 minus_i_h = float2(h.y, -h.x);
    const float2 i_h = float2(-h.y, h.x);
    const float2 dx = direction.x * minus_i_h;			// -i kx / k h: choppy displacement
    const float2 dz = direction.y * minus_i_h;
    const float2 slope_x = k.x * i_h;					// i kx h: dh/dx
    const float2 slope_z = k.y * i_h;
    const float2 dxx = k.x * k.x * inv_k * h;			// dDx/dx
    const float2 dzz = k.y * k.y * inv_k * h;			// dDz/dz
    const float2 dxz = k.x * k.y * inv_k * h;			// dDx/dz
    ocean_uav_b[uint3(id.xy, cascade * 2u)] = float4(pack_pair(dx, dz), pack_pair(h, slope_x));
    ocean_uav_b[uint3(id.xy, cascade * 2u + 1u)] = float4(pack_pair(slope_z, dxx), pack_pair(dzz, dxz));

#elif defined(OCEAN_BAKE)
    if (any(id.xy >= OCEAN_SIZE))
        return;
    const float4 a = ocean_uav_b[uint3(id.xy, cascade * 2u)];		// Dx, Dz, h, dh/dx
    const float4 b = ocean_uav_b[uint3(id.xy, cascade * 2u + 1u)];	// dh/dz, dDx/dx, dDz/dz, dDx/dz
    const float lambda = ocean_choppiness();
    const float3 displacement = float3(lambda * a.x, a.z, lambda * a.y);
    const float2 slope = float2(a.w, b.x);

    // Where the surface folds (Jacobian of the choppy displacement below the bias) the waves break; foam stays and fades.
    // Only the long waves break into whitecaps: the two smaller cascades' ripples fold without foam.
    const float jacobian = (1.0f + lambda * b.y) * (1.0f + lambda * b.z) - lambda * lambda * b.w * b.w;
    const float4 foam_settings = buffer_environment.water_foam;	// bias, ratio, scale, fade per second
    const float fresh = cascade < OCEAN_FOAM_CASCADES ? saturate((foam_settings.x - jacobian) * foam_settings.y) : 0.0f;
    const bool reset = pass_get_f4_value().x > 0.5f;	// No history yet (first frame, the time started over)
    const float previous = reset ? 0.0f : saturate(ocean_uav_c[uint3(id.xy, cascade)].w);
    const float kept = previous * pow(saturate(1.0f - foam_settings.w), max(buffer_environment.water_tessellation.w, 0.0f));
    ocean_uav_c[uint3(id.xy, cascade)] = float4(displacement, max(fresh, isnan(kept) ? 0.0f : kept));
    ocean_uav_d[uint3(id.xy, cascade)] = float4(slope, slope * slope);

#elif defined(OCEAN_MIP)
    uint width, height, layers;
    ocean_uav_c.GetDimensions(width, height, layers);
    if (id.x >= width || id.y >= height)
        return;
    float4 displacement = 0.0f;
    float4 slopes = 0.0f;
    for (uint y = 0; y < 2; ++y)
    {
        for (uint x = 0; x < 2; ++x)
        {
            const int4 texel = int4(int(id.x * 2u + x), int(id.y * 2u + y), int(cascade), 0);
            displacement += tex_ocean_displacement.Load(texel);
            slopes += tex_ocean_slopes.Load(texel);
        }
    }
    ocean_uav_c[uint3(id.xy, cascade)] = displacement * 0.25f;
    ocean_uav_d[uint3(id.xy, cascade)] = slopes * 0.25f;
#endif
}

#endif
