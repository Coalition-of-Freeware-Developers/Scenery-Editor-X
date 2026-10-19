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
 * noise.slang
 * -------------------------------------------------------
 * Created: 07/03/2026
 * -------------------------------------------------------
 */
import resources;
import constants;

// fast 1d hash
public float hash(float p)
{
    // scale input, convert to uint for bit manipulation
    uint u = asuint(p * 3141592653.0f);

    // mix with multiply and xor, normalize to [0,1)
    return float(u * u * 3141592653u) / 4294967295.0f;
}

// fast 2d hash
public float hash(float2 p)
{
    // scale each component, convert to uint2
    uint2 u = asuint(p * float2(141421356.0f, 2718281828.0f));

    // combine with xor, mix, normalize to [0,1)
    return float((u.x ^ u.y) * 3141592653u) / 4294967295.0f;
}

public float hash(uint p)
{
    return hash((float)p);
}

public float hash(int2 p)
{
    return hash(float2(p));
}

public float noise_perlin(float x)
{
    float scale = 0.1f;
    return tex_perlin.SampleLevel(samplers[sampler_bilinear_wrap], float2(x * scale, 0.5f), 0).r * 2.0f - 1.0f;
}

public float noise_perlin(float2 x)
{
    float scale = 0.1f;
    return tex_perlin.SampleLevel(samplers[sampler_bilinear_wrap], x * scale, 0).r * 2.0f - 1.0f;
}

// spartan take on the interleaved gradient function from Jimenez 2014 http://goo.gl/eomGso
public float noise_interleaved_gradient(float2 screen_pos, bool temporal = true)
{
    // temporal factor
    float animate = saturate((float)is_taa_enabled() + (float)temporal);
    float frame_count = (float)get_frame_index();
    float frame_step = float(frame_count % 16) * RPC_16 * animate;
    screen_pos.x += frame_step * 4.7526;
    screen_pos.y += frame_step * 3.1914;

    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(screen_pos, magic.xy)));
}


