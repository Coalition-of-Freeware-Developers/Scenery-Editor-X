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
 * selection_outline.slang
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
import common;
import resources;
import constants;

// Editor selection and hover outlines (ScenePicking::RecordOutline), drawn over the composed frame (tex_uav) from the
// g-buffer's pick ids (tex_uav_uint, gbuffer_pick: entity index + 1 per pixel, 0 for none). Pass constants: f4 = (outline
// radius in pixels, tint of selected surfaces, -, -). selection_mask holds two
// bits per entity index, written by the CPU only when the selection or the hovered object changes:
//   0 none, 1 selected, 2 hovered, 3 active (the primary selection)
// A pixel is outlined when it doesn't belong to a highlighted object but a pixel within the radius does, so the
// outline hugs the object from outside and never covers it; selected objects also get a light tint. Only surfaces the
// g-buffer drew are covered: blended glass shows the object behind it, hidden parts aren't outlined.

[[vk::binding(145)]] RWStructuredBuffer<uint> selection_mask : register(u145);

static const int MAX_OUTLINE_RADIUS = 4;
static const float3 COLOR_ACTIVE   = float3(1.00f, 0.62f, 0.12f);
static const float3 COLOR_SELECTED = float3(0.95f, 0.42f, 0.08f);
static const float3 COLOR_HOVERED  = float3(0.55f, 0.78f, 1.00f);

uint state_of(uint pick)
{
    if (pick == 0u)
        return 0u;
    const uint index = pick - 1u;
    return (selection_mask[index >> 4u] >> ((index & 15u) * 2u)) & 3u;
}

// Selection outranks hover: active > selected > hovered > none
uint rank_of(uint state)
{
    return state == 3u ? 3u : (state == 1u ? 2u : (state == 2u ? 1u : 0u));
}

float3 color_of(uint rank)
{
    return rank == 3u ? COLOR_ACTIVE : (rank == 2u ? COLOR_SELECTED : COLOR_HOVERED);
}

[shader("compute")]
[numthreads(THREAD_GROUP_COUNT_X, THREAD_GROUP_COUNT_Y, 1)]
void main_comp(uint3 thread_id : SV_DispatchThreadID)
{
    uint2 size;
    tex_uav.GetDimensions(size.x, size.y);
    if (any(thread_id.xy >= size))
        return;

    uint2 pick_size;
    tex_uav_uint.GetDimensions(pick_size.x, pick_size.y);
    const int2 limit = int2(pick_size) - 1;
    const int2 pixel = min(int2(thread_id.xy), limit);

    const float4 settings = pass_get_f4_value();
    const int radius = clamp(int(settings.x + 0.5f), 1, MAX_OUTLINE_RADIUS);
    const float fill = saturate(settings.y);

    const uint center = rank_of(state_of(tex_uav_uint[uint2(pixel)]));

    // The strongest highlight around this pixel and how close it is (the outline fades over its last pixel)
    uint around = 0u;
    float nearest = 1e6f;
    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            const float distance = length(float2(x, y));
            if ((x == 0 && y == 0) || distance > float(radius) + 0.5f)
                continue;
            const uint rank = rank_of(state_of(tex_uav_uint[uint2(clamp(pixel + int2(x, y), int2(0, 0), limit))]));
            if (rank > around)
            {
                around = rank;
                nearest = distance;
            }
            else if (rank == around && rank > 0u)
                nearest = min(nearest, distance);
        }
    }

    float4 color = tex_uav[thread_id.xy];
    if (around > center)
    {
        // Outside the object: the outline, solid next to it and softer at its outer edge
        const float coverage = saturate(float(radius) + 0.5f - nearest + 0.5f);
        const float strength = around == 1u ? 0.85f : 1.0f;
        color.rgb = lerp(color.rgb, color_of(around), coverage * strength);
    }
    else if (center >= 2u)
    {
        // On a selected object: a light tint so flat ground art (pavement, polygons) reads as selected too
        color.rgb = lerp(color.rgb, color_of(center), fill);
    }
    tex_uav[thread_id.xy] = color;
}
