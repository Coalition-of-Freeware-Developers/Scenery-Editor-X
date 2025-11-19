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

// -------------------------------------------------------

namespace SceneryEditorX
{
    struct RasterSpec
    {
        PolygonMode polygonMode		= PolygonMode::MaxEnum;
        bool depthClipEnabled		= false;
        float depthBias				= 0.0f;
        float depthBiasClamp		= 0.0f;
        float depthBiasSlopeScaled	= 0.0f;
    };

    // -------------------------------------------------------

	class Rasterizer : public RefCounted
	{
    public:
        Rasterizer() = default;
        explicit Rasterizer(const RasterSpec &rasterSpec, float line_width = 1.0f);
        virtual ~Rasterizer() override;

        bool operator==(const Rasterizer &rhs) const { return m_hash == rhs.GetHash(); }

        PolygonMode GetPolygonMode()		const { return m_rasterSpec.polygonMode; }
        bool GetDepthClipEnabled()			const { return m_rasterSpec.depthClipEnabled; }
        void* GetRhiResource()				const { return m_rhiResource; }
        float GetLineWidth()				const { return m_lineWidth; }
        float GetDepthBias()				const { return m_rasterSpec.depthBias; }
        float GetDepthBiasClamp()			const { return m_rasterSpec.depthBiasClamp; }
        float GetDepthBiasSlopeScaled()		const { return m_rasterSpec.depthBiasSlopeScaled; }
        uint64_t GetHash()					const { return m_hash; }

	private:
        RasterSpec m_rasterSpec;
        float m_lineWidth	= 1.0f;
        uint64_t m_hash		= 0;
        void* m_rhiResource = nullptr;
	};
}

// -------------------------------------------------------
