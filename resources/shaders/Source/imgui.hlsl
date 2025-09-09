/*
-------------------------------------------------------
Scenery Editor X
-------------------------------------------------------
Copyright (c) 2025 Thomas Ray
Copyright (c) 2025 Coalition of Freeware Developers
-------------------------------------------------------
imgui.hlsl
Shader for rendering ImGui textures with various options.
-------------------------------------------------------
Created: 29/8/2025
-------------------------------------------------------
*/

#include "output.hlsl"

// ---------------------------------------------------------------

struct Vertex_Pos2dUvColor
{
    float2 position : POSITION;
    float2 uv       : TEXCOORD;
    float4 color    : COLOR;
};

struct vertex
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
    float2 uv       : TEXCOORD;
};

vertex main_vs(Vertex_Pos2dUvColor input)
{
    vertex output;

    output.position = mul(buffer_pass.transform, float4(input.position.x, input.position.y, 0.0f, 1.0f));
    output.color    = input.color;
    output.uv       = input.uv;

    return output;
}

float4 main_ps(vertex input) : SV_Target
{
    // extract push constant data
    float3 flags_packed = pass_get_f3_value();
    uint flags          = asuint(flags_packed.x); // m00 contains bitfield
    float2 mip_array    = pass_get_f2_value();    // mip_level, array_level

    // extract booleans
    uint channel_r        = (flags & (1 << 0))  != 0 ? 1 : 0;
    uint channel_g        = (flags & (1 << 1))  != 0 ? 1 : 0;
    uint channel_b        = (flags & (1 << 2))  != 0 ? 1 : 0;
    uint channel_a        = (flags & (1 << 3))  != 0 ? 1 : 0;
    uint gamma_correct    = (flags & (1 << 4))  != 0 ? 1 : 0;
    uint packed           = (flags & (1 << 5))  != 0 ? 1 : 0;
    uint boost            = (flags & (1 << 6))  != 0 ? 1 : 0;
    uint absolute         = (flags & (1 << 7))  != 0 ? 1 : 0;
    uint point_sampling   = (flags & (1 << 8))  != 0 ? 1 : 0;
    uint is_visualized    = (flags & (1 << 9))  != 0 ? 1 : 0;
    uint is_frame_texture = (flags & (1 << 10)) != 0 ? 1 : 0;

    float4 channels = float4(channel_r, channel_g, channel_b, channel_a);

    // sample texture
    float4 color_texture;
    float mip_level             = mip_array.x;
    float array_level           = mip_array.y;
    float is_array              = array_level > 0.0f ? 1.0f : 0.0f; // not needed anymore
    float3 uv_array             = float3(input.uv, array_level);
    float4 sample_point_wrap    = tex.SampleLevel(samplers[sampler_point_clamp], input.uv, mip_level);
    float4 sample_bilinear_wrap = tex.SampleLevel(samplers[sampler_bilinear_clamp], input.uv, mip_level);
    color_texture               = lerp(sample_bilinear_wrap, sample_point_wrap, float(point_sampling));

    // visualization
    float4 color_original = color_texture;
    uint num_channels     = channel_r + channel_g + channel_b + channel_a;
    float f_visualized    = float(is_visualized);
    float val             = dot(color_texture, channels);
    float3 rgb_single     = val.xxx;
    float a_single        = 1.0f;
    float3 rgb_multi      = color_texture.rgb * channels.rgb;
    float a_multi         = lerp(1.0f, color_texture.a, channels.a);
    float is_single       = num_channels == 1u ? 1.0f : 0.0f;
    float3 rgb_vis        = lerp(rgb_multi, rgb_single, is_single);
    float a_vis           = lerp(a_multi, a_single, is_single);
    color_texture         = lerp(color_original, float4(rgb_vis, a_vis), f_visualized);

    color_texture      = lerp(color_texture, abs(color_texture), f_visualized * float(absolute));
    color_texture.rgb  = lerp(color_texture.rgb, pack(color_texture.rgb), f_visualized * float(packed));
    color_texture.rgb  = lerp(color_texture.rgb, linear_to_srgb(color_texture.rgb), f_visualized * float(gamma_correct));
    color_texture.rgb *= lerp(1.0f, 10.0f, f_visualized * float(boost));

    // final
    float4 color = input.color * color_texture;

    // hdr
    float apply_hdr     = buffer_frame.hdr_enabled * (1.0f - is_frame_texture);
    float3 color_linear = srgb_to_linear(color.rgb);
    float3 color_hdr    = linear_to_hdr10(color_linear, 400.0f);
    color.rgb           = lerp(color.rgb, color_hdr, apply_hdr);

    return color;
}
