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
 * position.slang
 * -------------------------------------------------------
 * Created: 07/03/2026
 * -------------------------------------------------------
 */
module position;

import resources;
import depth;

float3 get_position(float z, float2 uv)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0f - uv.y) * 2.0f - 1.0f;
    float4 pos_clip = float4(x, y, z, 1.0f);
    float4 pos_world = mul(pos_clip, buffer_frame.view_projection_inverted);
    return pos_world.xyz / pos_world.w;
}

float3 get_position(float2 uv)
{
    return get_position(get_depth(uv), uv / buffer_frame.resolution_scale);
}

float3 get_position(uint2 pos)
{
    const float2 uv = (pos + 0.5f) / (buffer_frame.resolution_render * buffer_frame.resolution_scale);
    return get_position(get_depth(pos), uv);
}

float3 get_position_view_space(uint2 pos)
{
    return mul(float4(get_position(pos), 1.0f), buffer_frame.view).xyz;
}

float3 get_position_view_space(float2 uv)
{
    return mul(float4(get_position(get_depth(uv), uv / buffer_frame.resolution_scale), 1.0f), buffer_frame.view).xyz;
}
