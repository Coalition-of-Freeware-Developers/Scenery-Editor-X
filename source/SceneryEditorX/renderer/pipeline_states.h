/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* pipeline_states.h
* -------------------------------------------------------
* Created: 28/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <colors.h>

#include "blend_state.h"
#include "texture.h"
#include "shaders/shader.h"
#include "vulkan/vk_swapchain.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class PipelineStates
	{
	public:
        PipelineStates();
        ~PipelineStates();

	    void Prepare();
        [[nodiscard]] bool HasClearValues() const;
        [[nodiscard]] uint64_t GetHash() const   { return hash; }
        [[nodiscard]] uint32_t GetWidth() const  { return m_width; }
        [[nodiscard]] uint32_t GetHeight() const { return m_height; }
        [[nodiscard]] bool IsGraphics() const;
        [[nodiscard]] bool IsCompute() const;
        bool HasTessellation();

        /// -------------------------------------------------------

	    RasterizerState *rasterizerState			= nullptr;
        BlendState *blendState						= nullptr;
        DepthStencilState *depthStencil				= nullptr;
        Ref<SwapChain> *render_target_swapchain		= nullptr;
        PrimitiveTopology topology					= PrimitiveTopology::Triangles;
        Ref<Texture2D> *render_target_depth_texture	= nullptr;
        Ref<Texture2D> *vrs_input_texture			= nullptr;
        uint32_t render_target_array_index			= 0;
        std::array<Ref<Shader> *, static_cast<uint32_t>(ShaderStage::Stage::MaxEnum)> shaders = {};
        std::array< Ref<Texture2D>*, renderTargetCountLimit> render_target_color_textures;
		
        /// -------------------------------------------------------
		
        // dynamic properties, changing these will not create a new PSO
        bool resolution = false;
        float clearDepth = depthLoad;
        uint32_t clearStencil = stencilLoad;
        std::array<Color, renderTargetCountLimit> clearColor;
        std::string debugName; // used by the validation layer

	private:
        [[nodiscard]] bool HasShader(ShaderStage::Stage shader_stage) const;

        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint64_t hash = 0;
	};

}

/// -------------------------------------------------------
