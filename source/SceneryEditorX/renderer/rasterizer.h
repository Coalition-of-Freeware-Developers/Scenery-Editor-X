/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* rasterizer.h
* -------------------------------------------------------
* Created: 5/9/2025
* -------------------------------------------------------
*/
#pragma once
#include "vulkan/vk_enums.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
    struct RasterSpec
    {
        PolygonMode m_polygon_mode = PolygonMode::MaxEnum;
        bool m_depth_clip_enabled = false;
        float m_depth_bias = 0.0f;
        float m_depth_bias_clamp = 0.0f;
        float m_depth_bias_slope_scaled = 0.0f;
    };

	class Rasterizer : public RefCounted
	{
    public:
        Rasterizer() = default;
        explicit Rasterizer(const RasterSpec &rasterSpec, float line_width = 1.0f);
        virtual ~Rasterizer() override;
		
        PolygonMode GetPolygonMode()		const { return m_raster_spec.m_polygon_mode; }
        bool GetDepthClipEnabled()			const { return m_raster_spec.m_depth_clip_enabled; }
        void* GetRhiResource()				const { return m_rhi_resource; }
        float GetLineWidth()				const { return m_line_width; }
        float GetDepthBias()				const { return m_raster_spec.m_depth_bias; }
        float GetDepthBiasClamp()			const { return m_raster_spec.m_depth_bias_clamp; }
        float GetDepthBiasSlopeScaled()		const { return m_raster_spec.m_depth_bias_slope_scaled; }
        uint64_t GetHash()					const { return m_hash; }

        bool operator==(const Rasterizer &rhs) const { return m_hash == rhs.GetHash(); }

	private:
        RasterSpec m_raster_spec;
        float m_line_width = 1.0f;
        uint64_t m_hash = 0;
        void* m_rhi_resource = nullptr;
	};
}


/// -------------------------------------------------------
