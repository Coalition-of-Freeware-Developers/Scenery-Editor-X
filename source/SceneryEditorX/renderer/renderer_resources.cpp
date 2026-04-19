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
 * renderer_resources.cpp
 * -------------------------------------------------------
 * Created: 02/03/2026
 * -------------------------------------------------------
 */
// ReSharper disable CommentTypo
#include "gbuffer.h"
#include "renderer.h"
#include "renderer_buffers.h"
#include "renderer_declarations.h"
#include "SceneryEditorX/asset/asset_extensions.h"
#include "SceneryEditorX/asset/model/mesh_gen.h"
#include "SceneryEditorX/core/window/window.h"
#include "font/font.h"
#include "vulkan/buffer.h"
#include "vulkan/depth_stencil.h"
#include "vulkan/image_resource.h"
#include "vulkan/rasterizer.h"
#include "vulkan/sampler.h"
#include <SceneryEditorX/asset/import/texture_importer.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/mesh.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/settings/settings.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

#pragma region Resource Declarations

	// Renderer resources
	static std::array<Ref<Sampler>, static_cast<uint32_t>(Renderer_Sampler::MaxEnum)>			 s_Samplers;
	static std::array<Ref<Buffer>,  static_cast<uint32_t>(Renderer_Buffer::MaxEnum)>			 s_Buffers;
	static std::array<Ref<ImageResource>, static_cast<uint32_t>(Renderer_RenderTarget::MaxEnum)> s_RenderTargets;
	Ref<Font>																					 s_StandardFont;
	Ref<MaterialAsset>                                                                           s_StandardMaterial;
	std::array<Ref<ImageResource>, static_cast<uint32_t>(Renderer_StandardTexture::MaxEnum)>	 s_StandardTextures;
	static std::array<Ref<Mesh>, static_cast<uint8_t>(MeshType::MaxEnum)>                        s_StandardMeshes;

	// Static state object instances (created once, never mutated after init)
	static std::array<RasterizerState,   static_cast<uint8_t>(Renderer_RasterizerState::MaxEnum)> s_RasterizerStates  = {
		RasterizerState(RasterStateSpec{ PolygonMode::Solid,     false, true,  0.0f, 0.0f, 0.0f, 1.0f }),   // Solid
		RasterizerState(RasterStateSpec{ PolygonMode::Wireframe, false, true,  0.0f, 0.0f, 0.0f, 1.0f }),   // Wireframe
		RasterizerState(RasterStateSpec{ PolygonMode::Solid,     true,  true,  1.0f, 1.75f, 0.0f, 1.0f }), // Light_point_spot  (depth bias)
		RasterizerState(RasterStateSpec{ PolygonMode::Solid,     true,  true,  1.0f, 2.00f, 0.0f, 1.0f }), // Light_directional (depth bias)
	};

	static std::array<BlendState, static_cast<uint8_t>(Renderer_BlendState::MaxEnum)> s_BlendStates = {
		BlendState(BlendStateSpec{ false }), // Off
		BlendState(BlendStateSpec{ true, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
									VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, 0.0f }), // Alpha
		BlendState(BlendStateSpec{ true, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
									VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD, 0.0f }), // Additive
		BlendState(BlendStateSpec{ true,  VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
									VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, 0.0f }), // Premultiplied
	};

	static std::array<DepthStencilState, static_cast<uint8_t>(Renderer_DepthStencilState::MaxEnum)> s_DepthStencilStates = {
		DepthStencilState(DepthStencilSpec{ false, false, VK_COMPARE_OP_ALWAYS }), // Off
		DepthStencilState(DepthStencilSpec{ true,  false, VK_COMPARE_OP_EQUAL  }), // ReadEqual
		DepthStencilState(DepthStencilSpec{ true,  false, VK_COMPARE_OP_GREATER_OR_EQUAL }), // ReadGreaterEqual (reverse-z)
		DepthStencilState(DepthStencilSpec{ true,  true,  VK_COMPARE_OP_GREATER }), // ReadWrite (reverse-z)
	};

	// Static members defined here (declared in renderer.h)
	PushConstantBuffer_Pass Renderer::m_Pcb_Pass_Cpu;
	uint32_t           Renderer::m_DrawCall_Count           = 0;
	Flag               Renderer::m_BindlessSamplers_Dirty;
	uint32_t           Renderer::m_DrawCalls_Prepass_Count  = 0;
	uint32_t           Renderer::m_Indirect_DrawCount       = 0;
	bool               Renderer::m_Transparents_Present     = false;
	bool               Renderer::m_Is_Hiz_Suppressed        = false;

	// Additional static member definitions centralized here
	std::array<Renderer_DrawCall, RENDERER_MAX_DRAW_CALLS> Renderer::m_DrawCalls;
	std::array<Renderer_DrawCall, RENDERER_MAX_DRAW_CALLS> Renderer::m_DrawCalls_Prepass;
	std::array<ShaderBuffer_IndirectDrawArgs, MAX_ARRAY_SIZE> Renderer::m_Indirect_DrawArgs;
	std::array<ShaderBuffer_DrawData, MAX_ARRAY_SIZE> Renderer::m_Indirect_DrawData;

	// Debug primitives and bindless arrays
	std::vector<Vertex_PosCol> Renderer::m_Lines_Vertices;
	Ref<Buffer> Renderer::m_Lines_VertexBuffer;
	std::vector<PersistentLine> Renderer::m_Persistent_Lines;
	std::vector<std::tuple<ImageResource*, xMath::Vec3>> Renderer::m_Icons;

	std::array<ImageResource*, MAX_ARRAY_SIZE> Renderer::m_Bindless_Textures;
	std::array<ShaderBuffer_Light, MAX_ARRAY_SIZE> Renderer::m_Bindless_Lights;
	std::array<ShaderBuffer_Aabb, MAX_ARRAY_SIZE> Renderer::m_Bindless_Aabbs;

	/**
	 * @brief Helper function to access resources by enum type without needing to static_cast every time.
	 * @tparam E An enum type that can be cast to size_t for indexing.
	 * @tparam A The type of the array or container.
	 * @param arr The array or container to access.
	 * @param e The enum value used as the index.
	 * @return A reference to the element at the specified index.
	 */
	template<typename E, typename A>
	auto& At(A& arr, E e) { return arr[static_cast<size_t>(e)]; }

#pragma endregion

	void Renderer::CreateRenderTargets(const bool createRender, const bool createOutput, const bool createDynamic)
	{
		SEDX_CORE_TRACE_TAG("RendererResources", "Creating render targets");

		uint32_t widthRender  = static_cast<uint32_t>(GetRendererResolution().x);
		uint32_t heightRender = static_cast<uint32_t>(GetRendererResolution().y);
		uint32_t widthOutput  = static_cast<uint32_t>(GetOutputResolution().x);
		uint32_t heightOutput = static_cast<uint32_t>(GetOutputResolution().y);

#pragma region Mip Count Calculation

		// lambda to compute mip count based on dimensions and minimum mip size (used for render targets with per-mip views)
		auto compute_mip_count = [](const uint32_t width, const uint32_t height, const uint32_t minDimension)
		{
			uint32_t maxDimension = xMath::Max(width, height);
			uint32_t mipCount     = 1;
			while (maxDimension >= minDimension)
			{
				maxDimension /= 2;
				mipCount++;
			}
			return mipCount;
		};

#pragma endregion

		SEDX_CORE_TRACE_TAG("RendererResources", "Mip count calculation completed");

		// avoid combining uav + rtv on frequently accessed targets (forces suboptimal layouts on amd)
		// resolution - render
		if (createRender)
		{
#pragma region Frame Render Targets
			{
				s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_render)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D,
					widthRender, heightRender, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, UnorderedAccessView | ShaderViews | RenderTargetViews | BlitClear, "frame_render"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_render)] != nullptr, "Failed to create frame render target");

				s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_render)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D,
					widthRender, heightRender, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, ShaderViews | UnorderedAccessView | RenderTargetViews | BlitClear, "frame_render"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_render)] != nullptr, "Failed to create frame render target");

				s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_render_opaque)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D,
					widthRender, heightRender, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, ShaderViews | RenderTargetViews | BlitClear, "frame_render_opaque"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_render_opaque)] != nullptr, "Failed to create frame_render_opaque render target");
			}
#pragma endregion
#pragma region G-Buffer Render Targets
			// g-buffer (concurrent sharing: read by async compute for ssao/sss)
			{
				uint32_t flags = RenderTargetViews | ShaderViews | BlitClear | QueueShare;

				s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_color)]    = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthRender, heightRender, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, flags, "gbuffer_color"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_color)] != nullptr, "Failed to create gbuffer_color render target");

				s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_normal)]   = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthRender, heightRender, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, flags, "gbuffer_normal"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_normal)] != nullptr, "Failed to create gbuffer_normal render target");

				s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_material)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthRender, heightRender, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, flags, "gbuffer_material"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_material)] != nullptr, "Failed to create gbuffer_material render target");

				s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_velocity)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthRender, heightRender, 1, 1, VK_FORMAT_R16G16_SFLOAT, flags, "gbuffer_velocity"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_velocity)] != nullptr, "Failed to create gbuffer_velocity render target");

				s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_depth)]    = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthRender, heightRender, 1, 1, VK_FORMAT_D32_SFLOAT, flags, "gbuffer_depth"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_depth)] != nullptr, "Failed to create gbuffer_depth render target");
			}
#pragma endregion
#pragma region Lighting Render Targets
			// light
			{
				uint32_t flags = UnorderedAccessView | ShaderViews | BlitClear;

				s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::light_diffuse)]    = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthRender, heightRender, 1, 1, VK_FORMAT_B10G11R11_UFLOAT_PACK32, flags, "light_diffuse"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::light_diffuse)] != nullptr, "Failed to create light_diffuse render target");

				s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::light_specular)]   = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthRender, heightRender, 1, 1, VK_FORMAT_B10G11R11_UFLOAT_PACK32, flags, "light_specular"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::light_specular)] != nullptr, "Failed to create light_specular render target");
				
				s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::light_volumetric)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthRender, heightRender, 1, 1, VK_FORMAT_B10G11R11_UFLOAT_PACK32, flags, "light_volumetric"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::light_volumetric)] != nullptr, "Failed to create light_volumetric render target");
			}
#pragma endregion
#pragma region Occlusion Render Targets
			// occlusion
			{
				// amd depth format restrictions: separate texture for uav + manual blit
				s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_depth_occluders)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthRender, heightRender, 1, 1, VK_FORMAT_D32_SFLOAT, RenderTargetViews | ShaderViews, "depth_occluders"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_depth_occluders)] != nullptr, "Failed to create depth_occluders render target");

				// full mip chain so the cull shader can pick a level where the aabb fits in ~1-2 texels
				uint32_t hizMipCount = static_cast<uint32_t>(floor(log2(static_cast<float>(xMath::Max(widthRender, heightRender))))) + 1;

				s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_depth_occluders_hiz)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthRender, heightRender, 1, hizMipCount, VK_FORMAT_R32_SFLOAT, UnorderedAccessView | ShaderViews | BlitClear | PerMipViews, "depth_occluders_hiz"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_depth_occluders_hiz)] != nullptr, "Failed to create depth_occluders_hiz render target");
			}
#pragma endregion
			// misc
			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::sss)]                = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2DArray, widthRender, heightRender, 4, 1, VK_FORMAT_R16_SFLOAT, UnorderedAccessView | ShaderViews | BlitClear | QueueShare, "sss"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::sss)] != nullptr, "Failed to create Screen Space Shadows (SSS) render target");

			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::reflections)]        = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthRender, heightRender, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, UnorderedAccessView | ShaderViews | BlitClear, "reflections"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::reflections)] != nullptr, "Failed to create reflections render target");

			// optional render targets (ssao, rt reflections, restir)
			UpdateOptionalRenderTargets();
			
			const Ref<Device> device = RenderContext::Get()->GetLogicalDevice();
			if (device->GetDeviceStatics().isShadingRateSupported)
			{
				// vrs texture dimensions must match the gpu's reported texel size
				uint32_t texelSizeX = xMath::Max(device->GetDeviceStatics().maxShadingRateTexelSizeX, 1u);
				uint32_t texelSizeY = xMath::Max(device->GetDeviceStatics().maxShadingRateTexelSizeY, 1u);
				uint32_t vrsWidth    = (widthRender + texelSizeX - 1) / texelSizeX;
				uint32_t vrsHeight   = (heightRender + texelSizeY - 1) / texelSizeY;
				s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::shading_rate)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, vrsWidth, vrsHeight, 1, 1, VK_FORMAT_R8_UINT, ShaderViews | UnorderedAccessView | RenderTargetViews | VariableRateShader | BlitClear | QueueShare, "shading_rate"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::shading_rate)] != nullptr, "Failed to create shading_rate render target");
			}

			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::shadow_atlas)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, 8192, 8192, 1, 1, VK_FORMAT_D32_SFLOAT, RenderTargetViews | ShaderViews | BlitClear, "shadow_atlas"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::shadow_atlas)] != nullptr, "Failed to create shadow_atlas render target");
		}

#pragma region Resolution Output Render Targets
		// resolution - output
		if (createOutput)
		{
			// frame
			uint32_t mipCount = compute_mip_count(widthOutput, heightOutput, 16);
			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_output)]   = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthOutput, heightOutput, 1, mipCount, VK_FORMAT_R16G16B16A16_SFLOAT, UnorderedAccessView | ShaderViews | RenderTargetViews | BlitClear | PerMipViews | QueueShare, "frame_output"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_output)] != nullptr, "Failed to create frame_output render target");

			// Transition frame_output to General during bootstrap. Using UNDEFINED ->
			// SHADER_READ_ONLY_OPTIMAL can trigger validation warnings because that pattern
			// discards contents and immediately declares a read-only layout. General keeps
			// the image valid for early compute write/read usage until pass code performs
			// the first explicit read-only transition.
			if (ImageResource* frameOutput = s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_output)].Get())
			{
				if (CommandList* initCmd = CommandList::BeginImmediateExecution(QueueType::Graphics))
				{
					frameOutput->SetLayout(Layout::ImageLayout::General, initCmd, ALL_MIPS, 0);
					CommandList::EndImmediateExecution(initCmd);
					SEDX_CORE_TRACE_TAG("RendererResources", "Transitioned frame_output to General layout");
				}
			}

			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_output_2)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthOutput, heightOutput, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, UnorderedAccessView | ShaderViews | RenderTargetViews | BlitClear, "frame_output_2"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_output_2)] != nullptr, "Failed to create frame_output_2 render target");

			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::debug_output)]   = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthOutput, heightOutput, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, UnorderedAccessView | ShaderViews | RenderTargetViews | BlitClear, "debug_output"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::debug_output)] != nullptr, "Failed to create debug_output render target");

			// misc
			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::bloom)]                       = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthOutput, heightOutput, 1, mipCount, VK_FORMAT_R16G16B16A16_SFLOAT, UnorderedAccessView | ShaderViews | PerMipViews, "bloom"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::bloom)] != nullptr, "Failed to create bloom render target");

			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::outline)]                     = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthOutput, heightOutput, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, UnorderedAccessView | ShaderViews | RenderTargetViews, "outline"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::outline)] != nullptr, "Failed to create outline render target");

			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_depth_opaque_output)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthOutput, heightOutput, 1, 1, VK_FORMAT_D32_SFLOAT, ShaderViews | RenderTargetViews | BlitClear, "depth_opaque_output"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_depth_opaque_output)] != nullptr, "Failed to create depth_opaque_output render target");
		}
#pragma endregion
#pragma region Fixed Resolution Render Targets
		// resolution - fixed (created once)
		if (!s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::lut_brdf_specular)])
		{
			// lookup tables
			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::lut_brdf_specular)]           = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, 512, 512, 1, 1, VK_FORMAT_R16G16_SFLOAT, UnorderedAccessView | ShaderViews, "lut_brdf_specular"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::lut_brdf_specular)] != nullptr, "Failed to create lut_brdf_specular render target");

			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::lut_atmosphere_scatter)]      = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type3D, 256, 256, 32, 1, VK_FORMAT_R16G16B16A16_SFLOAT, UnorderedAccessView | ShaderViews, "lut_atmosphere_scatter"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::lut_atmosphere_scatter)] != nullptr, "Failed to create lut_atmosphere_scatter render target");

			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::lut_atmosphere_transmittance)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, 256, 64, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, UnorderedAccessView | ShaderViews, "lut_atmosphere_transmittance"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::lut_atmosphere_transmittance)] != nullptr, "Failed to create lut_atmosphere_transmittance render target");

			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::lut_atmosphere_multiscatter)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, 32,  32, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, UnorderedAccessView | ShaderViews, "lut_atmosphere_multiscatter"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::lut_atmosphere_multiscatter)] != nullptr, "Failed to create lut_atmosphere_multi-scatter render target");

			// Misc
			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::blur)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, 4096, 4096, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, UnorderedAccessView | ShaderViews, "blur_scratch"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::blur)] != nullptr, "Failed to create blur render target");

			constexpr uint32_t lowestDimension = 16; // lowest mip is 16x16, preserving directional detail for diffuse IBL (1x1 loses directionality)
			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::skysphere)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, 4096, 2048, 1, compute_mip_count(4096, 2048, lowestDimension), VK_FORMAT_B10G11R11_UFLOAT_PACK32, UnorderedAccessView | ShaderViews | PerMipViews | BlitClear | QueueShare, "skysphere"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::skysphere)] != nullptr, "Failed to create skysphere render target");

			// Auto-Exposure
			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::auto_exposure)]          = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, 1, 1, 1, 1, VK_FORMAT_R32_SFLOAT, UnorderedAccessView | ShaderViews | BlitClear, "auto_exposure_1"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::auto_exposure)] != nullptr, "Failed to create auto_exposure render target");

			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::auto_exposure_previous)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, 1, 1, 1, 1, VK_FORMAT_R32_SFLOAT, UnorderedAccessView | ShaderViews | BlitClear, "auto_exposure_2"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::auto_exposure_previous)] != nullptr, "Failed to create auto_exposure_previous render target");

			// Volumetric Clouds (VK_FORMAT_R16G16B16A16_SFLOAT to avoid material texture detection)
			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::cloud_noise_shape)]  = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type3D, 128, 128, 128, 1, VK_FORMAT_R16G16B16A16_SFLOAT, UnorderedAccessView | ShaderViews | QueueShare, "cloud_noise_shape"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::cloud_noise_shape)] != nullptr, "Failed to create cloud_noise_shape render target");

			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::cloud_noise_detail)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type3D, 32,  32,  32,  1, VK_FORMAT_R16G16B16A16_SFLOAT, UnorderedAccessView | ShaderViews | QueueShare, "cloud_noise_detail"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::cloud_noise_detail)] != nullptr, "Failed to create cloud_noise_detail render target");

			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::cloud_shadow)]       = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, 1024, 1024, 1, 1, VK_FORMAT_R16_SFLOAT, UnorderedAccessView | ShaderViews | QueueShare, "cloud_shadow"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::cloud_shadow)] != nullptr, "Failed to create cloud_shadow render target");
		}
#pragma endregion
	}

	void Renderer::UpdateOptionalRenderTargets()
	{
		uint32_t width  = static_cast<uint32_t>(GetRendererResolution().x);
		uint32_t height = static_cast<uint32_t>(GetRendererResolution().y);
		uint32_t flags  = UnorderedAccessView | ShaderViews | BlitClear;
		
		// (SSAO) Screen-space ambient occlusion - optional since it can be expensive and not noticeable in many scenes
		bool needSSAO = false; /* TODO: Have the option settable in user settings */
		if (needSSAO && !s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::ssao)])
		{
		    s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::ssao)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, width, height, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, flags | QueueShare, "ssao"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::ssao)] != nullptr, "Failed to create ssao render target");
		}
		else if (!needSSAO && s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::ssao)])
		{
			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::ssao)] = nullptr;
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::ssao)] == nullptr, "Failed to destroy ssao render target");
		}

		{
			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_reflections_position)]  = nullptr;
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_reflections_position)] == nullptr, "Failed to destroy gbuffer_reflections_position render target");

			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_reflections_normal)]	= nullptr;
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_reflections_normal)] == nullptr, "Failed to destroy gbuffer_reflections_normal render target");

			s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_reflections_albedo)]	= nullptr;
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::gbuffer_reflections_albedo)] == nullptr, "Failed to destroy gbuffer_reflections_albedo render target");
		}

		SEDX_CORE_TRACE_TAG("RendererResources","Optional render targets updated");
	}

	ImageResource* Renderer::GetRenderTarget(const Renderer_RenderTarget type)
	{
		return s_RenderTargets[static_cast<uint8_t>(type)].Get();
	}

	/*
	void Renderer::UpdateFrustumAndDistanceCulling()
	{
		if (Camera* camera = Scene::GetCamera())
		{
			Vec3 camera_position = camera->GetEntity()->GetPosition();

			const BoundingBox& bounding_box = GetBoundingBox();

			// first, check if the bounding box is in the frustum
			if (camera->IsInViewFrustum(bounding_box))
			{
				// only if in frustum, calculate distance
				m_distance_squared = Vec3::DistanceSquared(camera_position, bounding_box.GetClosestPoint(camera_position));
				m_is_visible       = m_distance_squared <= m_max_distance_render * m_max_distance_render;
			}
			else
			{
				// outside frustum, no need for distance check
				m_is_visible = false;
			}
		}
		else
		{
			m_distance_squared = 0.0f;
			m_is_visible       = true;
		}
	}
	*/

	/*
	void Renderable::UpdateLodIndices()
	{
		// screen-space coverage based lod selection
		// this approach (used by unreal, unity, cryengine, frostbite) naturally handles:
		// - distance: farther objects appear smaller
		// - object size: larger objects maintain detail longer
		// - fov: wider fov = everything smaller on screen
		// - works uniformly for all object types (no special cases needed)

		const uint32_t lod_count = GetLodCount();
		if (lod_count == 0)
		{
			m_lod_index = 0;
			return;
		}

		Camera* camera = World::GetCamera();
		if (!camera)
		{
			m_lod_index = lod_count - 1;
			return;
		}

		const BoundingBox& box        = GetBoundingBox();
		const Vec3 camera_position = camera->GetEntity()->GetPosition();

		// camera inside bounding box = maximum detail
		if (box.Contains(camera_position))
		{
			m_lod_index = 0;
			return;
		}

		// distance from camera to closest point on bounding box
		Vec3 closest_point = box.GetClosestPoint(camera_position);
		float distance        = std::max((closest_point - camera_position).Length(), 0.001f);

		// compute screen-space coverage: fraction of vertical screen space the object covers
		// screen_fraction = (object_diameter) / (visible_height_at_distance)
		// visible_height_at_distance = 2 * distance * tan(fov_v / 2)
		float bounding_diameter = box.GetExtents().Length() * 2.0f;
		float tan_half_fov      = tan(camera->GetFovVerticalRad() * 0.5f);
		float screen_fraction   = bounding_diameter / (2.0f * distance * tan_half_fov);

		// lod thresholds as percentage of screen height coverage
		// calibrated so transitions remain imperceptible to the user
		// higher threshold = object must cover more screen to qualify for that lod
		static constexpr std::array<float, 5> screen_thresholds =
		{
			0.05f,   // lod0: object covers >= 5% of screen height
			0.025f,  // lod1: object covers >= 2.5% of screen height
			0.012f,  // lod2: object covers >= 1.2% of screen height
			0.006f,  // lod3: object covers >= 0.6% of screen height
			0.003f   // lod4: object covers >= 0.3% of screen height
		};

		// hysteresis prevents lod popping at threshold boundaries
		// upgrading to higher detail requires exceeding threshold by 10%
		// downgrading to lower detail requires dropping 10% below threshold
		constexpr float hysteresis = 1.1f;

		uint32_t new_lod = lod_count - 1;
		for (uint32_t i = 0; i < std::min(lod_count, static_cast<uint32_t>(screen_thresholds.size())); i++)
		{
			float threshold = screen_thresholds[i];

			// apply hysteresis based on relationship to current lod
			if (i < m_lod_index)
			{
				// upgrading to higher detail: raise the bar
				threshold *= hysteresis;
			}
			else if (i == m_lod_index)
			{
				// staying at current lod: lower the bar (easier to stay)
				threshold /= hysteresis;
			}

			if (screen_fraction >= threshold)
			{
				new_lod = i;
				break;
			}
		}

		m_lod_index = std::clamp(new_lod, 0u, lod_count - 1);
	}
	*/

#pragma region Samplers

	void Renderer::CreateSamplers()
	{
		SEDX_CORE_TRACE_TAG("Renderer", "Creating Renderer Samplers");

#pragma region Non-Anisotropic samplers
		// Non-anisotropic samplers - guarded by Ref validity so they are created only once
		if (!s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Compare_depth)])
		{
			s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Compare_depth)] = CreateRef<Sampler>(SamplerSpec{
				.min            = VK_FILTER_NEAREST,
				.mag            = VK_FILTER_LINEAR,
				.mipMap         = VK_SAMPLER_MIPMAP_MODE_NEAREST,
				.addressMode    = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
				.compareFunc    = VK_COMPARE_OP_GREATER,
				.compareEnabled = true
			}); // reverse-z depth comparison
			SEDX_CORE_ASSERT(s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Compare_depth)] != nullptr, "Failed to create sampler: Compare_depth");

			s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Point_clamp_edge)] = CreateRef<Sampler>(SamplerSpec{
				.min         = VK_FILTER_NEAREST,
				.mag         = VK_FILTER_NEAREST,
				.mipMap      = VK_SAMPLER_MIPMAP_MODE_NEAREST,
				.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
			});
			SEDX_CORE_ASSERT(s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Point_clamp_edge)] != nullptr, "Failed to create sampler: Point_clamp_edge");

			s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Point_clamp_border)] = CreateRef<Sampler>(SamplerSpec{
				.min         = VK_FILTER_NEAREST,
				.mag         = VK_FILTER_NEAREST,
				.mipMap      = VK_SAMPLER_MIPMAP_MODE_NEAREST,
				.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
			});
			SEDX_CORE_ASSERT(s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Point_clamp_border)] != nullptr, "Failed to create sampler: Point_clamp_border");

			s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Point_wrap)] = CreateRef<Sampler>(SamplerSpec{
				.min         = VK_FILTER_NEAREST,
				.mag         = VK_FILTER_NEAREST,
				.mipMap      = VK_SAMPLER_MIPMAP_MODE_NEAREST,
				.addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT
			});
			SEDX_CORE_ASSERT(s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Point_wrap)] != nullptr, "Failed to create sampler: Point_wrap ");

			s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Bilinear_clamp_edge)] = CreateRef<Sampler>(SamplerSpec{
				.min         = VK_FILTER_LINEAR,
				.mag         = VK_FILTER_LINEAR,
				.mipMap      = VK_SAMPLER_MIPMAP_MODE_NEAREST,
				.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
			});
			SEDX_CORE_ASSERT(s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Bilinear_clamp_edge)] != nullptr, "Failed to create sampler: Bilinear_clamp_edge");

			s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Bilinear_clamp_border)] = CreateRef<Sampler>(SamplerSpec{
				.min         = VK_FILTER_LINEAR,
				.mag         = VK_FILTER_LINEAR,
				.mipMap      = VK_SAMPLER_MIPMAP_MODE_NEAREST,
				.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
			});
			SEDX_CORE_ASSERT(s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Bilinear_clamp_border)] != nullptr, "Failed to create sampler: Bilinear_clamp_border");

			s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Bilinear_wrap)] = CreateRef<Sampler>(SamplerSpec{
				.min         = VK_FILTER_LINEAR,
				.mag         = VK_FILTER_LINEAR,
				.mipMap      = VK_SAMPLER_MIPMAP_MODE_NEAREST,
				.addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT
			});
			SEDX_CORE_ASSERT(s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Bilinear_wrap)] != nullptr, "Failed to create sampler: Bilinear_wrap");

			s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Trilinear_clamp)] = CreateRef<Sampler>(SamplerSpec{
				.min         = VK_FILTER_LINEAR,
				.mag         = VK_FILTER_LINEAR,
				.mipMap      = VK_SAMPLER_MIPMAP_MODE_LINEAR,
				.addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
			});
			SEDX_CORE_ASSERT(s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Trilinear_clamp)] != nullptr, "Failed to create sampler: Trilinear_clamp");

			SEDX_CORE_TRACE_TAG("Renderer", "Non-anisotropic samplers created");
		}
#pragma endregion

#pragma region Anisotropic sampler
		// Recreated when upscaling resolution changes to apply negative mip bias
		{
			float mipBiasNew = 0.0f;
			if (GetOutputResolution().x > GetRendererResolution().x)
			{
				mipBiasNew = log2f(GetRendererResolution().x / GetOutputResolution().x) - 1.0f;
			}

			static float mipBias = std::numeric_limits<float>::max();
			if (mipBiasNew != mipBias)
			{
				mipBias = mipBiasNew;
				s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Anisotropic_wrap)] = CreateRef<Sampler>(SamplerSpec{
					.min         = VK_FILTER_LINEAR,
					.mag         = VK_FILTER_LINEAR,
					.mipMap      = VK_SAMPLER_MIPMAP_MODE_LINEAR,
					.addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
					.compareFunc = VK_COMPARE_OP_ALWAYS,
					.anisotropy  = 16.0f,
					.mipLodBias  = mipBias
				});
				SEDX_CORE_ASSERT(s_Samplers[static_cast<uint8_t>(Renderer_Sampler::Anisotropic_wrap)] != nullptr, "Failed to create sampler: Anisotropic_wrap");
				SEDX_CORE_TRACE_TAG("Renderer", "Anisotropic sampler created (mip bias: {:.4f}, anisotropy: {:.1f})", mipBias, 16.0f);
			}
		}
#pragma endregion

		m_BindlessSamplers_Dirty.SetDirty();
		SEDX_CORE_TRACE_TAG("Renderer", "Sampler setup complete");
	}

	void Renderer::CreateStandardMeshes()
	{
		typedef std::vector<Vertex_PosTexNorTan> VertexVec;
		typedef std::vector<uint32_t> IndexVec;

		struct MeshDef
		{
			MeshType type;
			void (*generate)(VertexVec*, IndexVec*);
			const char* name;
		};

		// wrappers to bind default arguments for functions with extra parameters
		auto gen_sphere   = [](VertexVec* v, IndexVec* i) { MeshGenerator::GenerateSphere(v, i); };
		auto gen_cylinder = [](VertexVec* v, IndexVec* i) { MeshGenerator::GenerateCylinder(v, i); };
		auto gen_cone     = [](VertexVec* v, IndexVec* i) { MeshGenerator::GenerateCone(v, i); };

		const MeshDef definedMeshType[] =
		{
			{.type = MeshType::Cube, .generate = MeshGenerator::GenerateCube, .name = "standard_cube" },
			{.type = MeshType::Quad, .generate = MeshGenerator::GenerateQuad, .name = "standard_quad" },
			{.type = MeshType::Sphere, .generate = +gen_sphere, .name = "standard_sphere" },
			{.type = MeshType::Cylinder, .generate = gen_cylinder, .name = "standard_cylinder" },
			{.type = MeshType::Cone, .generate = gen_cone, .name = "standard_cone" },
		};

		const std::string projectDirectory = ResourceCache::GetProjectDirectory();
		for (const auto &[type, generate, name] : definedMeshType)
		{
			Ref<Mesh> mesh = CreateRef<Mesh>();
			VertexVec vertices;
			IndexVec indices;

			generate(&vertices, &indices);
			mesh->SetResourceFilePath(projectDirectory + name + ".edm" );
			mesh->SetFlag(static_cast<uint32_t>(MeshFlags::PostProcessOptimize), false);
			mesh->AddGeometry(vertices, indices, false);
			mesh->SetType(type);
			mesh->CreateGpuBuffers();

			s_StandardMeshes[static_cast<uint8_t>(type)] = mesh;
		}

		m_Lines_VertexBuffer = CreateRef<Buffer>();
	}

	void Renderer::CreateFonts()
	{
		const std::string dirFont = ResourceCache::GetResourceDirectory(ResourceDirectory::Fonts);

		uint32_t size = static_cast<uint32_t>(10 * Window::GetDpiScale());
		s_StandardFont = CreateRef<Font>(dirFont + "opensans/OpenSans-Medium.ttf", size, Color(0.9f, 0.9f, 0.9f, 1.0f));
	}

	void Renderer::CreateStandardMaterials()
	{
		const std::string dataDir = std::string(ResourceCache::GetResourceDirectory());
		IO::FileSystem::CreateDir(dataDir);

		s_StandardMaterial = CreateRef<MaterialAsset>();
		SEDX_CORE_ASSERT(s_StandardMaterial != nullptr, "Failed to create standard material");

		s_StandardMaterial->SetName("standard");
		s_StandardMaterial->SetAlbedoColor(Vec3(1.0f, 1.0f, 1.0f));
		s_StandardMaterial->SetMetalness(0.0f);
		s_StandardMaterial->SetRoughness(1.0f);
		s_StandardMaterial->SetEmission(0.0f);
	}

	void Renderer::CreateStandardTextures()
	{
	   const std::string dirTexture = ResourceCache::GetResourceDirectory(ResourceDirectory::Textures);

		for (Ref<ImageResource>& texture : s_StandardTextures)
		{
			texture.Reset();
		}

		#define standard_texture(x) s_StandardTextures[static_cast<uint32_t>(x)]

		auto create_texture = [](const char* name,
			const uint32_t width,
			const uint32_t height,
			const VkFormat format,
			const uint32_t flags,
			std::vector<std::byte> pixelBytes,
			const uint32_t bitsPerChannel,
			const uint32_t channelCount)
		{
			std::vector<MipBytes> mips = { MipBytes{ std::move(pixelBytes) } };
			std::vector<Slice> slices  = { Slice{ std::move(mips) } };
			Ref<ImageResource> texture = CreateRef<ImageResource>(
				ImgResourceSpec{
					.type = ImageType::Type2D,
					.width = width,
					.height = height,
					.depth = 1,
					.mipCount = 1,
					.format = format,
					.flags = flags,
					.name = name
			}, std::move(slices));

			if (texture)
			{
				texture->SetBitsPerChannel(bitsPerChannel);
				texture->SetChannelCount(channelCount);
			}

			return texture;
		};

		auto create_solid_texture = [&create_texture](const char* name, const std::byte r, const std::byte g, const std::byte b, const std::byte a)
		{
			return create_texture(name, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, ShaderViews | UnorderedAccessView,
				{ r, g, b, a }, 8, 4);
		};

		auto create_or_load_texture = [&](const Renderer_StandardTexture textureType,
			const char* name,
			const std::string& filePath,
			const std::byte fallbackR,
			const std::byte fallbackG,
			const std::byte fallbackB,
			const std::byte fallbackA)
		{
			VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
			uint32_t width  = 0;
			uint32_t height = 0;
			Memory::Buffer buffer = TextureImporter::ToBufferFromFile(filePath, format, width, height);

			if (buffer && width > 0 && height > 0 && buffer.GetSize() > 0)
			{
				std::vector<std::byte> pixels(buffer.GetSize());
				std::memcpy(pixels.data(), buffer.data, buffer.GetSize());
				buffer.Release();

				const uint32_t bitsPerChannel = format == VK_FORMAT_R32G32B32A32_SFLOAT ? 32u : 8u;
				standard_texture(textureType) = create_texture(name, width, height, format, ShaderViews | UnorderedAccessView, std::move(pixels), bitsPerChannel, 4);
				SEDX_CORE_ASSERT(standard_texture(textureType) != nullptr, "Failed to create loaded standard texture");
				standard_texture(textureType)->SetResourceFilePath(filePath);
				SEDX_CORE_INFO_TAG("Renderer", "Loaded standard texture '{}' from '{}' ({}x{})", name, filePath, width, height);
			}
			else
			{
				standard_texture(textureType) = create_solid_texture(name, fallbackR, fallbackG, fallbackB, fallbackA);
				SEDX_CORE_WARN_TAG("Renderer", "Failed to load '{}', using fallback for standard texture '{}'", filePath, name);
			}

			SEDX_CORE_ASSERT(standard_texture(textureType) != nullptr, "Failed to create standard texture");
		};

		create_or_load_texture(Renderer_StandardTexture::Noise_perlin, "noise_perlin", dirTexture + "noise_perlin.png", std::byte{ 127 }, std::byte{ 127 }, std::byte{ 127 }, std::byte{ 255 });
		create_or_load_texture(Renderer_StandardTexture::Noise_blue, "noise_blue", dirTexture + "noise_blue_0.png", std::byte{ 127 }, std::byte{ 127 }, std::byte{ 255 }, std::byte{ 255 });

		create_or_load_texture(Renderer_StandardTexture::Gizmo_light_directional, "gizmo_light_directional", dirTexture + "sun.png", std::byte{ 255 }, std::byte{ 230 }, std::byte{ 128 }, std::byte{ 255 });
		create_or_load_texture(Renderer_StandardTexture::Gizmo_light_point, "gizmo_light_point", dirTexture + "light_bulb.png", std::byte{ 255 }, std::byte{ 255 }, std::byte{ 160 }, std::byte{ 255 });
		create_or_load_texture(Renderer_StandardTexture::Gizmo_light_spot, "gizmo_light_spot", dirTexture + "flashlight.png", std::byte{ 255 }, std::byte{ 255 }, std::byte{ 255 }, std::byte{ 255 });

		create_or_load_texture(Renderer_StandardTexture::Checkerboard, "checkerboard", dirTexture + "no_texture.png", std::byte{ 180 }, std::byte{ 180 }, std::byte{ 180 }, std::byte{ 255 });

		standard_texture(Renderer_StandardTexture::Black) = create_solid_texture("black_texture", std::byte{ 0 }, std::byte{ 0 }, std::byte{ 0 }, std::byte{ 255 });
		standard_texture(Renderer_StandardTexture::White) = create_solid_texture("white_texture", std::byte{ 255 }, std::byte{ 255 }, std::byte{ 255 }, std::byte{ 255 });

		SEDX_CORE_ASSERT(standard_texture(Renderer_StandardTexture::Black) != nullptr, "Failed to create black texture");
		SEDX_CORE_ASSERT(standard_texture(Renderer_StandardTexture::White) != nullptr, "Failed to create white texture");
	}

	void Renderer::CreateBuffers()
	{
		uint32_t elementCount = DRAW_DATA_BUFFER_COUNT;

		// initialization values
		std::array<Instance, MAX_INSTANCE_COUNT> identity;
		identity.fill(Instance::GetIdentity());

		At(s_Buffers, Renderer_Buffer::ConstantFrame)      = CreateRef<Buffer>(sizeof(ConstantBuffer_Frame), elementCount, nullptr, true, "frame");
		At(s_Buffers, Renderer_Buffer::MaterialParameters) = CreateRef<Buffer>(static_cast<uint32_t>(sizeof(ShaderBuffer_Material)), MAX_ARRAY_SIZE, nullptr, true, "materials");
		At(s_Buffers, Renderer_Buffer::LightParameters)    = CreateRef<Buffer>(static_cast<uint32_t>(sizeof(ShaderBuffer_Light)), MAX_ARRAY_SIZE, nullptr, true, "lights");
		At(s_Buffers, Renderer_Buffer::DummyInstance)      = CreateRef<Buffer>(sizeof(Instance), static_cast<uint32_t>(identity.size()), &identity, true, "dummy_instance_buffer");
		At(s_Buffers, Renderer_Buffer::GeometryInfo)       = CreateRef<Buffer>(static_cast<uint32_t>(sizeof(ShaderBuffer_GeometryInfo)), MAX_ARRAY_SIZE, nullptr, true, "geometry_info");

		// single draw data and aabb buffers large enough for all frames; each frame writes to its
		// own offset region so the bindless descriptors never change, eliminating the race where
		// vkUpdateDescriptorSets (host-side, instantly visible under UPDATE_AFTER_BIND) would
		// change the buffer pointer while in-flight gpu commands were still reading from it
		At(s_Buffers, Renderer_Buffer::DrawData) = CreateRef<Buffer>(static_cast<uint32_t>(sizeof(ShaderBuffer_DrawData)), MAX_DRAW_CALLS * DRAW_DATA_BUFFER_COUNT, nullptr, true, "draw_data");
		At(s_Buffers, Renderer_Buffer::AABBs) = CreateRef<Buffer>(static_cast<uint32_t>(sizeof(ShaderBuffer_Aabb)), MAX_ARRAY_SIZE * DRAW_DATA_BUFFER_COUNT, nullptr, true, "aabbs");

		// per-frame rotated buffers
		uint32_t drawCountInit = 0;
		for (uint32_t i = 0; i < DRAW_DATA_BUFFER_COUNT; i++)
		{
			IndirectFrameResource& fr = m_FrameResources[i];

			fr.m_DrawArgs = CreateRef<Buffer>(static_cast<uint32_t>(sizeof(ShaderBuffer_IndirectDrawArgs)), MAX_ARRAY_SIZE, nullptr, true, (std::string("indirect_draw_args_") + std::to_string(i)).c_str());
			fr.m_DrawData = CreateRef<Buffer>(static_cast<uint32_t>(sizeof(ShaderBuffer_DrawData)), MAX_ARRAY_SIZE, nullptr, true, (std::string("indirect_draw_data_") + std::to_string(i)).c_str());

			fr.m_DrawArgs_Out = CreateRef<Buffer>(static_cast<uint32_t>(sizeof(ShaderBuffer_IndirectDrawArgs)), MAX_ARRAY_SIZE, nullptr, true, (std::string("indirect_draw_args_out_") + std::to_string(i)).c_str());
			fr.m_DrawData_Out = CreateRef<Buffer>(static_cast<uint32_t>(sizeof(ShaderBuffer_DrawData)), MAX_ARRAY_SIZE, nullptr, true, (std::string("indirect_draw_data_out_") + std::to_string(i)).c_str());

			fr.m_DrawCount = CreateRef<Buffer>(static_cast<uint32_t>(sizeof(uint32_t)), 1, &drawCountInit, true, (std::string("indirect_draw_count_") + std::to_string(i)).c_str());

		}

		// point the active buffer slots at frame 0
		const IndirectFrameResource& fr = m_FrameResources[0];
		At(s_Buffers, Renderer_Buffer::IndirectDrawArgs)    = fr.m_DrawArgs;
		At(s_Buffers, Renderer_Buffer::IndirectDrawData)    = fr.m_DrawData;
		At(s_Buffers, Renderer_Buffer::IndirectDrawArgsOut) = fr.m_DrawArgs_Out;
		At(s_Buffers, Renderer_Buffer::IndirectDrawDataOut) = fr.m_DrawData_Out;
		At(s_Buffers, Renderer_Buffer::IndirectDrawCount)   = fr.m_DrawCount;

		// particle buffers
		constexpr uint32_t particleMax = 100000;
		uint32_t particleCounterInit[2] = { 0, 0 };
		At(s_Buffers, Renderer_Buffer::ParticleBufferA) = CreateRef<Buffer>(static_cast<uint32_t>(sizeof(ShaderBuffer_Particle)), particleMax, nullptr, true, "particle_buffer_a");
		At(s_Buffers, Renderer_Buffer::ParticleCounter) = CreateRef<Buffer>(static_cast<uint32_t>(sizeof(uint32_t)), 2, particleCounterInit, true, "particle_counter");
		At(s_Buffers, Renderer_Buffer::ParticleEmitter) = CreateRef<Buffer>(static_cast<uint32_t>(sizeof(ShaderBuffer_EmitterParams)), 1, nullptr, true, "particle_emitter");
	}

	void Renderer::CreateDepthStencilStates()
	{
		// arguments: depth_test, depth_write, depth_function, stencil_test, stencil_write, stencil_function
		At(s_DepthStencilStates, Renderer_DepthStencilState::Off)              = CreateRef<DepthStencilState>(DepthStencilSpec{false, false, VkCompareOp::VK_COMPARE_OP_NEVER});
		At(s_DepthStencilStates, Renderer_DepthStencilState::ReadEqual)        = CreateRef<DepthStencilState>(DepthStencilSpec{true,  false, VkCompareOp::VK_COMPARE_OP_EQUAL});
		At(s_DepthStencilStates, Renderer_DepthStencilState::ReadGreaterEqual) = CreateRef<DepthStencilState>(DepthStencilSpec{true,  false, VkCompareOp::VK_COMPARE_OP_GREATER_OR_EQUAL});
		At(s_DepthStencilStates, Renderer_DepthStencilState::ReadWrite)        = CreateRef<DepthStencilState>(DepthStencilSpec{true,  true,  VkCompareOp::VK_COMPARE_OP_GREATER_OR_EQUAL});
	}

	void Renderer::CreateRasterizerStates()
	{
		// bias done in shader, hardware bias is uncontrollable across cascades
		float bias            = 0.0f;
		float biasClamp       = 0.0f;
		float biasSlopeScaled = 0.0f;
		float lineWidth       = 3.0f;

		At(s_RasterizerStates, Renderer_RasterizerState::Solid)             = CreateRef<RasterizerState>(RasterStateSpec{PolygonMode::Solid, true, false,0.0f, 0.0f, 0.0f, lineWidth});
		At(s_RasterizerStates, Renderer_RasterizerState::Wireframe)         = CreateRef<RasterizerState>(RasterStateSpec{PolygonMode::Wireframe, true, false,0.0f, 0.0f, 0.0f, lineWidth});
		At(s_RasterizerStates, Renderer_RasterizerState::Light_point_spot)  = CreateRef<RasterizerState>(RasterStateSpec{PolygonMode::Solid, true, true, bias, biasClamp, biasSlopeScaled, lineWidth});
		At(s_RasterizerStates, Renderer_RasterizerState::Light_directional) = CreateRef<RasterizerState>(RasterStateSpec{PolygonMode::Solid, false, true, bias * 0.5f, biasClamp, biasSlopeScaled, lineWidth});
	}

	void Renderer::CreateBlendStates()
	{
		At(s_BlendStates, Renderer_BlendState::Off)      = CreateRef<BlendState>(BlendStateSpec{false});
		At(s_BlendStates, Renderer_BlendState::Alpha)    = CreateRef<BlendState>(BlendStateSpec{true, VkBlendFactor::VK_BLEND_FACTOR_SRC_ALPHA, VkBlendFactor::VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VkBlendOp::VK_BLEND_OP_ADD, VkBlendFactor::VK_BLEND_FACTOR_ONE, VkBlendFactor::VK_BLEND_FACTOR_ONE, VkBlendOp::VK_BLEND_OP_ADD, 0.0f});
		At(s_BlendStates, Renderer_BlendState::Additive) = CreateRef<BlendState>(BlendStateSpec{true, VkBlendFactor::VK_BLEND_FACTOR_ONE, VkBlendFactor::VK_BLEND_FACTOR_ONE, VkBlendOp::VK_BLEND_OP_ADD, VkBlendFactor::VK_BLEND_FACTOR_ONE, VkBlendFactor::VK_BLEND_FACTOR_ONE, VkBlendOp::VK_BLEND_OP_ADD, 1.0f});
	}

	ImageResource* Renderer::GetStandardTexture(const Renderer_StandardTexture type)
	{
		return s_StandardTextures[static_cast<uint8_t>(type)].Get();
	}

	Sampler* Renderer::GetSampler(const Renderer_Sampler type)
	{
		return s_Samplers[static_cast<uint8_t>(type)].Get();
	}

	std::array<Ref<Sampler>, static_cast<uint32_t>(Renderer_Sampler::MaxEnum)> &Renderer::GetSamplers()
	{
		return s_Samplers;
	}

#pragma endregion

	std::array<Ref<ImageResource>, static_cast<uint32_t>(Renderer_RenderTarget::MaxEnum)> &Renderer::GetRenderTargets()
	{
		return s_RenderTargets;
	}

	std::array<Ref<Buffer>, static_cast<uint32_t>(Renderer_Buffer::MaxEnum)> &Renderer::GetStructuredBuffers()
	{
		return s_Buffers;
	}

	Buffer *Renderer::GetBuffer(const Renderer_Buffer type)
	{
		return s_Buffers[static_cast<uint8_t>(type)].Get();
	}

	RasterizerState *Renderer::GetRasterizerState(const Renderer_RasterizerState type)
	{
		SEDX_CORE_ASSERT(static_cast<uint8_t>(type) < static_cast<uint8_t>(Renderer_RasterizerState::MaxEnum), "Renderer_RasterizerState out of range");
		return &s_RasterizerStates[static_cast<uint8_t>(type)];
	}

	BlendState *Renderer::GetBlendState(const Renderer_BlendState type)
	{
		SEDX_CORE_ASSERT(static_cast<uint8_t>(type) < static_cast<uint8_t>(Renderer_BlendState::MaxEnum), "Renderer_BlendState out of range");
		return &s_BlendStates[static_cast<uint8_t>(type)];
	}

	Ref<Font> &Renderer::GetFont()
	{
		return s_StandardFont;
	}

	Ref<MaterialAsset> &Renderer::GetStandardMaterial()
	{
		return s_StandardMaterial;
	}

	DepthStencilState *Renderer::GetDepthStencilState(const Renderer_DepthStencilState type)
	{
		SEDX_CORE_ASSERT(static_cast<uint8_t>(type) < static_cast<uint8_t>(Renderer_DepthStencilState::MaxEnum), "Renderer_DepthStencilState out of range");
		return &s_DepthStencilStates[static_cast<uint8_t>(type)];
	}

	Mesh *Renderer::GetStandardMesh(const MeshType /*type*/)
	{
		class StandardQuadMesh final : public Mesh
		{
		public:
			[[nodiscard]] static Buffer* GetVertexBuffer() { return GeometryBuffer::GetVertexBuffer(); }
			[[nodiscard]] static Buffer* GetIndexBuffer() { return GeometryBuffer::GetIndexBuffer(); }
		};

		static StandardQuadMesh s_QuadMesh;
		Buffer* vtxB = StandardQuadMesh::GetVertexBuffer();
		Buffer* indxB = StandardQuadMesh::GetIndexBuffer();

		if (vtxB && indxB)
			return &s_QuadMesh;

		static bool warned = false;
		if (!warned)
		{
			SEDX_CORE_WARN_TAG("Renderer", "GetStandardMesh: quad mesh unavailable (geometry buffer not ready)");
			warned = true;
		}

		return nullptr;
	}

	bool Renderer::IsCpuDrivenDraw(const Renderer_DrawCall& drawCall, const MaterialAsset* material)
	{
		bool isTessellated = material->GetProperty(MaterialProperty::Tessellation) > 0.0f;
		bool isInstanced = drawCall.instanceCount > 1;
		bool isAlphaTested = material->IsAlphaTested();
		bool isNonStandardCull = static_cast<CullMode>(material->GetProperty(MaterialProperty::CullMode)) != CullMode::Back;
		return isTessellated || isInstanced || isAlphaTested || isNonStandardCull;
	}

	void Renderer::RotateFrameBuffers()
	{
		m_FrameResource_Index = (m_FrameResource_Index + 1) % DRAW_DATA_BUFFER_COUNT;
		const IndirectFrameResource& fr = m_FrameResources[m_FrameResource_Index];

		s_Buffers[static_cast<uint8_t>(Renderer_Buffer::IndirectDrawArgs)]    = fr.m_DrawArgs;
		s_Buffers[static_cast<uint8_t>(Renderer_Buffer::IndirectDrawData)]    = fr.m_DrawData;
		s_Buffers[static_cast<uint8_t>(Renderer_Buffer::IndirectDrawArgsOut)] = fr.m_DrawArgs_Out;
		s_Buffers[static_cast<uint8_t>(Renderer_Buffer::IndirectDrawDataOut)] = fr.m_DrawData_Out;
		s_Buffers[static_cast<uint8_t>(Renderer_Buffer::IndirectDrawCount)]   = fr.m_DrawCount;
	}

}

// -------------------------------------------------------
