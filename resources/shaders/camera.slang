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
 * camera.slang
 * -------------------------------------------------------
 * Created: 07/03/2026
 * -------------------------------------------------------
 */
import common;

struct CameraBufferData
{
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
    float4x4 inverseViewProjection;
    float3 positionWorld;
    float padding;
};

// Bind this data via a Vulkan Uniform Buffer (UBO) at set 1, binding 0.
// Set 0 is reserved for bindless texture arrays.
[[vk::binding(0, 1)]]
cbuffer CameraBuffer : register(b0)
{
    CameraBufferData camera;
};

// GPU-Offloaded Operations

// 1. Offload all vertex clip-space transformations
float4 transform_to_clip_space(float3 world_position)
{
    return mul(camera.viewProjection, float4(world_position, 1.0f));
}

// 2. Offload camera direction calculations for lighting/BRDF
float3 get_view_direction(float3 world_position)
{
    return normalize(camera.positionWorld - world_position);
}

// 3. Offload world-space reconstruction (useful for deferred rendering/post-processing)
float3 reconstruct_world_position(float2 uv, float depth)
{
    float4 clip_space = float4(uv * 2.0f - 1.0f, depth, 1.0f);

    clip_space.y = -clip_space.y; // Vulkan uses top-left origin, so flip Y if necessary depending on viewport setup

    float4 world_space = mul(camera.inverseViewProjection, clip_space);
    return (world_space.xyz / world_space.w);
}
