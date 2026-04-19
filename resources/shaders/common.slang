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
 * common.slang
 * -------------------------------------------------------
 * Created: 06/03/2026
 * -------------------------------------------------------
 */
import resources;
import color;
import math;
import constants;

/*------------------------------------------------------------------------------
    SATURATE
------------------------------------------------------------------------------*/
public float saturate_16(float x) { return clamp(x, 0.0f, FLT_MAX_16U); }
public float2 saturate_16(float2 x) { return clamp(x, 0.0f, FLT_MAX_16U); }
public float3 saturate_16(float3 x) { return clamp(x, 0.0f, FLT_MAX_16U); }
public float4 saturate_16(float4 x) { return clamp(x, 0.0f, FLT_MAX_16U); }

/*------------------------------------------------------------------------------
    VALIDATE (used because AMD SSSR is full of issues)
------------------------------------------------------------------------------*/
public float validate_output(float value)
{
    value = saturate_16(value);
    float is_nan_mask = isnan(value) ? 1.0f : 0.0f;
    return lerp(value, 1.0f, is_nan_mask);
}

public float2 validate_output(float2 value)
{
    return float2
    (
        validate_output(value.x),
        validate_output(value.y)
    );
}

public float3 validate_output(float3 value)
{
    return float3
    (
        validate_output(value.x),
        validate_output(value.y),
        validate_output(value.z)
    );
}

public float4 validate_output(float4 value)
{
    return float4
    (
        validate_output(value.x),
        validate_output(value.y),
        validate_output(value.z),
        validate_output(value.w)
    );
}

/*------------------------------------------------------------------------------
    PACK/UNPACK
------------------------------------------------------------------------------*/
public float3 unpack(float3 value) { return value * 2.0f - 1.0f; }
public float3 pack(float3 value) { return value * 0.5f + 0.5f; }
public float2 unpack(float2 value) { return value * 2.0f - 1.0f; }
public float2 pack(float2 value) { return value * 0.5f + 0.5f; }
public float unpack(float value) { return value * 2.0f - 1.0f; }
public float pack(float value) { return value * 0.5f + 0.5f; }


/*------------------------------------------------------------------------------
    NORMAL
------------------------------------------------------------------------------*/
// reconstruct normal z, x and y components have to be in a -1 to 1 range.
public float3 reconstruct_normal_z(float2 normal)
{
    float z = sqrt(saturate(1.0f - dot(normal, normal)));
    return float3(normal, z);
}

public float3 get_normal(uint2 pos)
{
    // load returns 0 for any value accessed out of bounds, so clamp.
    float2 resolution_render = get_resolution_render();
    pos.x = clamp(pos.x, 0u, (uint)resolution_render.x);
    pos.y = clamp(pos.y, 0u, (uint)resolution_render.y);

    return tex_normal[pos].xyz;
}

public float3 get_normal(float2 uv)
{
    return tex_normal.SampleLevel(samplers[sampler_point_clamp], uv, 0).xyz;
}

public float3 get_normal_view_space(uint2 pos)
{
    float4 normal_vs = mul(float4(get_normal(pos), 0.0f), get_view_matrix());
    return normalize(normal_vs.xyz);
}

public float3 get_normal_view_space(float2 uv)
{
    float4 normal_vs = mul(float4(get_normal(uv), 0.0f), get_view_matrix());
    return normalize(normal_vs.xyz);
}

public float3x3 make_tangent_to_world_matrix(float3 n, float3 t)
{
    // re-orthogonalize T with respect to N
    t = normalize(t - dot(t, n) * n);
    // compute bitangent
    float3 b = cross(n, t);
    // create matrix
    return float3x3(t, b, n);
}

public float3x3 make_world_to_tangent_matrix(float3 n, float3 t)
{
    return transpose(make_tangent_to_world_matrix(n, t));
}

/*------------------------------------------------------------------------------
    VELOCITY
------------------------------------------------------------------------------*/
public float2 get_velocity_uv(uint2 pos)
{
    return tex_velocity[pos].xy;
}

public float2 get_velocity_uv(float2 uv)
{
    return tex_velocity.SampleLevel(samplers[sampler_bilinear_clamp], uv, 0).xy;
}

/*------------------------------------------------------------------------------
    DIRECTION UV
------------------------------------------------------------------------------*/
public float2 direction_sphere_uv(float3 direction)
{
    float u = 0.5f + atan2(direction.z, direction.x) / PI2;
    float v = 0.5f - asin(direction.y) / PI;

    return float2(u, v);
}

/*------------------------------------------------------------------------------
    OCCLUSION/SHADOWING
------------------------------------------------------------------------------*/
// The Technical Art of Uncharted 4 - http://advances.realtimerendering.com/other/2016/naughty_dog/index.html
public float microw_shadowing_nt(float n_dot_l, float ao)
{
    float aperture = 2.0f * ao * ao;
    return saturate(abs(n_dot_l) + aperture - 1.0f);
}

// Chan 2018, "Material Advances in Call of Duty: WWII"
public float microw_shadowing_cod(float n_dot_l, float visibility)
{
    float aperture = rsqrt(1.0 - visibility);
    float microShadow = saturate(n_dot_l * aperture);
    return microShadow * microShadow;
}

/*------------------------------------------------------------------------------
    MISC
------------------------------------------------------------------------------*/
public bool is_valid_uv(float2 value) { return (value.x >= 0.0f && value.x <= 1.0f) && (value.y >= 0.0f && value.y <= 1.0f); }

public float screen_fade(float2 uv)
{
    float2 fade = max(0.0f, 12.0f * abs(uv - 0.5f) - 5.0f);
    return saturate(1.0f - dot(fade, fade));
}

// find good arbitrary axis vectors to represent U and V axes of a plane, given just the normal. ported from UnMath.h
public void find_best_axis_vectors(float3 In, out float3 Axis1, out float3 Axis2)
{
    const float3 N = abs(In);

    // find best basis vector
    if (N.z > N.x && N.z > N.y)
    {
        Axis1 = float3(1, 0, 0);
    }
    else
    {
        Axis1 = float3(0, 0, 1);
    }

    Axis1 = normalize(Axis1 - In * dot(Axis1, In));
    Axis2 = cross(Axis1, In);
}

static const float3 hemisphere_samples[64] =
    {
        float3(0.04977, -0.04471, 0.04996),
        float3(0.01457, 0.01653, 0.00224),
        float3(-0.04065, -0.01937, 0.03193),
        float3(0.01378, -0.09158, 0.04092),
        float3(0.05599, 0.05979, 0.05766),
        float3(0.09227, 0.04428, 0.01545),
        float3(-0.00204, -0.0544, 0.06674),
        float3(-0.00033, -0.00019, 0.00037),
        float3(0.05004, -0.04665, 0.02538),
        float3(0.03813, 0.0314, 0.03287),
        float3(-0.03188, 0.02046, 0.02251),
        float3(0.0557, -0.03697, 0.05449),
        float3(0.05737, -0.02254, 0.07554),
        float3(-0.01609, -0.00377, 0.05547),
        float3(-0.02503, -0.02483, 0.02495),
        float3(-0.03369, 0.02139, 0.0254),
        float3(-0.01753, 0.01439, 0.00535),
        float3(0.07336, 0.11205, 0.01101),
        float3(-0.04406, -0.09028, 0.08368),
        float3(-0.08328, -0.00168, 0.08499),
        float3(-0.01041, -0.03287, 0.01927),
        float3(0.00321, -0.00488, 0.00416),
        float3(-0.00738, -0.06583, 0.0674),
        float3(0.09414, -0.008, 0.14335),
        float3(0.07683, 0.12697, 0.107),
        float3(0.00039, 0.00045, 0.0003),
        float3(-0.10479, 0.06544, 0.10174),
        float3(-0.00445, -0.11964, 0.1619),
        float3(-0.07455, 0.03445, 0.22414),
        float3(-0.00276, 0.00308, 0.00292),
        float3(-0.10851, 0.14234, 0.16644),
        float3(0.04688, 0.10364, 0.05958),
        float3(0.13457, -0.02251, 0.13051),
        float3(-0.16449, -0.15564, 0.12454),
        float3(-0.18767, -0.20883, 0.05777),
        float3(-0.04372, 0.08693, 0.0748),
        float3(-0.00256, -0.002, 0.00407),
        float3(-0.0967, -0.18226, 0.29949),
        float3(-0.22577, 0.31606, 0.08916),
        float3(-0.02751, 0.28719, 0.31718),
        float3(0.20722, -0.27084, 0.11013),
        float3(0.0549, 0.10434, 0.32311),
        float3(-0.13086, 0.11929, 0.28022),
        float3(0.15404, -0.06537, 0.22984),
        float3(0.05294, -0.22787, 0.14848),
        float3(-0.18731, -0.04022, 0.01593),
        float3(0.14184, 0.04716, 0.13485),
        float3(-0.04427, 0.05562, 0.05586),
        float3(-0.02358, -0.08097, 0.21913),
        float3(-0.14215, 0.19807, 0.00519),
        float3(0.15865, 0.23046, 0.04372),
        float3(0.03004, 0.38183, 0.16383),
        float3(0.08301, -0.30966, 0.06741),
        float3(0.22695, -0.23535, 0.19367),
        float3(0.38129, 0.33204, 0.52949),
        float3(-0.55627, 0.29472, 0.3011),
        float3(0.42449, 0.00565, 0.11758),
        float3(0.3665, 0.00359, 0.0857),
        float3(0.32902, 0.0309, 0.1785),
        float3(-0.08294, 0.51285, 0.05656),
        float3(0.86736, -0.00273, 0.10014),
        float3(0.45574, -0.77201, 0.00384),
        float3(0.41729, -0.15485, 0.46251),
        float3(-0.44272, -0.67928, 0.1865)
    };

public float get_alpha_threshold(float3 position_world)
{
    static const float ALPHA_THRESHOLD_DEFAULT = 0.6f;
    static const float ALPHA_MAX_DISTANCE = 700.0f;
    static const float ALPHA_MAX_DISTANCE_SQ = ALPHA_MAX_DISTANCE * ALPHA_MAX_DISTANCE;

    // beyond max distance, no alpha testing (threshold = 0)
    float3 offset = position_world - get_camera_position();
    float pixel_distance_sq = dot(offset, offset);
    float distance_factor = step(ALPHA_MAX_DISTANCE_SQ, pixel_distance_sq);

    return saturate(lerp(ALPHA_THRESHOLD_DEFAULT, 0.0f, distance_factor));
}



