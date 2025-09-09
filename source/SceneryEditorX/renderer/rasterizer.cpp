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

/// -------------------------------------------------------

namespace SceneryEditorX
{
    Rasterizer::Rasterizer(const RasterSpec &rasterSpec, float line_width)
    {
        // save
        m_raster_spec = rasterSpec;
        m_line_width = line_width;

        // hash
        std::hash<float> hasher;
        m_hash = hash_combine(m_hash, static_cast<uint64_t>(m_raster_spec.m_polygon_mode));
        m_hash = hash_combine(m_hash, static_cast<uint64_t>(m_raster_spec.m_depth_clip_enabled));
        m_hash = hash_combine(m_hash, static_cast<uint64_t>(m_line_width));
        m_hash = hash_combine(m_hash, static_cast<uint64_t>(hasher(m_raster_spec.m_depth_bias)));
        m_hash = hash_combine(m_hash, static_cast<uint64_t>(hasher(m_raster_spec.m_depth_bias_clamp)));
        m_hash = hash_combine(m_hash, static_cast<uint64_t>(hasher(m_raster_spec.m_depth_bias_slope_scaled)));
        m_hash = hash_combine(m_hash, static_cast<uint64_t>(hasher(m_line_width)));
    }
    
    Rasterizer::~Rasterizer() = default;

}

/// -------------------------------------------------------
