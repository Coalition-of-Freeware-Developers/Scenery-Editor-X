/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * light_billboard.slang
 * -------------------------------------------------------
 * Camera-facing light sprites added onto the composed frame (Renderer::Pass_Light_Billboards).
 *
 * The CPU writes six vertices per sprite in the PositionUvNormalTangent layout:
 *   position - sprite center (world)
 *   uv       - texture coordinate of this corner
 *   normal   - linear HDR color, brightness included
 *   tangent  - x: corner x (+-1) scaled by (1 + 2 * mode), y: corner y (+-1), z: world-space half size
 * mode 0 tints the texel's color, 1 tints its brightness (X-Plane 12 photometric lights), 2 weights the color by alpha.
 * -------------------------------------------------------
 */
import resources;
import color;
import environment_lighting;

struct BillboardVertex
{
    float3 position : POSITION;
    float2 uv       : TEXCOORD;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
};

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
    nointerpolation float3 color : COLOR;
    nointerpolation uint mode : MODE;
    nointerpolation float3 world : WORLD;	// the sprite's centre, for the air between it and the camera
};

static const float min_radius_pixels = 1.5f; // far lights stay a few pixels wide
static const float BILLBOARD_NITS = 10.0f;      // luminance of a sprite of brightness 1 (ASSUMPTION until plan Phase 14's photometric lights)

[shader("vertex")]
PixelInput main_vs(BillboardVertex input)
{
    float radius = input.tangent.z;
    float2 corner = float2(sign(input.tangent.x), input.tangent.y);

    // pull the sprite towards the camera so the fixture it sits on doesn't cut it in half
    float3 center = input.position;
    float3 to_camera = get_camera_position() - center;
    float distance = length(to_camera);
    center += to_camera / max(distance, 1e-4f) * min(radius, distance * 0.5f);

    // expand in clip space: offsetting clip xy by the projection scale is a view-space offset before the divide
    float4 clip = mul(float4(center, 1.0f), get_view_projection_matrix());
    matrix projection = get_projection_matrix();
    float2 scale = float2(abs(projection[0][0]), abs(projection[1][1]));

    float w = max(abs(clip.w), 1e-4f);
    float radius_ndc = radius * scale.y / w;
    float min_ndc = 2.0f * min_radius_pixels / max(get_resolution_render().y, 1.0f);
    float grow = max(1.0f, min_ndc / max(radius_ndc, 1e-6f));
    clip.xy += corner * radius * grow * scale;

    PixelInput output;
    output.position = clip;
    output.uv = input.uv;
    output.color = input.normal / grow; // widened sprites spread their light rather than brighten
    output.mode = (uint)round((abs(input.tangent.x) - 1.0f) * 0.5f);
    output.world = input.position;
    return output;
}

[shader("fragment")]
float4 main_frag(PixelInput input) : SV_TARGET
{
    float4 texel = tex.SampleLevel(samplers[sampler_bilinear_clamp], input.uv, 0.0f);
    float3 rgb = srgb_to_linear(texel.rgb);

    float3 result;
    if (input.mode == 1)
        result = input.color * max(rgb.r, max(rgb.g, rgb.b));
    else if (input.mode == 2)
        result = input.color * rgb * texel.a;
    else
        result = input.color * rgb;

    // A sprite of brightness 1 is BILLBOARD_NITS of luminance, pre-exposed like the rest of the frame, dimmed by the air
    // between (additive: the frame behind already has the in-scattering)
    const float2 screen_uv = input.position.xy / get_resolution_render();
    const float3 air = xp_aerial_transmittance(screen_uv, length(input.world - get_camera_position()));
    return float4(max(result, 0.0f) * air * BILLBOARD_NITS * get_pre_exposure(), 0.0f); // additive
}
