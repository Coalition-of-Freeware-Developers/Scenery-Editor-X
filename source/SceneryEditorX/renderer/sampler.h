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
#include "enums.h"

// -------------------------------------------------------

namespace SceneryEditorX
{

    struct SamplerSpec
    {
        FilterMode filterMin = FilterMode::Nearest;
        FilterMode filterMag = FilterMode::Nearest;
        FilterMode filterMipMap = FilterMode::Nearest;
        SamplerWrap samplerAddressMode = SamplerWrap::Repeat;
        CompareFunc comparisonFunction = CompareFunc::Never;
        float anisotropy = 0.0f;
        bool comparisonEnabled = false;
        float mipBias = 0.0f;
    };

    class Sampler : public RefCounted
    {
    public:
        Sampler() = default;
        Sampler(const SamplerSpec &samplerSpec, const std::string &debugName);
		virtual ~Sampler() override;

		static Ref<Sampler> Get();

		FilterMode GetFilterMin()						const { return m_samplerSpec.filterMin; }
        FilterMode GetFilterMag()						const { return m_samplerSpec.filterMag; }
        FilterMode GetFilterMipmap()					const { return m_samplerSpec.filterMipMap; }
        SamplerWrap GetAddressMode()					const { return m_samplerSpec.samplerAddressMode; }
        CompareFunc GetComparisonFunction()				const { return m_samplerSpec.comparisonFunction; }
        bool GetAnisotropyEnabled()                     const { return m_samplerSpec.anisotropy != 0; }
        bool GetComparisonEnabled()                     const { return m_samplerSpec.comparisonEnabled; }
		void* GetResource()								const { return m_resource; }

    private:
        void CreateResource();          // Create the sampler resource
        SamplerSpec m_samplerSpec;      // Specification of the sampler
        std::string samplerName;        // Debug name for the sampler
        void *m_resource = nullptr;		// Internal representation of the sampler
        VkSampler m_sampler = nullptr;	// Vulkan sampler handle
    };

}

// -------------------------------------------------------
