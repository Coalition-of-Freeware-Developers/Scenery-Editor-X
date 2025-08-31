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
        PrimitiveTopology topology					= PrimitiveTopology::TOPOLOGY_TRIANGLES;
        Ref<Texture> *render_target_depth_texture	= nullptr;
        Ref<Texture> *vrs_input_texture				= nullptr;
        uint32_t render_target_array_index			= 0;
        std::array< Ref<Shader>*, static_cast<uint32_t>(ShaderType::SHADER_TYPE_MAX_ENUM)> shaders = {};
        std::array< Ref<Texture>*, renderTargetCountLimit> render_target_color_textures;
		
        /// -------------------------------------------------------
		
        // dynamic properties, changing these will not create a new PSO
        bool resolution = false;
        float clearDepth = depthLoad;
        uint32_t clearStencil = stencilLoad;
        std::array<Color, renderTargetCountLimit> clearColor;
        std::string debugName; // used by the validation layer

	private:
        [[nodiscard]] bool HasShader(const ShaderType shader_stage) const;

        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint64_t hash = 0;
	};

}

/// -------------------------------------------------------
