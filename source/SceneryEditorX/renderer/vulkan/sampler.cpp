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
 * sampler.cpp
 * -------------------------------------------------------
 * Created: 04/03/2026
 * -------------------------------------------------------
 */
#include "sampler.h"
#include "render_context.h"
#include <limits>
#include <volk/volk.h>

// -----------------------------------------------------------------

namespace SceneryEditorX
{
	
	Sampler::Sampler(const SamplerSpec& spec)
	{
		m_Spec = spec;
		m_SamplerAddrMode = spec.addressMode;

		Ref<Device> device = RenderContext::Get()->GetDevice();
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter           = spec.mag;
		samplerInfo.minFilter           = spec.min;
		samplerInfo.mipmapMode          = spec.mipMap;
		samplerInfo.addressModeU        = spec.addressMode;
		samplerInfo.addressModeV        = spec.addressMode;
		samplerInfo.addressModeW        = spec.addressMode;
		samplerInfo.anisotropyEnable    = spec.anisotropy > std::numeric_limits<float>::epsilon();
		samplerInfo.maxAnisotropy       = spec.anisotropy;
		samplerInfo.compareEnable       = spec.compareEnabled ? VK_TRUE : VK_FALSE;
		samplerInfo.compareOp           = spec.compareFunc;
		samplerInfo.borderColor         = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
		samplerInfo.mipLodBias          = spec.mipLodBias;
		samplerInfo.minLod              = 0.0f;
		samplerInfo.maxLod              = FLT_MAX;

		SEDX_VK_RESULT_ASSERT(vkCreateSampler(device->GetLogicalDevice(), &samplerInfo, nullptr, &m_Sampler), "Failed to create Vulkan sampler");
	}

	Sampler::~Sampler()
	{
		QueueManager::AddDeletionQueue(ResourceType::Sampler, m_Sampler);
		m_Spec = {};
		m_Sampler = nullptr;
	}

}

// -----------------------------------------------------------------
