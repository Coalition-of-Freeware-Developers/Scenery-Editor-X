/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* render_resource.cpp
* -------------------------------------------------------
* Created: 5/9/2025
* -------------------------------------------------------
*/
#include "render_resource.h"
#include <SceneryEditorX/scene/lights.h>
#include "blend_state.h"
#include "rasterizer.h"
#include "renderer.h"
#include "sampler.h"
#include "shaders/shader.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

    namespace
    {
        // Graphics States
        std::array<Ref<Rasterizer>, static_cast<uint32_t>(RasterizerState::MaxEnum)> rasterizer_states;
        std::array<Ref<DepthStencilState>, static_cast<uint32_t>(DepthStencilState::MaxEnum)> depth_stencil_states;
        std::array<Ref<BlendState>, 3> blend_states;

        // Renderer Resources
        std::array<Ref<Texture2D>, static_cast<uint32_t>(RenderTarget::MaxEnum)> render_targets;
        std::array<Ref<Shader>, static_cast<uint32_t>(ShaderType::MaxEnum)> shaders;
        std::array<Ref<Sampler>, static_cast<uint32_t>(SamplerPreset::MaxEnum)> samplers;
        std::array<Ref<Buffer>, static_cast<uint32_t>(RendererBufferId::MaxEnum)> buffers;

        // Asset Resources
        std::array<Ref<Texture2D>, static_cast<uint32_t>(StandardTexture::MaxEnum)> standard_textures;
        //std::array<Ref<Mesh>, static_cast<uint32_t>(PrimitiveType::MaxEnum)> standard_meshes;
        //Ref<Font> standard_font;
        //Ref<Material> standard_material;
    }

    /// -------------------------------------------------------

    //Ref<Font>& Renderer::GetFont() { return standard_font; }
    //Ref<Material>& Renderer::GetStandardMaterial() { return standard_material; }

    /// -------------------------------------------------------

    void Renderer::CreateDepthStencilStates()
    {
        #define depth_stencil_state(x) depth_stencil_states[static_cast<uint8_t>(x)]

        // arguments: depth_test, depth_write, depth_function, stencil_test, stencil_write, stencil_function
        depth_stencil_state(DepthStencilState::Off)              = CreateRef<DepthStencilState>(false, false, DepthCompareOperator::Never);
        depth_stencil_state(DepthStencilState::ReadEqual)        = CreateRef<DepthStencilState>(true,  false, DepthCompareOperator::Equal);
        depth_stencil_state(DepthStencilState::ReadGreaterEqual) = CreateRef<DepthStencilState>(true,  false, DepthCompareOperator::GreaterOrEqual);
        depth_stencil_state(DepthStencilState::ReadWrite)        = CreateRef<DepthStencilState>(true,  true,  DepthCompareOperator::GreaterOrEqual);
    }

    void Renderer::CreateRasterizerStates()
    {
        float bias              = LightNode::GetBias();
        float bias_clamp        = 0.0f;
        float bias_slope_scaled = LightNode::GetBiasSlopeScaled();
        float line_width        = 3.0f;

        #define rasterizer_state(x) rasterizer_states[static_cast<uint8_t>(x)]

		/**
		 * Raster State Layout: Fill Mode, Depth Clipping, Bias, Bias Clamp, Slope Scaled Bias, Line Width
		 */
        rasterizer_state(RasterizerState::Solid)					= CreateRef<Rasterizer>(PolygonMode::Fill, true, 0.0f, 0.0f, 0.0f, line_width);
        rasterizer_state(RasterizerState::Wireframe)				= CreateRef<Rasterizer>(PolygonMode::Line, true, 0.0f, 0.0f, 0.0f, line_width);
        rasterizer_state(RasterizerState::PointSpot_Light)			= CreateRef<Rasterizer>(PolygonMode::Fill, true, bias, bias_clamp, bias_slope_scaled, line_width);
        rasterizer_state(RasterizerState::DirectionalSpot_Light)	= CreateRef<Rasterizer>(PolygonMode::Fill, false, bias * 0.1f, bias_clamp, bias_slope_scaled, line_width);
    }
	
    void Renderer::CreateBlendModes()
    {
        #define blend_state(x) blend_states[static_cast<uint8_t>(x)]

		BlendStateDesc alphaBlend;
        alphaBlend.src = BlendSpec::Src_Alpha;
        alphaBlend.dst = BlendSpec::Inv_Src_Alpha;
        alphaBlend.blendOp = BlendOperation::Add;
        alphaBlend.srcAlpha = BlendSpec::One;
        alphaBlend.dstAlpha = BlendSpec::One;
        alphaBlend.blendOpAlpha = BlendOperation::Add;

		BlendStateDesc additiveBlend;
        additiveBlend.src = BlendSpec::One;
        additiveBlend.dst = BlendSpec::One;
        additiveBlend.blendOp = BlendOperation::Add;
        additiveBlend.srcAlpha = BlendSpec::One;
        additiveBlend.dstAlpha = BlendSpec::One;
        additiveBlend.blendOpAlpha = BlendOperation::Add;

        // blend_enabled, source_blend, dest_blend, blend_op, source_blend_alpha, dest_blend_alpha, blend_op_alpha, blend_factor
        blend_state(BlendMode::None)	 = CreateRef<BlendState>(false);
        blend_state(BlendMode::Alpha)	 = CreateRef<BlendState>(true, alphaBlend, 0.0f);
        blend_state(BlendMode::Additive) = CreateRef<BlendState>(true, additiveBlend, 1.0f);
    }

    void Renderer::CreateSamplers()
    {
        #define sampler(type, filter_min, filter_mag, filter_mip, address_mode, comparison_func, anisotropy, comparison_enabled, mip_bias) \
        samplers[static_cast<uint8_t>(type)] = CreateRef<Sampler>(filter_min, filter_mag, filter_mip, address_mode, comparison_func, anisotropy, comparison_enabled, mip_bias)

        // Non-Anisotropic
        {
            static bool samplers_created = false;
            if (!samplers_created)
            {
                sampler(SamplerPreset::CompareDepth,			SamplerFilter::Linear,  SamplerFilter::Linear,  SamplerFilter::Nearest, SamplerWrap::Repeat, DepthCompareOperator::Greater, 0.0f, true,  0.0f); // Reverse-Z
                sampler(SamplerPreset::PointClampEdge,			SamplerFilter::Nearest, SamplerFilter::Nearest, SamplerFilter::Nearest, SamplerWrap::ClampEdge,       DepthCompareOperator::Never,   0.0f, false, 0.0f);
                sampler(SamplerPreset::PointClampBorder,		SamplerFilter::Nearest, SamplerFilter::Nearest, SamplerFilter::Nearest, SamplerWrap::ClampBorder, DepthCompareOperator::Never,   0.0f, false, 0.0f);
                sampler(SamplerPreset::PointWrap,				SamplerFilter::Nearest, SamplerFilter::Nearest, SamplerFilter::Nearest, SamplerWrap::Repeat,        DepthCompareOperator::Never,   0.0f, false, 0.0f);
                sampler(SamplerPreset::BilinearClampEdge,		SamplerFilter::Linear,  SamplerFilter::Linear,  SamplerFilter::Nearest, SamplerWrap::ClampEdge,       DepthCompareOperator::Never,   0.0f, false, 0.0f);
                sampler(SamplerPreset::BilinearClampBorder,		SamplerFilter::Linear,  SamplerFilter::Linear,  SamplerFilter::Nearest, SamplerWrap::ClampBorder, DepthCompareOperator::Never,   0.0f, false, 0.0f);
                sampler(SamplerPreset::BilinearWrap,			SamplerFilter::Linear,  SamplerFilter::Linear,  SamplerFilter::Nearest, SamplerWrap::Repeat,        DepthCompareOperator::Never,   0.0f, false, 0.0f);
                sampler(SamplerPreset::TrilinearClamp,			SamplerFilter::Linear,  SamplerFilter::Linear,  SamplerFilter::Linear,  SamplerWrap::ClampEdge,       DepthCompareOperator::Never,   0.0f, false, 0.0f);

                samplers_created = true;
            }
        }

        // Anisotropic
        {
            /**
             * compute mip bias for enhanced texture detail in upsampling, applicable when output resolution is higher than render resolution
             * this adjustment, beneficial even without FSR, ensures textures remain detailed at higher output resolutions by applying a negative bias
             */
            float mip_bias_new = 0.0f;
            if (GetResolutionOutput().x > GetResolutionRender().x)
                mip_bias_new = log2(GetResolutionRender().x / GetResolutionOutput().x) - 1.0f;

            static float mip_bias = std::numeric_limits<float>::max();
            if (mip_bias_new != mip_bias)
            {
                mip_bias = mip_bias_new;
                float anisotropy = GetOption<float>(Renderer_Option::Anisotropy);
                sampler(SamplerPreset::AnisotropicWrap, SamplerFilter::Linear, SamplerFilter::Linear, SamplerFilter::Linear, SamplerWrap::Repeat, DepthCompareOperator::Always, anisotropy, false, mip_bias);
            }
        }

        m_bindless_samplers_dirty = true;
    }

    void Renderer::CreateRenderTargets(const bool create_render, const bool create_output, const bool create_dynamic)
    {
        // Get render and output resolutions
        uint32_t width_render  = static_cast<uint32_t>(GetResolutionRender().x);
        uint32_t height_render = static_cast<uint32_t>(GetResolutionRender().y);
        uint32_t width_output  = static_cast<uint32_t>(GetResolutionOutput().x);
        uint32_t height_output = static_cast<uint32_t>(GetResolutionOutput().y);

        auto compute_mip_count = [&width_render, &height_render](const uint32_t smallest_dimension)
        {
            uint32_t max_dimension = std::max(width_render, height_render);
            uint32_t mip_count     = 1;
        
            while (max_dimension > smallest_dimension)
            {
                max_dimension /= 2;
                mip_count++;
            }
        
            return mip_count;
        };

        #define render_target(x) render_targets[static_cast<uint8_t>(x)]

        // Resolution - Render
        if (create_render)
        {
            // Frame
            {
                render_target(RenderTarget::FrameRender) = CreateRef<Texture2D>(ResourceType::Texture2D, width_render, height_render, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, Texture_Uav | Texture_Srv | Texture_Rtv | ClearBlit, "frame_render");
                render_target(RenderTarget::FrameRenderOpaque) = CreateRef<Texture2D>(ResourceType::Texture2D, width_render, height_render, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, Texture_Uav | Texture_Srv | Texture_Rtv | ClearBlit, "frame_render_opaque");
            }

            // G-Buffer
            {
                uint32_t flags = Texture_Srv | Texture_Rtv | ClearBlit;
                render_target(RenderTarget::gBuffer_Color) = CreateRef<Texture2D>(ResourceType::Texture2D, width_render, height_render, 1, 1, VkFormat::VK_FORMAT_R8G8B8A8_UNORM, flags | Texture_Uav, "gbuffer_color");
                render_target(RenderTarget::gBuffer_Normal) = CreateRef<Texture2D>(ResourceType::Texture2D, width_render, height_render, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, flags | Texture_Uav, "gbuffer_normal");
                render_target(RenderTarget::gBuffer_Material) = CreateRef<Texture2D>(ResourceType::Texture2D, width_render, height_render, 1, 1, VkFormat::VK_FORMAT_R8G8B8A8_UNORM, flags | Texture_Uav, "gbuffer_material");
                render_target(RenderTarget::gBuffer_Velocity) = CreateRef<Texture2D>(ResourceType::Texture2D, width_render, height_render, 1, 1, VkFormat::VK_FORMAT_R16G16_SFLOAT, flags | Texture_Uav, "gbuffer_velocity");
                render_target(RenderTarget::gBuffer_Depth) = CreateRef<Texture2D>(ResourceType::Texture2D, width_render, height_render, 1, 1, VkFormat::VK_FORMAT_D32_SFLOAT, flags, "gbuffer_depth");
            }

            // Light
            {
                uint32_t flags = Texture_Uav | Texture_Srv | ClearBlit;
                render_target(RenderTarget::LightDiffuse)     = CreateRef<Texture2D>(ResourceType::Texture2D, width_render, height_render, 1, 1, VkFormat::VK_FORMAT_B10G11R11_UFLOAT_PACK32, flags, "light_diffuse");
                render_target(RenderTarget::LightSpecular)    = CreateRef<Texture2D>(ResourceType::Texture2D, width_render, height_render, 1, 1, VkFormat::VK_FORMAT_B10G11R11_UFLOAT_PACK32, flags, "light_specular");
                render_target(RenderTarget::LightShadow)      = CreateRef<Texture2D>(ResourceType::Texture2D, width_render, height_render, 1, 1, VkFormat::VK_FORMAT_R8_UNORM, flags, "light_shadow");
                render_target(RenderTarget::LightVolumetric)  = CreateRef<Texture2D>(ResourceType::Texture2D, width_render, height_render, 1, 1, VkFormat::VK_FORMAT_B10G11R11_UFLOAT_PACK32, flags, "light_volumetric");
            }

            // Occlusion
            {
                /**
                 * @Note #1: amd is very specific with depth formats, so if something is a depth render target, it can only have one mip and flags like RHI_Texture_Uav
                 * so we create second texture with the flags we want and then blit to that, not mention that we can't even use vkBlitImage so we do a manual one (AMD is killing is us here)
                 * @Note #2: too many mips can degrade depth to nothing (0), which is infinite distance (in reverse-z), which breaks things
                 */
                render_target(RenderTarget::gBuffer_DepthOccluders) = CreateRef<Texture2D>(ResourceType::Texture2D, width_render, height_render, 1, 1, VkFormat::VK_FORMAT_D32_SFLOAT, Texture_Rtv | Texture_Srv, "depth_occluders");
                render_target(RenderTarget::gBuffer_DepthOccluders_hiz) = CreateRef<Texture2D>(ResourceType::Texture2D, width_render, height_render, 1, 5, VkFormat::VK_FORMAT_R32_SFLOAT, Texture_Uav | Texture_Srv | ClearBlit | PerMipViews, "depth_occluders_hiz");
            }

            // Misc
            render_target(RenderTarget::SSS) = CreateRef<Texture2D>(ResourceType::Texture2DArray, width_render, height_render, 4, 1, VkFormat::VK_FORMAT_R16_SFLOAT, Texture_Uav | Texture_Srv | ClearBlit, "sss");
            render_target(RenderTarget::SSR) = CreateRef<Texture2D>(ResourceType::Texture2D, width_render, height_render, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, Texture_Uav | Texture_Srv | ClearBlit, "ssr");
            render_target(RenderTarget::SSAO) = CreateRef<Texture2D>(ResourceType::Texture2D, width_render, height_render, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, Texture_Uav | Texture_Srv | ClearBlit, "ssao");
            if (VulkanDevice::PropertyIsShadingRateSupported())
            { 
                render_target(RenderTarget::ShadingRate) = CreateRef<Texture2D>(ResourceType::Texture2D, width_render / 4, height_render / 4, 1, 1, VkFormat::VK_FORMAT_R8_UINT, Texture_Srv | Texture_Uav | Texture_Rtv | Texture_Vrs, "shading_rate");
            }

            render_target(RenderTarget::ShadowAtlas) = CreateRef<Texture2D>(ResourceType::Texture2D, 8192, 8192, 1, 1, VkFormat::VK_FORMAT_D32_SFLOAT, Texture_Rtv | Texture_Srv | ClearBlit, "shadow_atlas");
        }

        // Resolution - output
        if (create_output)
        {
            // frame
            render_target(RenderTarget::FrameOutput) = CreateRef<Texture2D>(ResourceType::Texture2D, width_output, height_output, 1, compute_mip_count(16), VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, Texture_Uav | Texture_Srv | Texture_Rtv | ClearBlit | PerMipViews, "frame_output");
            render_target(RenderTarget::FrameOutput2) = CreateRef<Texture2D>(ResourceType::Texture2D, width_output, height_output, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, Texture_Uav | Texture_Srv | Texture_Rtv | ClearBlit, "frame_output_2");

            // misc
            render_target(RenderTarget::Bloom) = CreateRef<Texture2D>(ResourceType::Texture2D, width_output, height_output, 1, compute_mip_count(64), VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, Texture_Uav | Texture_Srv | PerMipViews, "bloom");
            render_target(RenderTarget::Outline) = CreateRef<Texture2D>(ResourceType::Texture2D, width_output, height_output, 1, 1, VkFormat::VK_FORMAT_R8G8B8A8_UNORM, Texture_Uav | Texture_Srv | Texture_Rtv, "outline");
            render_target(RenderTarget::gBuffer_DepthOpaque_Output) = CreateRef<Texture2D>(ResourceType::Texture2D, width_output, height_output, 1, 1, VkFormat::VK_FORMAT_D32_SFLOAT, Texture_Srv | Texture_Rtv | ClearBlit, "depth_opaque_output");
        }

        // Fixed Resolution - (created once)
        if (!render_target(RenderTarget::LUT_Specular_BRDF))
        {
            // Look-Up Tables
            render_target(RenderTarget::LUT_Specular_BRDF) = CreateRef<Texture2D>(ResourceType::Texture2D, 512, 512, 1,  1, VkFormat::VK_FORMAT_R8G8_UNORM, Texture_Uav | Texture_Srv, "lut_brdf_specular");
            render_target(RenderTarget::LUT_Atmosphere_Scatter) = CreateRef<Texture2D>(ResourceType::TextureCube, 256, 256, 32, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, Texture_Uav |  Texture_Srv, "lut_atmosphere_scatter");

            // Misc
            render_target(RenderTarget::Blur) = CreateRef<Texture2D>(ResourceType::Texture2D, 4096, 4096, 1, 1, VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT, Texture_Uav | Texture_Srv, "blur_scratch");
            uint32_t lowest_dimension = 32; // lowest mip is 32x32, preserving directional detail for diffuse IBL (1x1 loses directionality)
            render_target(RenderTarget::SkySphere) = CreateRef<Texture2D>(ResourceType::Texture2D, 4096, 2048, 1, compute_mip_count(lowest_dimension), VkFormat::VK_FORMAT_B10G11R11_UFLOAT_PACK32, Texture_Uav | Texture_Srv | PerMipViews | ClearBlit, "skysphere");
        }

        RHI_VendorTechnology::Resize(GetResolutionRender(), GetResolutionOutput());
    }

    void Renderer::CreateShaders()
    {
        const bool async        = true;
        const std::string shader_dir = ResourceCache::GetResourceDirectory(ResourceDirectory::Shaders) + "\\";
        #define shader(x) shaders[static_cast<uint8_t>(x)]

        // Debug
        {
            // Line
            shader(ShaderType::line_v) = CreateRef<Shader>();
            shader(ShaderType::line_v)->Compile(ShaderStage::Stage::Vertex, shader_dir + "line.hlsl", async, RHI_Vertex_Type::PosCol);
            shader(ShaderType::line_p) = CreateRef<Shader>();
            shader(ShaderType::line_p)->Compile(ShaderStage::Stage::Fragment, shader_dir + "line.hlsl", async);

            // Grid
            {
                shader(ShaderType::grid_v) = CreateRef<Shader>();
                shader(ShaderType::grid_v)->Compile(ShaderStage::Stage::Vertex, shader_dir + "grid.hlsl", async, RHI_Vertex_Type::PosUvNorTan);

                shader(ShaderType::grid_p) = CreateRef<Shader>();
                shader(ShaderType::grid_p)->Compile(ShaderStage::Stage::Fragment, shader_dir + "grid.hlsl", async);
            }

            // Outline
            {
                shader(ShaderType::outline_v) = CreateRef<Shader>();
                shader(ShaderType::outline_v)->Compile(ShaderStage::Stage::Vertex, shader_dir + "outline.hlsl", async, RHI_Vertex_Type::PosUvNorTan);

                shader(ShaderType::outline_p) = CreateRef<Shader>();
                shader(ShaderType::outline_p)->Compile(ShaderStage::Stage::Fragment, shader_dir + "outline.hlsl", async);

                shader(ShaderType::outline_c) = CreateRef<Shader>();
                shader(ShaderType::outline_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "outline.hlsl", async);
            }
        }

        // Depth
        {
            shader(ShaderType::depth_prepass_v) = CreateRef<Shader>();
            shader(ShaderType::depth_prepass_v)->Compile(ShaderStage::Stage::Vertex, shader_dir + "depth_prepass.hlsl", async, RHI_Vertex_Type::PosUvNorTan);

            shader(ShaderType::depth_prepass_alpha_test_p) = CreateRef<Shader>();
            shader(ShaderType::depth_prepass_alpha_test_p)->Compile(ShaderStage::Stage::Fragment, shader_dir + "depth_prepass.hlsl", async);
        }

        // Light Depth
        {
            shader(ShaderType::depth_light_v) = CreateRef<Shader>();
            shader(ShaderType::depth_light_v)->Compile(ShaderStage::Stage::Vertex, shader_dir + "depth_light.hlsl", async, RHI_Vertex_Type::PosUvNorTan);

            shader(ShaderType::depth_light_alpha_color_p) = CreateRef<Shader>();
            shader(ShaderType::depth_light_alpha_color_p)->Compile(ShaderStage::Stage::Fragment, shader_dir + "depth_light.hlsl", async);
        }

        // G-Buffer
        {
            shader(ShaderType::gbuffer_v) = CreateRef<Shader>();
            shader(ShaderType::gbuffer_v)->Compile(ShaderStage::Stage::Vertex, shader_dir + "g_buffer.hlsl", async, RHI_Vertex_Type::PosUvNorTan);

            shader(ShaderType::gbuffer_p) = CreateRef<Shader>();
            shader(ShaderType::gbuffer_p)->Compile(ShaderStage::Stage::Fragment, shader_dir + "g_buffer.hlsl", async);
        }

        // Tessellation
        {
            shader(ShaderType::tessellation_h) = CreateRef<Shader>();
            shader(ShaderType::tessellation_h)->Compile(ShaderStage::Stage::TesselationControl, shader_dir + "common_vertex_processing.hlsl", async);

            shader(ShaderType::tessellation_d) = CreateRef<Shader>();
            shader(ShaderType::tessellation_d)->Compile(ShaderStage::Stage::TesselationEval, shader_dir + "common_vertex_processing.hlsl", async);
        }

        // Light
        {
            // BRDF Specular LUT - Compile synchronously as it's needed immediately
            shader(ShaderType::light_integration_brdf_specular_lut_c) = CreateRef<Shader>();
            shader(ShaderType::light_integration_brdf_specular_lut_c)->AddDefine("BRDF_SPECULAR_LUT");
            shader(ShaderType::light_integration_brdf_specular_lut_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "light_integration.hlsl", false);

            // Environment PreFilter - Compile synchronously as it's needed immediately
            shader(ShaderType::light_integration_environment_filter_c) = CreateRef<Shader>();
            shader(ShaderType::light_integration_environment_filter_c)->AddDefine("ENVIRONMENT_FILTER");
            shader(ShaderType::light_integration_environment_filter_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "light_integration.hlsl", async);

            // Light
            shader(ShaderType::light_c) = CreateRef<Shader>();
            shader(ShaderType::light_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "light.hlsl", async);

            // Composition
            shader(ShaderType::light_composition_c) = CreateRef<Shader>();
            shader(ShaderType::light_composition_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "light_composition.hlsl", async);

            // Image Based
            shader(ShaderType::light_image_based_c) = CreateRef<Shader>();
            shader(ShaderType::light_image_based_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "light_image_based.hlsl", async);
        }

        // Blur
        {
            // Gaussian
            shader(ShaderType::blur_gaussian_c) = CreateRef<Shader>();
            shader(ShaderType::blur_gaussian_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "blur.hlsl", async);

            // Gaussian Bilateral - or Depth Aware
            shader(ShaderType::blur_gaussian_bilaterial_c) = CreateRef<Shader>();
            shader(ShaderType::blur_gaussian_bilaterial_c)->AddDefine("PASS_BLUR_GAUSSIAN_BILATERAL");
            shader(ShaderType::blur_gaussian_bilaterial_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "blur.hlsl", async);
        }

        // Bloom
        {
            // Downsample Luminance
            shader(ShaderType::bloom_luminance_c) = CreateRef<Shader>();
            shader(ShaderType::bloom_luminance_c)->AddDefine("LUMINANCE");
            shader(ShaderType::bloom_luminance_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "bloom.hlsl", async);

            // Upsample blend (with previous MIP)
            shader(ShaderType::bloom_upsample_blend_mip_c) = CreateRef<Shader>();
            shader(ShaderType::bloom_upsample_blend_mip_c)->AddDefine("UPSAMPLE_BLEND_MIP");
            shader(ShaderType::bloom_upsample_blend_mip_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "bloom.hlsl", async);

            // Upsample blend (with frame)
            shader(ShaderType::bloom_blend_frame_c) = CreateRef<Shader>();
            shader(ShaderType::bloom_blend_frame_c)->AddDefine("BLEND_FRAME");
            shader(ShaderType::bloom_blend_frame_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "bloom.hlsl", async);
        }

        // AMD FIDELITY-FX
        {
            // CAS - Contrast Adaptive Sharpening
            shader(ShaderType::ffx_cas_c) = CreateRef<Shader>();
            shader(ShaderType::ffx_cas_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "amd_fidelity_fx\\cas.hlsl", async);

            // SPD - Single Pass Downsample - Compile synchronously as they are needed everywhere
            {
                shader(ShaderType::ffx_spd_average_c) = CreateRef<Shader>();
                shader(ShaderType::ffx_spd_average_c)->AddDefine("AVERAGE");
                shader(ShaderType::ffx_spd_average_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "amd_fidelity_fx\\spd.hlsl", false);

                shader(ShaderType::ffx_spd_min_c) = CreateRef<Shader>();
                shader(ShaderType::ffx_spd_min_c)->AddDefine("MIN");
                shader(ShaderType::ffx_spd_min_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "amd_fidelity_fx\\spd.hlsl", false);

                shader(ShaderType::ffx_spd_max_c) = CreateRef<Shader>();
                shader(ShaderType::ffx_spd_max_c)->AddDefine("MAX");
                shader(ShaderType::ffx_spd_max_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "amd_fidelity_fx\\spd.hlsl", false);

                shader(ShaderType::ffx_spd_luminance_c) = CreateRef<Shader>();
                shader(ShaderType::ffx_spd_luminance_c)->AddDefine("LUMINANCE");
                shader(ShaderType::ffx_spd_luminance_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "amd_fidelity_fx\\spd.hlsl", false);
            }
        }

        // Sky
        {
            shader(ShaderType::skysphere_c) = CreateRef<Shader>();
            shader(ShaderType::skysphere_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "skysphere.hlsl", async);

            shader(ShaderType::skysphere_lut_c) = CreateRef<Shader>();
            shader(ShaderType::skysphere_lut_c)->AddDefine("LUT");
            shader(ShaderType::skysphere_lut_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "skysphere.hlsl", async);
        }

        // FXAA
        shader(ShaderType::fxaa_c) = CreateRef<Shader>();
        shader(ShaderType::fxaa_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "fxaa\\fxaa.hlsl", async);

        // Font
        shader(ShaderType::font_v) = CreateRef<Shader>();
        shader(ShaderType::font_v)->Compile(ShaderStage::Stage::Vertex, shader_dir + "font.hlsl", async, RHI_Vertex_Type::PosUv);
        shader(ShaderType::font_p) = CreateRef<Shader>();
        shader(ShaderType::font_p)->Compile(ShaderStage::Stage::Fragment, shader_dir + "font.hlsl", async);

        // Film Grain
        shader(ShaderType::film_grain_c) = CreateRef<Shader>();
        shader(ShaderType::film_grain_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "film_grain.hlsl", async);

        // Chromatic Aberration
        shader(ShaderType::chromatic_aberration_c) = CreateRef<Shader>();
        shader(ShaderType::chromatic_aberration_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "chromatic_aberration.hlsl", async);

        // VHS
        shader(ShaderType::vhs_c) = CreateRef<Shader>();
        shader(ShaderType::vhs_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "vhs.hlsl", async);

        // Tone-mapping & Gamma Correction
        shader(ShaderType::output_c) = CreateRef<Shader>();
        shader(ShaderType::output_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "output.hlsl", async);

        // Motion Blur
        shader(ShaderType::motion_blur_c) = CreateRef<Shader>();
        shader(ShaderType::motion_blur_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "motion_blur.hlsl", async);

        // Screen Space Global Illumination
        shader(ShaderType::ssao_c) = CreateRef<Shader>();
        shader(ShaderType::ssao_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "ssao.hlsl", async);

        // Screen Space Shadows
        shader(ShaderType::sss_c_bend) = CreateRef<Shader>();
        shader(ShaderType::sss_c_bend)->Compile(ShaderStage::Stage::Compute, shader_dir + "screen_space_shadows\\bend_sss.hlsl", async);

        // Depth of Field
        shader(ShaderType::depth_of_field_c) = CreateRef<Shader>();
        shader(ShaderType::depth_of_field_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "depth_of_field.hlsl", async);

        // Variable Rate Shading
        shader(ShaderType::variable_rate_shading_c) = CreateRef<Shader>();
        shader(ShaderType::variable_rate_shading_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "variable_rate_shading.hlsl", async);

        // BLIT
        shader(ShaderType::blit_c) = CreateRef<Shader>();
        shader(ShaderType::blit_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "blit.hlsl", async);

        // Occlusion
        shader(ShaderType::occlusion_c) = CreateRef<Shader>();
        shader(ShaderType::occlusion_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "occlusion.hlsl", async);

        // Icons
        shader(ShaderType::icon_c) = CreateRef<Shader>();
        shader(ShaderType::icon_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "icon.hlsl", async);

        // Dithering
        shader(ShaderType::dithering_c) = CreateRef<Shader>();
        shader(ShaderType::dithering_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "dithering.hlsl", async);

        // Dithering
        shader(ShaderType::transparency_reflection_refraction_c) = CreateRef<Shader>();
        shader(ShaderType::transparency_reflection_refraction_c)->Compile(ShaderStage::Stage::Compute, shader_dir + "transparency_reflection_refraction.hlsl", async);
    }

    void Renderer::CreateFonts()
    {
        // get standard font directory
        const std::string dir_font = ResourceCache::GetResourceDirectory(ResourceDirectory::Fonts) + "\\";

        // load a font
        uint32_t size = static_cast<uint32_t>(10 * Window::GetDpiScale());
        standard_font = CreateRef<Font>(dir_font + "OpenSans/OpenSans-Medium.ttf", size, Color(0.9f, 0.9f, 0.9f, 1.0f));
    }

    void Renderer::CreateStandardMeshes()
    {
        auto create_mesh = [](const MeshType type)
        {
            const std::string project_directory = ResourceCache::GetProjectDirectory();
            std::shared_ptr<Mesh> mesh = CreateRef<Mesh>();
            std::vector<Vertex_PosTexNorTan> vertices;
            std::vector<uint32_t> indices;

            if (type == MeshType::Cube)
            {
                geometry_generation::generate_cube(&vertices, &indices);
                mesh->SetResourceFilePath(project_directory + "standard_cube" + EXTENSION_MESH);
            }
            else if (type == MeshType::Quad)
            {
                geometry_generation::generate_quad(&vertices, &indices);
                mesh->SetResourceFilePath(project_directory + "standard_quad" + EXTENSION_MESH);
            }
            else if (type == MeshType::Sphere)
            {
                geometry_generation::generate_sphere(&vertices, &indices);
                mesh->SetResourceFilePath(project_directory + "standard_sphere" + EXTENSION_MESH);
            }
            else if (type == MeshType::Cylinder)
            {
                geometry_generation::generate_cylinder(&vertices, &indices);
                mesh->SetResourceFilePath(project_directory + "standard_cylinder" + EXTENSION_MESH);
            }
            else if (type == MeshType::Cone)
            {
                geometry_generation::generate_cone(&vertices, &indices);
                mesh->SetResourceFilePath(project_directory + "standard_cone" + EXTENSION_MESH);
            }

            // don't optimize this geometry as it's made to spec
            mesh->SetFlag(static_cast<uint32_t>(MeshFlags::PostProcessOptimize), false);

            mesh->AddGeometry(vertices, indices, false);
            mesh->SetType(type);
            mesh->CreateGpuBuffers();

            standard_meshes[static_cast<uint8_t>(type)] = mesh;
        };

        create_mesh(MeshType::Cube);
        create_mesh(MeshType::Quad);
        create_mesh(MeshType::Sphere);
        create_mesh(MeshType::Cylinder);
        create_mesh(MeshType::Cone);

        // this buffers holds all debug primitives that can be drawn
        m_lines_vertex_buffer = CreateRef<Buffer>();
    }

    void Renderer::CreateStandardTextures()
    {
        const std::string dir_texture   = ResourceCache::GetResourceDirectory(ResourceDirectory::Textures) + "\\";
        const std::string dir_materials = "project\\materials\\";

        #define standard_texture(x) standard_textures[static_cast<uint32_t>(x)]

        // blue noise textures with 2 channels, each channel containing a different pattern
        {
            standard_texture(StandardTexture::Noise_blue_0) = CreateRef<Texture2D>(dir_texture + "noise_blue_0.png");
            standard_texture(StandardTexture::Noise_blue_1) = CreateRef<Texture2D>(dir_texture + "noise_blue_1.png");
            standard_texture(StandardTexture::Noise_blue_2) = CreateRef<Texture2D>(dir_texture + "noise_blue_2.png");
            standard_texture(StandardTexture::Noise_blue_3) = CreateRef<Texture2D>(dir_texture + "noise_blue_3.png");
            standard_texture(StandardTexture::Noise_blue_4) = CreateRef<Texture2D>(dir_texture + "noise_blue_4.png");
            standard_texture(StandardTexture::Noise_blue_5) = CreateRef<Texture2D>(dir_texture + "noise_blue_5.png");
            standard_texture(StandardTexture::Noise_blue_6) = CreateRef<Texture2D>(dir_texture + "noise_blue_6.png");
            standard_texture(StandardTexture::Noise_blue_7) = CreateRef<Texture2D>(dir_texture + "noise_blue_7.png");
        }

        // gizmos
        {
            standard_texture(StandardTexture::Gizmo_light_directional) = CreateRef<Texture2D>(dir_texture + "sun.png");
            standard_texture(StandardTexture::Gizmo_light_point)       = CreateRef<Texture2D>(dir_texture + "light_bulb.png");
            standard_texture(StandardTexture::Gizmo_light_spot)        = CreateRef<Texture2D>(dir_texture + "flashlight.png");
            //standard_texture(StandardTexture::Gizmo_audio_source)    = CreateRef<Texture2D>(dir_texture + "audio.png");
        }

        // misc
        {
            standard_texture(StandardTexture::Checkerboard) = CreateRef<Texture2D>(dir_texture + "no_texture.png");
        }

        // black and white
        {
            // black texture (1x1 pixel, rgba = 0,0,0,255) // single pixel: r=0, g=0, b=0, a=255
            std::vector<TextureMip> black_mips = { TextureMip{std::vector<std::byte>{std::byte{0}, std::byte{0}, std::byte{0}, std::byte{255}}} };
            std::vector<TextureSlice> black_data = { TextureSlice{black_mips} };
            standard_texture(StandardTexture::Black) = CreateRef<Texture2D>(
				TextureType::Texture2D,             // type
                1,									// width
                1,									// height
                1,									// depth
                1,									// mip_count
                VkFormat::VK_FORMAT_R8G8B8A8_UNORM,	// format
                Texture_Srv | Texture_Uav,			// flags
                "black_texture",					// name
                black_data						// data
            );
        
            // white texture (1x1 pixel, rgba = 255,255,255,255)  // single pixel: r=255, g=255, b=255, a=255
            std::vector<TextureMip> white_mips = { TextureMip{std::vector<std::byte>{std::byte{255}, std::byte{255}, std::byte{255}, std::byte{255}}} };
            std::vector<TextureSlice> white_data = { TextureSlice{white_mips} };
            standard_texture(StandardTexture::White) = CreateRef<Texture2D>(
				TextureType::Texture2D,				// type
                1,									// width
                1,									// height
                1,									// depth
                1,									// mip_count
                VkFormat::VK_FORMAT_R8G8B8A8_UNORM, // format
                Texture_Srv | Texture_Uav,			// flags
                "white_texture",					// name
                white_data						// data
            );
        }
    }

    void Renderer::CreateStandardMaterials()
    {
        const std::string data_dir = ResourceCache::GetDataDirectory() + "\\";
        FileSystem::CreateDirectory_(data_dir);

        standard_material = CreateRef<Material>();
        standard_material->SetResourceFilePath(ResourceCache::GetProjectDirectory() + "standard" + EXTENSION_MATERIAL); // set resource file path so it can be used by the resource cache
        standard_material->SetProperty(MaterialProperty::TextureTilingX, 1.0f);
        standard_material->SetProperty(MaterialProperty::TextureTilingY, 1.0f);
        standard_material->SetProperty(MaterialProperty::ColorR,         1.0f);
        standard_material->SetProperty(MaterialProperty::ColorG,         1.0f);
        standard_material->SetProperty(MaterialProperty::ColorB,         1.0f);
        standard_material->SetProperty(MaterialProperty::ColorA,         1.0f);
        standard_material->SetProperty(MaterialProperty::WorldSpaceUv,   1.0f);
        standard_material->SetTexture(MaterialTextureType::Color,        Renderer::GetStandardTexture(StandardTexture::Checkerboard));
    }

    void Renderer::DestroyResources()
    {
        render_targets.fill(nullptr);
        shaders.fill(nullptr);
        samplers.fill(nullptr);
        standard_textures.fill(nullptr);
        standard_meshes.fill(nullptr);
        buffers.fill(nullptr);
        standard_font     = nullptr;
        standard_material = nullptr;
    }

    std::array<Ref<Texture2D>, static_cast<uint32_t>(RenderTarget::MaxEnum)> &Renderer::GetRenderTargets() { return render_targets; }
    std::array<Ref<Shader>, static_cast<uint32_t>(ShaderType::MaxEnum)> &Renderer::GetShaders() { return shaders; }
    std::array<Ref<Buffer>, static_cast<uint32_t>(RendererBufferId::MaxEnum)> &Renderer::GetStructuredBuffers() { return buffers; }
    std::array<Ref<Sampler>, static_cast<uint32_t>(SamplerPreset::MaxEnum)> &Renderer::GetSamplers() { return samplers; }

    Rasterizer* Renderer::GetRasterizerState(const RasterizerState type) { return rasterizer_states[static_cast<uint8_t>(type)].Get(); }

    DepthStencilState* Renderer::GetDepthStencilState(const Renderer_DepthStencilState type) { return depth_stencil_states[static_cast<uint8_t>(type)].Get(); }

    BlendState* Renderer::GetBlendState(const BlendMode type) { return blend_states[static_cast<uint8_t>(type)].Get(); }

    Texture* Renderer::GetRenderTarget(const RenderTarget type) { return render_targets[static_cast<uint8_t>(type)].Get(); }

    Shader* Renderer::GetShader(const ShaderType type) { return shaders[static_cast<uint8_t>(type)].Get(); }

    Buffer* Renderer::GetBuffer(const RendererBufferId type) { return buffers[static_cast<uint8_t>(type)].Get(); }

    void Renderer::SwapVisibilityBuffers() {std::ranges::swap(buffers[static_cast<uint8_t>(RendererBufferId::Visibility)], buffers[static_cast<uint8_t>(RendererBufferId::VisibilityPrevious)]); }

    Texture2D* Renderer::GetStandardTexture(const StandardTexture type) { return standard_textures[static_cast<uint8_t>(type)].Get(); }

    Ref<Mesh>& Renderer::GetStandardMesh(const MeshType type) { return standard_meshes[static_cast<uint8_t>(type)]; }

    Ref<Font>& Renderer::GetFont() { return standard_font; }

    Ref<Material>& Renderer::GetStandardMaterial() { return standard_material; }

}

/// -------------------------------------------------------
