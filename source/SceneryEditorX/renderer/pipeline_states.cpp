/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* pipeline_states.cpp
* -------------------------------------------------------
* Created: 28/8/2025
* -------------------------------------------------------
*/
#include "pipeline_states.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	
	PipelineStates::PipelineStates()
	{
        clearColor.fill(color_load);
        render_target_color_textures.fill(nullptr);
	}
	
	PipelineStates::~PipelineStates() = default;

    void PipelineStates::Prepare()
	{
        hash = compute_hash(*this);
        get_dimensions(*this, &m_width, &m_height);
        validate(*this);
	}
	
	bool PipelineStates::HasClearValues() const
	{
        if (clearDepth != depthLoad && clearDepth != depth_dont_care)
            return true;

        if (clearStencil != stencilLoad && clearStencil != stencil_dont_care)
            return true;

        for (const Color& color : clearColor)
        {
            if (color != colorLoad && color != color_dont_care)
                return true;
        }

        return false;
	}
	
	bool PipelineStates::IsGraphics() const
	{
        return (HasShader(ShaderType::Vertex) || HasShader(ShaderType::Fragment)) &&
               !HasShader(ShaderType::Compute);

	}
	
	bool PipelineStates::IsCompute() const
	{
        return HasShader(ShaderType::Compute) && !(HasShader(ShaderType::Vertex) || HasShader(ShaderType::Fragment));
	}
	
	bool PipelineStates::HasTessellation()
	{
	    return false;
	}
	
	bool PipelineStates::HasShader(const ShaderType shader_stage) const
	{
        return shaders[static_cast<uint32_t>(shader_stage)] != nullptr;
	}
}

/// -------------------------------------------------------
