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
 * renderer_declarations.h
 * -------------------------------------------------------
 * Created: 04/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <array>
#include <map>
#include <SceneryEditorX/scene/lights.h>
#include <xMath/includes/xmath.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
    // -------------------------------------------------------
    // PolygonMode: rasterisation fill mode
    // -------------------------------------------------------

    /**
     * @enum PolygonMode
     * @brief Vulkan polygon fill mode, exposed on RasterizerState objects.
     */
    enum class PolygonMode : uint8_t
    {
        Solid,
        Wireframe,
        Point,
        MaxEnum
    };

    // -------------------------------------------------------
    // Forward declarations for state object classes
    // -------------------------------------------------------
    class RasterizerState;
    class BlendState;
    class DepthStencilState;
    class ImageResource;
    class Shader;
    class Buffer;

    // -------------------------------------------------------
    // RasterizerState – configures polygon fill, culling, depth bias
    // -------------------------------------------------------

    /**
     * @class RasterizerState
     * @brief Immutable rasterizer configuration object returned by Renderer::GetRasterizerState().
     */
    class RasterizerState
    {
    public:
        explicit RasterizerState(PolygonMode polygonMode, bool depthBiasEnabled = false,
                                 float depthBiasConstant = 0.0f, float depthBiasSlope = 0.0f)
            : m_PolygonMode(polygonMode)
            , m_DepthBiasEnabled(depthBiasEnabled)
            , m_DepthBiasConstant(depthBiasConstant)
            , m_DepthBiasSlope(depthBiasSlope)
        {}

        PolygonMode GetPolygonMode()       const { return m_PolygonMode; }
        bool        IsDepthBiasEnabled()   const { return m_DepthBiasEnabled; }
        float       GetDepthBiasConstant() const { return m_DepthBiasConstant; }
        float       GetDepthBiasSlope()    const { return m_DepthBiasSlope; }

    private:
        PolygonMode m_PolygonMode       = PolygonMode::Solid;
        bool        m_DepthBiasEnabled  = false;
        float       m_DepthBiasConstant = 0.0f;
        float       m_DepthBiasSlope    = 0.0f;
    };

    // -------------------------------------------------------
    // BlendState – configures colour and alpha blending
    // -------------------------------------------------------

    /**
     * @class BlendState
     * @brief Immutable blend configuration object returned by Renderer::GetBlendState().
     */
    class BlendState
    {
    public:
        explicit BlendState(bool blendEnabled, VkBlendFactor srcColor = VK_BLEND_FACTOR_ONE,
                            VkBlendFactor dstColor = VK_BLEND_FACTOR_ZERO,
                            VkBlendOp     colorOp  = VK_BLEND_OP_ADD,
                            VkBlendFactor srcAlpha = VK_BLEND_FACTOR_ONE,
                            VkBlendFactor dstAlpha = VK_BLEND_FACTOR_ZERO,
                            VkBlendOp     alphaOp  = VK_BLEND_OP_ADD)
            : m_BlendEnabled(blendEnabled)
            , m_SrcColor(srcColor), m_DstColor(dstColor), m_ColorOp(colorOp)
            , m_SrcAlpha(srcAlpha), m_DstAlpha(dstAlpha), m_AlphaOp(alphaOp)
        {}

        bool          IsBlendEnabled() const { return m_BlendEnabled; }
        VkBlendFactor GetSrcColor()    const { return m_SrcColor; }
        VkBlendFactor GetDstColor()    const { return m_DstColor; }
        VkBlendOp     GetColorOp()     const { return m_ColorOp;  }
        VkBlendFactor GetSrcAlpha()    const { return m_SrcAlpha; }
        VkBlendFactor GetDstAlpha()    const { return m_DstAlpha; }
        VkBlendOp     GetAlphaOp()     const { return m_AlphaOp;  }

    private:
        bool          m_BlendEnabled = false;
        VkBlendFactor m_SrcColor     = VK_BLEND_FACTOR_ONE;
        VkBlendFactor m_DstColor     = VK_BLEND_FACTOR_ZERO;
        VkBlendOp     m_ColorOp      = VK_BLEND_OP_ADD;
        VkBlendFactor m_SrcAlpha     = VK_BLEND_FACTOR_ONE;
        VkBlendFactor m_DstAlpha     = VK_BLEND_FACTOR_ZERO;
        VkBlendOp     m_AlphaOp      = VK_BLEND_OP_ADD;
    };

    // -------------------------------------------------------
    // DepthStencilState – configures depth/stencil testing and writes
    // -------------------------------------------------------

    /**
     * @class DepthStencilState
     * @brief Immutable depth/stencil configuration returned by Renderer::GetDepthStencilState().
     */
    class DepthStencilState
    {
    public:
        explicit DepthStencilState(bool depthTestEnable, bool depthWriteEnable,
                                   VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS)
            : m_DepthTestEnable(depthTestEnable)
            , m_DepthWriteEnable(depthWriteEnable)
            , m_DepthCompareOp(depthCompareOp)
        {}

        bool          IsDepthTestEnabled()  const { return m_DepthTestEnable;  }
        bool          IsDepthWriteEnabled() const { return m_DepthWriteEnable; }
        VkCompareOp   GetDepthCompareOp()   const { return m_DepthCompareOp;   }

    private:
        bool        m_DepthTestEnable  = true;
        bool        m_DepthWriteEnable = true;
        VkCompareOp m_DepthCompareOp   = VK_COMPARE_OP_LESS;
    };


    /**
     * @enum Renderer_RenderTarget
     * @brief Enumeration of renderer render targets.
     *
     * This enum provides a type-safe way to identify and reference
     * the different render targets used in the rendering system.
     *
     * @note - The values in this enum are designed to be compatible with Vulkan's render target types.
     */
    enum class Renderer_RenderTarget : uint8_t
    {
        gbuffer_color,
        gbuffer_normal,
        gbuffer_material,
        gbuffer_velocity,
        gbuffer_depth,
        gbuffer_depth_occluders,
        gbuffer_depth_occluders_hiz,
        gbuffer_depth_opaque_output,
        lut_brdf_specular,
        lut_atmosphere_scatter,
        lut_atmosphere_transmittance,
        lut_atmosphere_multiscatter,
        light_diffuse,
        light_specular,
        light_volumetric,
        frame_render,
        frame_render_opaque,
        frame_output,
        frame_output_2,
        ssao,
        reflections,
        gbuffer_reflections_position,
        gbuffer_reflections_normal,
        gbuffer_reflections_albedo,
        sss,
        skysphere,
        bloom,
        blur,
        outline,
        shading_rate,
        shadow_atlas,
        auto_exposure,
        auto_exposure_previous,
        // restir reservoir buffers (current frame)
        restir_reservoir0,
        restir_reservoir1,
        restir_reservoir2,
        restir_reservoir3,
        restir_reservoir4,
        // restir reservoir buffers (previous frame for temporal)
        restir_reservoir_prev0,
        restir_reservoir_prev1,
        restir_reservoir_prev2,
        restir_reservoir_prev3,
        restir_reservoir_prev4,
        // restir reservoir buffers (spatial ping-pong)
        restir_reservoir_spatial0,
        restir_reservoir_spatial1,
        restir_reservoir_spatial2,
        restir_reservoir_spatial3,
        restir_reservoir_spatial4,
        // volumetric clouds
        cloud_noise_shape,
        cloud_noise_detail,
        cloud_shadow,
        // nrd denoiser textures
        nrd_viewz,
        nrd_normal_roughness,
        nrd_diff_radiance_hitdist,
        nrd_spec_radiance_hitdist,
        nrd_out_diff_radiance_hitdist,
        nrd_out_spec_radiance_hitdist,
        // debug
        debug_output,
        MaxEnum
    };


    enum class Renderer_Shader : uint8_t
    {
        tessellation_h,
        tessellation_d,
        gbuffer_v,
        gbuffer_p,
        depth_prepass_v,
        depth_prepass_alpha_test_p,
        depth_light_v,
        depth_light_alpha_color_p,
        fxaa_c,
        film_grain_c,
        motion_blur_c,
        depth_of_field_c,
        chromatic_aberration_c,
        vhs_c,
        bloom_luminance_c,
        bloom_blend_frame_c,
        bloom_upsample_blend_mip_c,
        bloom_downsample_c,
        output_c,
        light_integration_brdf_specular_lut_c,
        light_integration_environment_filter_c,
        light_c,
        light_composition_c,
        light_image_based_c,
        line_v,
        line_p,
        grid_v,
        grid_p,
        outline_v,
        outline_p,
        outline_c,
        font_v,
        font_p,
        ssao_c,
        sss_c_bend,
        skysphere_c,
        skysphere_lut_c,
        skysphere_transmittance_lut_c,
        skysphere_multiscatter_lut_c,
        blur_gaussian_c,
        blur_gaussian_bilaterial_c,
        variable_rate_shading_c,
        ffx_cas_c,
        ffx_spd_average_c,
        ffx_spd_min_c,
        ffx_spd_max_c,
        blit_c,
        icon_c,
        dithering_c,
        transparency_reflection_refraction_c,
        auto_exposure_c,
        // restir path tracing gi
        restir_pt_ray_generation_r,
        restir_pt_ray_miss_r,
        restir_pt_ray_hit_r,
        restir_pt_temporal_c,
        restir_pt_spatial_c,
        // volumetric clouds
        cloud_noise_shape_c,
        cloud_noise_detail_c,
        cloud_shadow_c,
        light_reflections_c,
        // nrd denoiser
        nrd_prepare_c,
        // gpu-driven indirect rendering
        indirect_cull_c,
        gbuffer_indirect_v,
        gbuffer_indirect_p,
        depth_prepass_indirect_v,
        // gpu-driven particles
        particle_emit_c,
        particle_simulate_c,
        particle_render_c,
        // gpu texture compression
        texture_compress_bc1_c,
        texture_compress_bc3_c,
        texture_compress_bc5_c,
        MaxEnum
    };
    

    enum class Renderer_Sampler : uint8_t
    {
        Compare_depth,
        Point_clamp_edge,
        Point_clamp_border,
        Point_wrap,
        Bilinear_clamp_edge,
        Bilinear_clamp_border,
        Bilinear_wrap,
        Trilinear_clamp,
        Anisotropic_wrap,
        MaxEnum
    };

    enum class Renderer_Buffer : uint8_t
    {
        ConstantFrame,
        SpdCounter,
        MaterialParameters,
        LightParameters,
        DummyInstance,
        AABBs,
        GeometryInfo,
        IndirectDrawArgs,
        IndirectDrawData,
        IndirectDrawDataOut,
        IndirectDrawArgsOut,
        IndirectDrawCount,
        DrawData, // bindless per-draw data (transforms, material index, etc.)
        // gpu-driven particles
        ParticleBufferA,
        ParticleBufferB,
        ParticleCounter,
        ParticleEmitter,
        MaxEnum
    };

    enum class StandardTexture : uint8_t
    {
        Noise_perlin,
        Noise_blue, // single blue noise texture (was 8, only 1 used)
        Checkerboard,
        Gizmo_light_directional,
        Gizmo_light_point,
        Gizmo_light_spot,
        Gizmo_audio_source,
        Black,
        White,
        MaxEnum
    };

    /**
     * @enum Renderer_RasterizerState
     * @brief Enumerates the rasterizer pipeline state presets available to the renderer.
     */
    enum class Renderer_RasterizerState : uint8_t
    {
        Solid,
        Wireframe,
        Light_point_spot,
        Light_directional,
        MaxEnum
    };

    /**
     * @enum Renderer_DepthStencilState
     * @brief Enumerates depth/stencil test and write configuration presets.
     */
    enum class Renderer_DepthStencilState : uint8_t
    {
        Off,
        ReadEqual,
        ReadGreaterEqual,
        ReadWrite,
        MaxEnum
    };

    enum class Renderer_BindingsSrv
    {
        // g-buffer
        gbuffer_albedo   = 0,
        gbuffer_normal   = 1,
        gbuffer_material = 2,
        gbuffer_velocity = 3,
        gbuffer_depth    = 4,

        // other
        ssao = 5,
    
        // misc
        tex   = 6,
        tex2  = 7,
        tex3  = 8,
        tex4  = 9,
        tex5  = 10,
        tex6  = 11,
        tex3d = 12,

        // noise
        tex_perlin = 13,
        // bindless
        bindless_material_textures   = 14,
        bindless_material_parameters = 15,
        bindless_light_parameters    = 16,
        bindless_aabbs               = 17,
        bindless_draw_data           = 18,
        
        // volumetric clouds 3D noise
        tex3d_cloud_shape  = 19,
        tex3d_cloud_detail = 20,
        // restir reservoir srv bindings (for temporal/spatial read)
        reservoir_prev0    = 21,
        reservoir_prev1    = 22,
        reservoir_prev2    = 23,
        reservoir_prev3    = 24,
        reservoir_prev4    = 25,
    };

    enum class Renderer_BindingsUav
    {
        tex           = 0,
        tex2          = 1,
        tex3          = 2,
        tex4          = 3,
        tex3d         = 4,
        tex_sss       = 5,
        sb_spd        = 7,
        tex_spd       = 8,
        // restir reservoir uav bindings
        reservoir0    = 20,
        reservoir1    = 21,
        reservoir2    = 22,
        reservoir3    = 23,
        reservoir4    = 24,
        // nrd output bindings
        nrd_viewz              = 25,
        nrd_normal_roughness   = 26,
        nrd_diff_radiance      = 27,
        nrd_spec_radiance      = 28,
        // integer format textures (vrs, etc)
        tex_uint               = 29,
        // gpu-driven indirect drawing
        indirect_draw_args     = 30,
        indirect_draw_data     = 31,
        indirect_draw_args_out = 32,
        indirect_draw_data_out = 33,
        indirect_draw_count    = 34,
        // gpu-driven particles
        particle_buffer_a      = 35,
        particle_buffer_b      = 36,
        particle_counter       = 37,
        particle_emitter       = 38,
        // gpu texture compression
        compress_input         = 39,
        compress_output        = 40,
        compress_output_bc1    = 41,
    };

    // -------------------------------------------------------
    // Renderer_BlendState: Alpha-blend preset enumeration
    // -------------------------------------------------------

    /**
     * @enum Renderer_BlendState
     * @brief Enumerates blending mode presets used when configuring render passes.
     */
    enum class Renderer_BlendState : uint8_t
    {
        Off,
        Alpha,
        Additive,
        Premultiplied,
        MaxEnum
    };

    // -------------------------------------------------------
    // Renderer_DownsampleFilter: mip-chain downscale filter modes
    // -------------------------------------------------------

    /**
     * @enum Renderer_DownsampleFilter
     * @brief Filter kernel used by Pass_Downscale when building mip chains via SPD.
     */
    enum class Renderer_DownsampleFilter : uint8_t
    {
        Average,
        Min,
        Max,
        MaxEnum
    };

    // -------------------------------------------------------
    // BarrierType: simplified image barrier intent
    // -------------------------------------------------------

    /**
     * @enum BarrierType
     * @brief Coarse-grained barrier intent used by CommandList::InsertBarrier(ImageResource*, BarrierType).
     * The command list maps these to the correct Vulkan pipeline-stage and access-mask pairs.
     */
    enum class BarrierType : uint8_t
    {
        EnsureWriteThenRead,   ///< Make previous UAV writes visible to subsequent reads
        EnsureReadThenWrite,   ///< Ensure prior reads complete before next UAV write
        EnsureWriteThenWrite,  ///< Serialise two consecutive UAV writes
        MaxEnum
    };

    // -------------------------------------------------------
    // MeshType: standard geometry used in built-in passes
    // -------------------------------------------------------

    /**
     * @enum MeshType
     * @brief Identifies built-in primitive meshes managed by the Renderer.
     */
    enum class MeshType : uint8_t
    {
        Quad,
        Cube,
        Sphere,
        MaxEnum
    };

    // -------------------------------------------------------
    // Stage alias for shader-type shorthand
    // -------------------------------------------------------
    // Stage is defined in vulkan/enums.h; ShaderType is an alias used in pass code.
    // The unscoped alias below lets pass code write `Stage::Pixel` as a synonym for
    // `Stage::Fragment` and index PipelineState::shaders with the Stage enum directly.
    // -------------------------------------------------------

    // -------------------------------------------------------
    // Constants
    // -------------------------------------------------------

    /** Sentinel passed to pso.clear_depth to instruct the pipeline to LOAD existing depth rather than clear it. */
    inline constexpr float rhi_depth_load  = -1.0f;
    inline constexpr float depth_load      = rhi_depth_load;

    /** Maximum simultaneous colour render targets in a PipelineState. */
    inline constexpr uint32_t MAX_RENDER_TARGET_COUNT  = 8;
    inline constexpr uint32_t MAX_SHADER_STAGES        = 8;
    inline constexpr uint32_t renderer_max_draw_calls  = 4096;
    inline constexpr uint32_t MAX_MIP_COUNT            = 16;

    // -------------------------------------------------------
    // PushConstantBuffer
    // -------------------------------------------------------

    /**
     * @struct PushConstantBuffer
     * @brief Per-pass push constant data uploaded via CommandList::PushConstants().
     * All helper setters mirror the API used in renderer_passes.cpp.
     */
    struct PushConstantBuffer
    {
        uint32_t draw_index     = 0;
        uint32_t is_transparent = 0;
        uint32_t material_index = 0;
        uint32_t _pad0          = 0;

        float f3_value[3]  = {};
        float _pad1        = 0.0f;
        float f3_value2[3] = {};
        float _pad2        = 0.0f;
        float f4_value[4]  = {};
        float f2_value[2]  = {};
        float _pad3[2]     = {};

        void set_f2_value (float x, float y)                     { f2_value[0]  = x; f2_value[1]  = y; }
        void set_f3_value (float x, float y, float z)            { f3_value[0]  = x; f3_value[1]  = y; f3_value[2]  = z; }
        void set_f3_value2(float x, float y, float z)            { f3_value2[0] = x; f3_value2[1] = y; f3_value2[2] = z; }
        void set_f4_value (float x, float y, float z, float w)   { f4_value[0]  = x; f4_value[1]  = y; f4_value[2]  = z; f4_value[3] = w; }
    };

    // -------------------------------------------------------
    // PipelineState
    // -------------------------------------------------------
    // Forward declarations required for PipelineState fields
    // (full definitions live in renderer.h after all includes)
    // -------------------------------------------------------

    // Color sentinel – matches the Color type used by command lists.
    // Using a raw float[4] here avoids pulling in <colors.h> from this header.
    struct PipelineStateColor { float r = 0, g = 0, b = 0, a = 0; };

    // Opaque "load" sentinel colours (negative alpha = "load, don't clear").
    inline const PipelineStateColor rhi_color_load{ 0.0f, 0.0f, 0.0f, -1.0f };

    /**
     * @struct PipelineState
     * @brief High-level, API-agnostic descriptor for a graphics or compute pipeline.
     *
     * Passes build a PipelineState, then call CommandList::SetPipelineState() which
     * resolves or creates the underlying VkPipeline and starts the render pass.
     */
    struct PipelineState
    {
        const char*        name                   = nullptr;

        // Shader stages – indexed by Stage enum (vertex=0, geometry=1, tess_ctrl=2, tess_eval=3, fragment=4, compute=5)
        std::map<uint32_t, Shader*> shaders;

        // Pipeline state objects (nullptr = use defaults)
        RasterizerState*   rasterizer_state        = nullptr;
        BlendState*        blend_state             = nullptr;
        DepthStencilState* depth_stencil_state     = nullptr;

        // Render targets
        std::array<ImageResource*, MAX_RENDER_TARGET_COUNT> render_target_color_textures = {};
        ImageResource*  render_target_depth_texture = nullptr;
        ImageResource*  vrs_input_texture           = nullptr;

        // Clear values (rhi_color_load.a < 0 = load; non-negative = clear to this colour)
        std::array<PipelineStateColor, MAX_RENDER_TARGET_COUNT> clear_color = {};
        float           clear_depth                 = rhi_depth_load;

        // Misc flags
        bool            resolution_scale            = false;
    };

    // Forward declarations for ordering
    class Entity;

    /**
     * @class Light
     * @brief Stub light class providing the interface consumed by renderer passes.
     */
    class Light
    {
    public:
        virtual ~Light() = default;
        virtual bool         GetFlag(uint32_t /*flag*/) const         { return false;  }
        virtual float        GetIntensityWatt() const                 { return 0.0f;   }
        virtual LightType    GetLightType() const                     { return LightType::Point; }
        virtual Entity*      GetEntity() const                        { return nullptr; }
        virtual void         SetScreenSpaceShadowsSliceIndex(uint32_t /*idx*/) {}
    };

    // -------------------------------------------------------
    // MaterialProperty / MaterialTextureType stubs
    // Used by renderer passes until the full Material system is implemented.
    // -------------------------------------------------------

    /**
     * @enum MaterialProperty
     * @brief Material scalar property identifiers used by renderer passes.
     */
    enum class MaterialProperty : uint8_t
    {
        Tessellation = 0,
        CullMode     = 1,
        MaxEnum
    };

    /**
     * @enum MaterialTextureType
     * @brief Material texture slot identifiers used by renderer passes.
     */
    enum class MaterialTextureType : uint8_t
    {
        Color  = 0,
        Normal = 1,
        MaxEnum
    };

    // -------------------------------------------------------
    // Minimal scene-object stubs used by renderer passes.
    // Full definitions live in scene/ once those subsystems are complete.
    // -------------------------------------------------------

    /**
     * @class Material
     * @brief Stub material class providing the interface consumed by renderer passes.
     */
    class Material
    {
    public:
        virtual ~Material() = default;
        virtual bool  IsTransparent() const                               { return false; }
        virtual bool  IsAlphaTested() const                               { return false; }
        virtual float GetProperty(MaterialProperty /*prop*/) const        { return 0.0f;  }
        virtual uint32_t GetIndex() const                                 { return 0;     }
        virtual bool  HasTextureOfType(MaterialTextureType /*t*/) const   { return false; }
    };

    /**
     * @class Entity
     * @brief Stub entity class providing the interface consumed by renderer passes.
     */
    class Entity
    {
    public:
        virtual ~Entity() = default;
        virtual xMath::Matrix GetMatrix() const                          { return xMath::Matrix{}; }
        virtual void SetMatrixPrevious(const xMath::Matrix& /*m*/)       {}
        virtual xMath::Vec3 GetPosition() const                          { return xMath::Vec3{};   }
        virtual xMath::Vec3 GetForward() const                           { return xMath::Vec3{0.0f, 0.0f, 1.0f}; }
        template<typename T> T* GetComponent()                           { return nullptr; }
    };

    /**
     * @class Renderable
     * @brief Stub renderable class providing the interface consumed by renderer passes.
     */
    class Renderable : public RefCounted
    {
    public:
        virtual ~Renderable() = default;
        virtual Material*   GetMaterial() const                          { return nullptr; }
        virtual Entity*     GetEntity() const                            { return nullptr; }
        virtual Buffer*     GetVertexBuffer() const                      { return nullptr; }
        virtual Buffer*     GetInstanceBuffer() const                    { return nullptr; }
        virtual Buffer*     GetIndexBuffer() const                       { return nullptr; }
        virtual uint32_t    GetIndexCount(uint32_t /*lod*/ = 0) const    { return 0; }
        virtual uint32_t    GetIndexOffset(uint32_t /*lod*/ = 0) const   { return 0; }
        virtual uint32_t    GetVertexOffset(uint32_t /*lod*/ = 0) const  { return 0; }
    };

    /**
     * @class Mesh
     * @brief Stub mesh class providing the interface consumed by renderer passes.
     */
    class Mesh
    {
    public:
        virtual ~Mesh() = default;
        virtual Buffer*   GetVertexBuffer() const    { return nullptr; }
        virtual Buffer*   GetIndexBuffer() const     { return nullptr; }
        virtual uint32_t  GetGlobalIndexOffset() const  { return 0; }
        virtual uint32_t  GetGlobalVertexOffset() const { return 0; }
    };

     /**
      * @struct GeometryBuffer
      * @brief Stub geometry buffer providing bindless index/vertex buffer access.
      */
     struct GeometryBuffer
     {
		//static Buffer* GetIndexBuffer()  { return nullptr; }
		//static Buffer* GetVertexBuffer() { return nullptr; }
		/** @brief Create static geometry resources used by built-in passes (quad VB/IB). */
		static void Initialize();
		/** @brief Release static geometry resources created by Initialize(). */
		static void Shutdown();
		/** @brief Get the static quad index buffer. */
		static Buffer* GetIndexBuffer();
		/** @brief Get the static quad vertex buffer. */
		static Buffer* GetVertexBuffer();
     };

    // -------------------------------------------------------
    // Renderer_DrawCall: per-draw submission record
    // -------------------------------------------------------

    /**
     * @struct Renderer_DrawCall
     * @brief Represents one opaque or transparent draw call submitted to the renderer.
     * Populated by scene traversal, consumed by render passes.
     */
    struct Renderer_DrawCall
    {
        Renderable* renderable        = nullptr; ///< Scene renderable to draw
        bool        camera_visible    = false;   ///< Passes frustum / occlusion cull
        bool        is_occluder       = false;   ///< Used as Hi-Z occluder mesh
        uint32_t    draw_data_index   = 0;       ///< Index into the GPU draw-data buffer
        uint32_t    lod_index         = 0;       ///< LOD level to render
        uint32_t    instance_index    = 0;       ///< First instance offset
        uint32_t    instance_count    = 1;       ///< Number of instances
    };

    // -------------------------------------------------------
    // CVar: lightweight console-variable stub
    // Provides GetValue() / GetValueAs<T>() used by renderer passes.
    // Replace with the real console-variable system when available.
    // -------------------------------------------------------

    /**
     * @struct CVar
     * @brief Lightweight console-variable stub consumed by renderer pass code.
     */
    struct CVar
    {
        float m_Value = 0.0f;

        float GetValue() const { return m_Value; }

        template<typename T>
        T GetValueAs() const { return static_cast<T>(m_Value != 0.0f); }

        void SetValue(float v) { m_Value = v; }
    };

}

// -------------------------------------------------------
