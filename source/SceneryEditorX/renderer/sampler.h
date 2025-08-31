/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* sampler.h
* -------------------------------------------------------
* Created: 31/8/2025
* -------------------------------------------------------
*/
#pragma once
#include "vulkan/vk_enums.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{
    class Sampler : public RefCounted
    {
    public:
		explicit Sampler(const std::string &debug_name);
		virtual ~Sampler() override;

		FilterMode GetFilterMin()						const { return m_filter_min; }
        FilterMode GetFilterMag()						const { return m_filter_mag; }
        FilterMode GetFilterMipmap()					const { return m_filter_mipmap; }
        SamplerWrap GetAddressMode()					const { return m_sampler_address_mode; }
        DepthCompareOperator GetComparisonFunction()	const { return m_comparison_function; }
        bool GetAnisotropyEnabled()                     const { return m_anisotropy != 0; }
        bool GetComparisonEnabled()                     const { return m_comparison_enabled; }
		void* GetResource()								const { return m_resource; }

    private:
        void CreateResource();

        FilterMode m_filter_min                         = FilterMode::Nearest;
        FilterMode m_filter_mag                         = FilterMode::Nearest;
        FilterMode m_filter_mipmap                      = FilterMode::Nearest;
        SamplerWrap m_sampler_address_mode				= SamplerWrap::Repeat;
        DepthCompareOperator m_comparison_function		= DepthCompareOperator::Always;
        float m_anisotropy                              = 0;
        bool m_comparison_enabled                       = false;
        float m_mip_lod_bias                            = 0.0f;

        void* m_resource = nullptr;

    };

}

/// -------------------------------------------------------
