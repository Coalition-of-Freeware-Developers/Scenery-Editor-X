/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* blend_state.h
* -------------------------------------------------------
* Created: 8/9/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/renderer/vulkan/vk_enums.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{

    struct BlendStateDesc
	{
        BlendSpec src				= BlendSpec::Src_Alpha;
        BlendSpec dst				= BlendSpec::Dest_Alpha;
		BlendOperation blendOp		= BlendOperation::Add;
        BlendSpec srcAlpha			= BlendSpec::One;
        BlendSpec dstAlpha			= BlendSpec::One;
		BlendOperation blendOpAlpha = BlendOperation::Add;
    };

	class BlendState : public RefCounted
	{
	public:
        BlendState(bool blendEnabled = false, BlendStateDesc blendState, float blendFactor = 0.0f);
        virtual ~BlendState() override = default;

		auto GetBlendEnabled()						  const { return m_blendEnabled; }
        auto GetSourceBlend()                         const { return m_blendState.src; }
        auto GetDestBlend()                           const { return m_blendState.dst; }
        auto GetBlendOp()                             const { return m_blendState.blendOp; }
        auto GetSourceBlendAlpha()                    const { return m_blendState.srcAlpha; }
        auto GetDestBlendAlpha()                      const { return m_blendState.dstAlpha; }
        auto GetBlendOpAlpha()                        const { return m_blendState.blendOpAlpha; }
        void SetBlendFactor(const float blendFactor)        { m_blendFactor = blendFactor; }
        float GetBlendFactor()                        const { return m_blendFactor; }
		
        uint64_t GetHash() const { return m_hash; }
        bool operator==(const BlendState& rhs) const { return m_hash == rhs.m_hash; }
        void* GetResource() const { return m_resource; }

	private:
        BlendStateDesc m_blendState;
        uint64_t m_hash = 0;

        bool m_blendEnabled = false;
        float m_blendFactor = 1.0f;

		void* m_resource = nullptr; // API-specific resource (e.g., VkPipelineColorBlendStateCreateInfo for Vulkan)
	};

}

/// -------------------------------------------------------
