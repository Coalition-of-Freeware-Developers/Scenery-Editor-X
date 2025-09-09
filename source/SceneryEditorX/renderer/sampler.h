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

    struct SamplerSpec
    {
        FilterMode filter_min = FilterMode::Nearest;
        FilterMode filter_mag = FilterMode::Nearest;
        FilterMode filter_mipmap = FilterMode::Nearest;
        SamplerWrap sampler_address_mode = SamplerWrap::Repeat;
        DepthCompareOperator comparison_function = DepthCompareOperator::Never;
        float anisotropy = 0.0f;
        bool comparison_enabled = false;
        float mip_bias = 0.0f;
    };

    class Sampler : public RefCounted
    {
    public:
        explicit Sampler(const SamplerSpec &samplerSpec, const std::string &debug_name);
		virtual ~Sampler() override;

		FilterMode GetFilterMin()						const { return m_samplerSpec.filter_min; }
        FilterMode GetFilterMag()						const { return m_samplerSpec.filter_mag; }
        FilterMode GetFilterMipmap()					const { return m_samplerSpec.filter_mipmap; }
        SamplerWrap GetAddressMode()					const { return m_samplerSpec.sampler_address_mode; }
        DepthCompareOperator GetComparisonFunction()	const { return m_samplerSpec.comparison_function; }
        bool GetAnisotropyEnabled()                     const { return m_samplerSpec.anisotropy != 0; }
        bool GetComparisonEnabled()                     const { return m_samplerSpec.comparison_enabled; }
		void* GetResource()								const { return m_resource; }

    private:
        void CreateResource();
        SamplerSpec m_samplerSpec;
        std::string samplerName;
        void* m_resource = nullptr;

    };

}

/// -------------------------------------------------------
