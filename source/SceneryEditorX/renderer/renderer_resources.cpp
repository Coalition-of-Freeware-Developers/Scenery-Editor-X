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
#include "renderer.h"
#include "renderer_declarations.h"
#include "SceneryEditorX/core/platform/settings/settings.h"
#include "vulkan/image_resource.h"
#include "vulkan/sampler.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

#pragma region Resource Declarations

	// Renderer resources
	static std::array<Ref<Shader>,  static_cast<uint32_t>(Renderer_Shader::MaxEnum)>			 s_Shaders;
	static std::array<Ref<Sampler>, static_cast<uint32_t>(Renderer_Sampler::MaxEnum)>			 s_Samplers;
	static std::array<Ref<Buffer>,  static_cast<uint32_t>(Renderer_Buffer::MaxEnum)>			 s_Buffers;
	static std::array<Ref<ImageResource>, static_cast<uint32_t>(Renderer_RenderTarget::MaxEnum)> s_RenderTargets;

	// Static state object instances (created once, never mutated after init)
	static std::array<RasterizerState,   static_cast<uint8_t>(Renderer_RasterizerState::MaxEnum)>   s_RasterizerStates  = {
		RasterizerState{ PolygonMode::Solid,     false },   // Solid
		RasterizerState{ PolygonMode::Wireframe, false },   // Wireframe
		RasterizerState{ PolygonMode::Solid,     true,  1.0f, 1.75f }, // Light_point_spot  (depth bias)
		RasterizerState{ PolygonMode::Solid,     true,  1.0f, 2.00f }, // Light_directional (depth bias)
	};
	static std::array<BlendState, static_cast<uint8_t>(Renderer_BlendState::MaxEnum)> s_BlendStates = {
		BlendState{ false },   // Off
		BlendState{ true,  VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
							VK_BLEND_FACTOR_ONE,      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD }, // Alpha
		BlendState{ true,  VK_BLEND_FACTOR_ONE,       VK_BLEND_FACTOR_ONE,                VK_BLEND_OP_ADD,
							VK_BLEND_FACTOR_ONE,      VK_BLEND_FACTOR_ONE,                VK_BLEND_OP_ADD }, // Additive
		BlendState{ true,  VK_BLEND_FACTOR_ONE,       VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
							VK_BLEND_FACTOR_ONE,      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD }, // Premultiplied
	};
	static std::array<DepthStencilState, static_cast<uint8_t>(Renderer_DepthStencilState::MaxEnum)> s_DepthStencilStates = {
		DepthStencilState{ false, false, VK_COMPARE_OP_ALWAYS    }, // Off
		DepthStencilState{ true,  false, VK_COMPARE_OP_EQUAL     }, // ReadEqual
		DepthStencilState{ true,  false, VK_COMPARE_OP_GREATER_OR_EQUAL }, // ReadGreaterEqual (reverse-z)
		DepthStencilState{ true,  true,  VK_COMPARE_OP_GREATER   }, // ReadWrite (reverse-z)
	};

	// Static members defined here (declared in renderer.h)
	PushConstantBuffer Renderer::m_pcb_pass_cpu;
	uint32_t           Renderer::m_draw_call_count          = 0;
	uint32_t           Renderer::m_draw_calls_prepass_count = 0;
	uint32_t           Renderer::m_indirect_draw_count      = 0;
	bool               Renderer::m_transparents_present     = false;
	bool               Renderer::m_is_hiz_suppressed        = false;

#pragma endregion


    void Renderer::CreateRenderTargets(const bool createRender, const bool createOutput, const bool createDynamic)
    {
        uint32_t widthRender  = static_cast<uint32_t>(GetRendererResolution().x);
        uint32_t heightRender = static_cast<uint32_t>(GetRendererResolution().y);
        uint32_t widthOutput  = static_cast<uint32_t>(GetOutputResolution().x);
        uint32_t heightOutput = static_cast<uint32_t>(GetOutputResolution().y);

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

        // avoid combining uav + rtv on frequently accessed targets (forces suboptimal layouts on amd)
        // resolution - render
        if (createRender)
        {
            // frame
            {
				s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_render)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D,
					widthRender, heightRender, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, UnorderedAccessView | ShaderViews | RenderTargetViews | BlitClear, "frame_render"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_render)] != nullptr, "Failed to create frame_render render target");

                s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_render)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D,
					widthRender, heightRender, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, ShaderViews | UnorderedAccessView | RenderTargetViews | BlitClear, "frame_render"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_render)] != nullptr, "Failed to create frame_render render target");

                s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_render_opaque)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D,
					widthRender, heightRender, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, ShaderViews | RenderTargetViews | BlitClear, "frame_render_opaque"});
				SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_render_opaque)] != nullptr, "Failed to create frame_render_opaque render target");
            }

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

            // misc
            s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::sss)]                = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2DArray, widthRender, heightRender, 4, 1, VK_FORMAT_R16_SFLOAT, UnorderedAccessView | ShaderViews | BlitClear | QueueShare, "sss"});
            SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::sss)] != nullptr, "Failed to create sss render target");
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

        // resolution - output
        if (createOutput)
        {
            // frame
            uint32_t mipCount = compute_mip_count(widthOutput, heightOutput, 16);
            s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_output)]   = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, widthOutput, heightOutput, 1, mipCount, VK_FORMAT_R16G16B16A16_SFLOAT, UnorderedAccessView | ShaderViews | RenderTargetViews | BlitClear | PerMipViews | QueueShare, "frame_output"});
            SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::frame_output)] != nullptr, "Failed to create frame_output render target");
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
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::lut_atmosphere_multiscatter)] != nullptr, "Failed to create lut_atmosphere_multiscatter render target");

            // Misc
            s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::blur)] = CreateRef<ImageResource>(ImgResourceSpec{ImageType::Type2D, 4096, 4096, 1, 1, VK_FORMAT_R16G16B16A16_SFLOAT, UnorderedAccessView | ShaderViews, "blur_scratch"});
			SEDX_CORE_ASSERT(s_RenderTargets[static_cast<uint8_t>(Renderer_RenderTarget::blur)] != nullptr, "Failed to create blur render target");

            const uint32_t lowestDimension = 16; // lowest mip is 16x16, preserving directional detail for diffuse IBL (1x1 loses directionality)
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
    }

    void Renderer::UpdateOptionalRenderTargets()
    {
        uint32_t width  = static_cast<uint32_t>(GetRendererResolution().x);
        uint32_t height = static_cast<uint32_t>(GetRendererResolution().y);
        uint32_t flags  = UnorderedAccessView | ShaderViews | BlitClear;
        
        // (SSAO) Screen-space ambient occlusion - optional since it can be expensive and not noticeable in many scenes
        bool needSSAO = false; /* TODO: Have the option settable in user settings */;
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
        
        /*
        // restir reservoirs
        bool need_restir = cvar_restir_pt.GetValueAs<bool>() && RHI_Device::IsSupportedRayTracing();
        if (need_restir && !render_target(Renderer_RenderTarget::restir_reservoir0))
        {
            uint32_t restir_flags = flags | QueueShare;

            static const char* reservoir_names[] =
            {
                "restir_reservoir0",         "restir_reservoir1",         "restir_reservoir2",         "restir_reservoir3",         "restir_reservoir4",
                "restir_reservoir_prev0",    "restir_reservoir_prev1",    "restir_reservoir_prev2",    "restir_reservoir_prev3",    "restir_reservoir_prev4",
                "restir_reservoir_spatial0", "restir_reservoir_spatial1", "restir_reservoir_spatial2", "restir_reservoir_spatial3", "restir_reservoir_spatial4",
            };

            for (uint32_t i = 0; i < 15; i++)
            {
                auto rt = static_cast<Renderer_RenderTarget>(static_cast<uint32_t>(Renderer_RenderTarget::restir_reservoir0) + i);
                render_target(rt) = CreateRef<RHI_Texture>(ImageType::Type2D, width, height, 1, 1, VkFormat::R32G32B32A32_Float, restir_flags, reservoir_names[i]);
            }
            
            // nrd denoiser
            render_target(Renderer_RenderTarget::nrd_viewz)                    = CreateRef<RHI_Texture>(RHI_Texture_Type::Type2D, width, height, 1, 1, VkFormat::R16_Float,          restir_flags, "nrd_viewz");
            render_target(Renderer_RenderTarget::nrd_normal_roughness)         = CreateRef<RHI_Texture>(RHI_Texture_Type::Type2D, width, height, 1, 1, VkFormat::R10G10B10A2_Unorm,  restir_flags, "nrd_normal_roughness");
            render_target(Renderer_RenderTarget::nrd_diff_radiance_hitdist)    = CreateRef<RHI_Texture>(RHI_Texture_Type::Type2D, width, height, 1, 1, VkFormat::R16G16B16A16_Float, restir_flags, "nrd_diff_radiance_hitdist");
            render_target(Renderer_RenderTarget::nrd_spec_radiance_hitdist)    = CreateRef<RHI_Texture>(RHI_Texture_Type::Type2D, width, height, 1, 1, VkFormat::R16G16B16A16_Float, restir_flags, "nrd_spec_radiance_hitdist");
            render_target(Renderer_RenderTarget::nrd_out_diff_radiance_hitdist)= CreateRef<RHI_Texture>(RHI_Texture_Type::Type2D, width, height, 1, 1, VkFormat::R16G16B16A16_Float, restir_flags, "nrd_out_diff_radiance_hitdist");
            render_target(Renderer_RenderTarget::nrd_out_spec_radiance_hitdist)= CreateRef<RHI_Texture>(RHI_Texture_Type::Type2D, width, height, 1, 1, VkFormat::R16G16B16A16_Float, restir_flags, "nrd_out_spec_radiance_hitdist");
        }
        else if (!need_restir && render_target(Renderer_RenderTarget::restir_reservoir0))
        {
            for (uint32_t i = 0; i < 15; i++)
            {
                auto rt = static_cast<Renderer_RenderTarget>(static_cast<uint32_t>(Renderer_RenderTarget::restir_reservoir0) + i);
                render_target(rt) = nullptr;
            }
            
            render_target(Renderer_RenderTarget::nrd_viewz)                     = nullptr;
            render_target(Renderer_RenderTarget::nrd_normal_roughness)          = nullptr;
            render_target(Renderer_RenderTarget::nrd_diff_radiance_hitdist)     = nullptr;
            render_target(Renderer_RenderTarget::nrd_spec_radiance_hitdist)     = nullptr;
            render_target(Renderer_RenderTarget::nrd_out_diff_radiance_hitdist) = nullptr;
            render_target(Renderer_RenderTarget::nrd_out_spec_radiance_hitdist) = nullptr;
        }
        */

    }

    ImageResource* Renderer::GetRenderTarget(const Renderer_RenderTarget type)
    {
        return s_RenderTargets[static_cast<uint8_t>(type)].Get();
    }

    Shader* Renderer::GetShader(const Renderer_Shader type)
    {
        return s_Shaders[static_cast<uint8_t>(type)].Get();
    }

#pragma region Samplers

    void Renderer::CreateSamplers()
    {
        SEDX_CORE_TRACE_TAG("Renderer", "Creating Renderer Samplers");

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

        // Anisotropic sampler - recreated when upscaling resolution changes to apply negative mip bias
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

        m_BindlessSamplers_Dirty = true;
        SEDX_CORE_TRACE_TAG("Renderer", "Sampler setup complete");
    }

    Sampler *Renderer::GetSampler(const Renderer_Sampler type)
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

    std::array<Ref<Shader>, static_cast<uint32_t>(Renderer_Shader::MaxEnum)> &Renderer::GetShaders()
    {
        return s_Shaders;
    }

    std::array<Ref<Buffer>, static_cast<uint32_t>(Renderer_Buffer::MaxEnum)> &Renderer::GetStructuredBuffers()
    {
        return s_Buffers;
    }

    Buffer* Renderer::GetBuffer(const Renderer_Buffer type)
    {
        return s_Buffers[static_cast<uint8_t>(type)].Get();
    }

    RasterizerState* Renderer::GetRasterizerState(const Renderer_RasterizerState type)
    {
        SEDX_CORE_ASSERT(static_cast<uint8_t>(type) < static_cast<uint8_t>(Renderer_RasterizerState::MaxEnum),
                         "Renderer_RasterizerState out of range");
        return &s_RasterizerStates[static_cast<uint8_t>(type)];
    }

    BlendState* Renderer::GetBlendState(const Renderer_BlendState type)
    {
        SEDX_CORE_ASSERT(static_cast<uint8_t>(type) < static_cast<uint8_t>(Renderer_BlendState::MaxEnum),
                         "Renderer_BlendState out of range");
        return &s_BlendStates[static_cast<uint8_t>(type)];
    }

    DepthStencilState* Renderer::GetDepthStencilState(const Renderer_DepthStencilState type)
    {
        SEDX_CORE_ASSERT(static_cast<uint8_t>(type) < static_cast<uint8_t>(Renderer_DepthStencilState::MaxEnum),
                         "Renderer_DepthStencilState out of range");
        return &s_DepthStencilStates[static_cast<uint8_t>(type)];
    }

    Mesh* Renderer::GetStandardMesh(const MeshType /*type*/)
    {
        // TODO: Return the pre-built standard mesh for the given type once the mesh registry is wired in.
        SEDX_CORE_WARN_TAG("Renderer", "GetStandardMesh: stub — mesh registry not yet connected");
        return nullptr;
    }

    uint32_t Renderer::WriteDrawData(const xMath::Matrix& /*transform*/)
    {
        // TODO: Write the transform matrix into the GPU draw-data structured buffer and return its index.
        SEDX_CORE_WARN_TAG("Renderer", "WriteDrawData: stub — draw-data buffer not yet wired");
        return m_DrawDataCount++;
    }

    bool Renderer::IsCpuDrivenDraw(const Renderer_DrawCall& /*drawCall*/, const Material* /*material*/)
    {
        // TODO: Return false for draws that have been batched into the GPU indirect path.
        return true;
    }

}

// -------------------------------------------------------
