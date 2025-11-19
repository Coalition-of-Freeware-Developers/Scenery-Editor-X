/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* rasterizer.cpp
* -------------------------------------------------------
* Created: 5/9/2025
* -------------------------------------------------------
*/
#include "rasterizer.h"
#include <functional>
#include "vulkan/vk_data.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
    Rasterizer::Rasterizer(const RasterSpec &rasterSpec, float line_width)
    {
        // save
        m_rasterSpec = rasterSpec;
        m_lineWidth = line_width;

        // hash
        std::hash<float> hasher;
        m_hash = HashCombine(m_hash, static_cast<uint64_t>(m_rasterSpec.polygonMode));
        m_hash = HashCombine(m_hash, static_cast<uint64_t>(m_rasterSpec.depthClipEnabled));
        m_hash = HashCombine(m_hash, static_cast<uint64_t>(m_lineWidth));
        m_hash = HashCombine(m_hash, static_cast<uint64_t>(hasher(m_rasterSpec.depthBias)));
        m_hash = HashCombine(m_hash, static_cast<uint64_t>(hasher(m_rasterSpec.depthBiasClamp)));
        m_hash = HashCombine(m_hash, static_cast<uint64_t>(hasher(m_rasterSpec.depthBiasSlopeScaled)));
        m_hash = HashCombine(m_hash, static_cast<uint64_t>(hasher(m_lineWidth)));
    }
    
    Rasterizer::~Rasterizer() = default;

}

// -------------------------------------------------------
