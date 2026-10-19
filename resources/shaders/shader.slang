/* Copyright (c) 2025-2026, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput {
	float3 Pos;
	float3 Normal;
	float2 UV;
};

Sampler2D textures[];

/**
 * Camera uniform buffer — set 1, binding 0.
 * Matches CameraShaderData (C++) and CameraBufferData (camera.slang).
 * Populated every frame by Renderer::RecordRenderCommands from the active Camera.
 */
struct CameraData
{
	float4x4 view;
	float4x4 projection;
	float4x4 viewProjection;
	float4x4 inverseViewProjection;
	float3   positionWorld;
	float    padding;
};

[[vk::binding(0, 1)]]
cbuffer CameraBuffer
{
	CameraData camera;
};

struct ShaderData {
	float4x4 model[3];
	float4 lightPos;
	uint32_t selected;
};

struct VSOutput {
	float4 Pos : SV_POSITION;
	float3 Normal;
	float2 UV;
	float3 Factor;
	float3 LightVec;
	float3 ViewVec;
	uint32_t InstanceIndex;
};

[shader("vertex")]
VSOutput main(VSInput input, uniform ShaderData *shaderData, uint instanceIndex : SV_VulkanInstanceID) {
	VSOutput output;
	// TEMP DEBUG: bypass ShaderData model buffer fetch (device-address path)
	// to verify whether stretched geometry comes from bad model matrix data.
	float4x4 modelMat = float4x4(
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0);
    output.Normal = input.Normal;
	output.UV = input.UV;
    // Stable diagnostic baseline: bypass camera/projection completely and render
	// model directly in clip-space so geometry visibility is guaranteed.
	float3 debugPos = input.Pos.xyz * 0.35;
	output.Pos = float4(debugPos.xy, 0.0, 1.0);
	output.Factor = (shaderData->selected == instanceIndex ? 3.0f : 1.0f);
	output.InstanceIndex = instanceIndex;
	output.LightVec = float3(0.0, 0.0, 1.0);
	output.ViewVec = float3(0.0, 0.0, 1.0);
	return output;
}

[shader("fragment")]
float4 main(VSOutput input) {
	// TEMP DEBUG PATH: consume InstanceIndex so vertex->fragment interface matches.
	float tint = (input.InstanceIndex == 0 ? 1.0 : (input.InstanceIndex == 1 ? 0.8 : 0.6));
	return float4(0.0, tint, 0.2, 1.0);
}
