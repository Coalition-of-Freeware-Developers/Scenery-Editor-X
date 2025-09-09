/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* blend_state.cpp
* -------------------------------------------------------
* Created: 8/9/2025
* -------------------------------------------------------
*/
#include "blend_state.h"
#include "vulkan/vk_data.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
	
	BlendState::BlendState(bool blendEnabled, BlendStateDesc blendState, float blendFactor)
	{
        // save
        m_blendEnabled = blendEnabled;
        m_blendState = blendState;
        m_blendFactor = blendFactor;

	    // hash
        m_hash = hash_combine(m_hash, static_cast<uint64_t>(m_blendEnabled));
        m_hash = hash_combine(m_hash, static_cast<uint64_t>(m_blendState.src));
        m_hash = hash_combine(m_hash, static_cast<uint64_t>(m_blendState.dst));
        m_hash = hash_combine(m_hash, static_cast<uint64_t>(m_blendState.blendOp));
        m_hash = hash_combine(m_hash, static_cast<uint64_t>(m_blendState.srcAlpha));
        m_hash = hash_combine(m_hash, static_cast<uint64_t>(m_blendState.dstAlpha));
        m_hash = hash_combine(m_hash, static_cast<uint64_t>(m_blendState.blendOpAlpha));
        m_hash = hash_combine(m_hash, static_cast<uint64_t>(m_blendFactor));
	}

}


///	-------------------------------------------------------
