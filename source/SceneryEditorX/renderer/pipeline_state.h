/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* pipeline_state.h
* -------------------------------------------------------
* Created: 28/8/2025
* -------------------------------------------------------
*/
#pragma once
/*
#include "blend_state.h"
#include "depth_stencil.h"
#include "rasterizer.h"
#include "texture.h"
#include "shaders/shader.h"
#include "vulkan_data.h"
#include <colors.h>

// -------------------------------------------------------


namespace SceneryEditorX
{
	// Forward declaration to break circular dependency
	class SwapChain;

	// Number of actual shader stages (Vertex, TessControl, TessEval, Geometry, Fragment, Compute, Raygen)
	constexpr uint32_t MAX_SHADER_STAGES = 8;

    class PipelineState : public RefCounted
	{
	public:
        PipelineState();
        virtual ~PipelineState() override;

	    void Prepare();
        [[nodiscard]] bool HasClearValues() const;
        [[nodiscard]] uint64_t GetHash() const   { return m_Hash; }
        [[nodiscard]] uint32_t GetWidth() const  { return m_Width; }
        [[nodiscard]] uint32_t GetHeight() const { return m_Height; }
        [[nodiscard]] bool IsGraphics() const;
        [[nodiscard]] bool IsCompute() const;
        bool HasTessellation();

        // -------------------------------------------------------

	    Rasterizer *rasterizerState					= nullptr;
        BlendState *blendState						= nullptr;
        DepthStencil *depthStencil					= nullptr;
        Ref<SwapChain> *renderTargetSwapchain		= nullptr;
        PrimitiveTopology topology					= PrimitiveTopology::Triangles;
        Ref<Texture2D> *renderTargetDepthTexture	= nullptr;
        Ref<Texture2D> *inputTexture				= nullptr;
        uint32_t renderTargetArrayIdx				= 0;
        std::array<Ref<Shader> *, MAX_SHADER_STAGES> shaders = {};
        std::array<Ref<Texture2D> *, MAX_RENDER_TARGET_COUNT> renderTargetColorTextures;

        // -------------------------------------------------------
		
        // dynamic properties, changing these will not create a new PSO
        bool resolution = false;
        float clearDepth = DEPTH_LOAD;
        uint32_t clearStencil = STENCIL_LOAD;
        std::array<Color, MAX_RENDER_TARGET_COUNT> clearColor;
        std::string debugName; // used by the validation layer

	private:
        [[nodiscard]] bool HasShader(ShaderStage::Stage shaderStage) const;

        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        uint64_t m_Hash = 0;
	};

}
*/

// -------------------------------------------------------
