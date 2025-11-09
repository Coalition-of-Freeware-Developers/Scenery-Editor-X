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

// -------------------------------------------------------

namespace SceneryEditorX
{

    struct SamplerSpec
    {
        FilterMode filterMin = FilterMode::Nearest;
        FilterMode filterMag = FilterMode::Nearest;
        FilterMode filterMipMap = FilterMode::Nearest;
        SamplerWrap samplerAddressMode = SamplerWrap::Repeat;
        DepthCompareOperator comparisonFunction = DepthCompareOperator::Never;
        float anisotropy = 0.0f;
        bool comparisonEnabled = false;
        float mipBias = 0.0f;
    };

    class Sampler : public RefCounted
    {
    public:
        explicit Sampler(const SamplerSpec &samplerSpec, const std::string &debugName);
		virtual ~Sampler() override;

		FilterMode GetFilterMin()						const { return m_samplerSpec.filterMin; }
        FilterMode GetFilterMag()						const { return m_samplerSpec.filterMag; }
        FilterMode GetFilterMipmap()					const { return m_samplerSpec.filterMipMap; }
        SamplerWrap GetAddressMode()					const { return m_samplerSpec.samplerAddressMode; }
        DepthCompareOperator GetComparisonFunction()	const { return m_samplerSpec.comparisonFunction; }
        bool GetAnisotropyEnabled()                     const { return m_samplerSpec.anisotropy != 0; }
        bool GetComparisonEnabled()                     const { return m_samplerSpec.comparisonEnabled; }
		void* GetResource()								const { return m_resource; }

    private:
        void CreateResource();
        SamplerSpec m_samplerSpec;
        std::string samplerName;
        void* m_resource = nullptr;

    };

}

// -------------------------------------------------------
