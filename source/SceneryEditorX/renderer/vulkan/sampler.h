/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * sampler.h
 * -------------------------------------------------------
 * Created: 04/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/utils/inheritance.h>

// -----------------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @struct SamplerSpec
	 * @brief 
	 */
	struct SamplerSpec
	{
		VkFilter min                        = VK_FILTER_NEAREST;
		VkFilter mag                        = VK_FILTER_NEAREST;
		VkSamplerMipmapMode mipMap          = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		VkSamplerAddressMode addressMode    = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkCompareOp compareFunc             = VK_COMPARE_OP_ALWAYS;
		float anisotropy                    = 0.0f;
		bool compareEnabled                 = false;
		float mipLodBias                    = 0.0f;
	};

	// TODO: Tie in with bindless system and track sampler states in a global array, with dirty flags to update shader bindings when states change (e.g., anisotropy level changes)

	/**
	 * @class Sampler
	 * @brief A wrapper around a Vulkan sampler object, created from a SamplerSpec.
	 */
	class Sampler : public SharedObject
	{
	public:
		Sampler(const SamplerSpec& spec);
		virtual ~Sampler() override;

		VkSampler Get()								const { return m_Sampler; }
		VkFilter GetFilterMin()						const { return m_Spec.min; }
		VkFilter GetFilterMag()						const { return m_Spec.mag; }
		VkSamplerMipmapMode GetFilterMipmap()		const { return m_Spec.mipMap; }
		VkSamplerAddressMode GetAddressMode()		const { return m_Spec.addressMode; }
		VkCompareOp GetComparisonFunction()			const { return m_Spec.compareFunc; }
		bool GetAnisotropyEnabled()                 const { return m_Spec.anisotropy != 0; }
		bool GetComparisonEnabled()                 const { return m_Spec.compareEnabled; }
	protected:
		VkSamplerAddressMode m_SamplerAddrMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		const SamplerSpec &spec;

	private:
		SamplerSpec m_Spec;
		VkSampler m_Sampler	= nullptr;
	};

}

// -----------------------------------------------------------------
