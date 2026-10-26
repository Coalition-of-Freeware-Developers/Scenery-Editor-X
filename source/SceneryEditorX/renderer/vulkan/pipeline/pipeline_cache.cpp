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
 * pipeline_cache.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "pipeline_cache.h"
#include "pipeline.h"
#include <SceneryEditorX/renderer/vulkan/bindless_manager.h>
#include <SceneryEditorX/renderer/vulkan/descriptor.h>
#include <SceneryEditorX/renderer/vulkan/descriptor_set.h>
#include <SceneryEditorX/renderer/vulkan/shader/shader.h>
#include <algorithm>
#include <mutex>
#include <unordered_map>

// -------------------------------------------------------

namespace SceneryEditorX
{

	/**
	 * @brief The objects cached for one PipelineState hash.
	 */
	struct PipelineCacheEntry
	{
		Ref<DescriptorSet> descriptorLayout;
		Ref<Pipeline> pipeline;  // created on the first GetPipeline() after the layout
		std::array<const Shader*, static_cast<size_t>(StageType::MaxEnum)> shaders = {}; // programs the entry was built from, for reload eviction
		uint64_t lastUsedFrame = 0;
	};

	static std::mutex s_Mutex;
	static std::unordered_map<uint64_t, PipelineCacheEntry> s_Entries;
	static std::unordered_map<const Shader*, uint64_t> s_WatchedShaders; // shader -> object id its reload callback was registered for
	static uint64_t s_Frame = 0;

	/**
	 * @brief Release an entry's Vulkan objects through the deletion queue.
	 * Command lists may still hold a Ref to the objects; those Refs then only keep the emptied C++ objects alive.
	 * @param entry The entry to release.
	 */
	static void ReleaseEntry(const PipelineCacheEntry& entry)
	{
		if (entry.pipeline)
			entry.pipeline->DestroyPipeline();

		if (entry.descriptorLayout)
			entry.descriptorLayout->Destroy();
	}

	/**
	 * @brief Register a reload callback on a shader once, so its entries are evicted when it hot-reloads.
	 * @param shader The shader to watch.
	 */
	static void WatchShader(Shader* shader)
	{
		const auto it = s_WatchedShaders.find(shader);
		if (it != s_WatchedShaders.end() && it->second == shader->GetObjectId())
			return;

		// A new shader at a recycled address registers again; the old shader's callbacks died with it.
		s_WatchedShaders[shader] = shader->GetObjectId();
		shader->AddShaderReloadedCallback([shader]() { PipelineCache::EvictShader(shader); });
	}

	Ref<DescriptorSet> PipelineCache::GetDescriptorLayout(PipelineState& pso)
	{
		const uint64_t hash = pso.GetHash();

		std::scoped_lock lock(s_Mutex);
		if (const auto it = s_Entries.find(hash); it != s_Entries.end())
		{
			it->second.lastUsedFrame = s_Frame;
			return it->second.descriptorLayout;
		}

		BindlessManager bindlessManager;
		std::array<Descriptor, 256> descriptors = {};
		size_t descriptorCount = 0;
		if (!bindlessManager.GetDescriptorsFromPipelineState(pso, descriptors.data(), descriptorCount))
		{
			SEDX_CORE_ERROR_TAG("PipelineCache", "Descriptor reflection merge failed for PSO '{}' due to binding type conflict", pso.name ? pso.name : "<unnamed>");
			return nullptr;
		}

		const char* descriptorSetName = pso.name ? pso.name : "PipelineCacheDescriptorSet";
		Ref<DescriptorSet> layout = CreateRef<DescriptorSet>(descriptors.data(), descriptorCount, descriptorSetName);
		if (layout->GetLayout() == VK_NULL_HANDLE)
		{
			SEDX_CORE_ERROR_TAG("PipelineCache", "Failed to create descriptor layout from reflected PSO descriptors for '{}'", descriptorSetName);
			return nullptr;
		}

		PipelineCacheEntry& entry = s_Entries[hash];
		entry.descriptorLayout = layout;
		entry.lastUsedFrame = s_Frame;
		for (size_t stage = 0; stage < pso.shaders.size(); ++stage)
		{
			entry.shaders[stage] = pso.shaders[stage];
			if (pso.shaders[stage])
				WatchShader(pso.shaders[stage]);
		}

		SEDX_CORE_TRACE_TAG("PipelineCache", "Cached descriptor layout for PSO '{}' (hash 0x{:016X}, {} entries)", descriptorSetName, hash, s_Entries.size());
		return layout;
	}

	Ref<Pipeline> PipelineCache::GetPipeline(PipelineState& pso)
	{
		std::scoped_lock lock(s_Mutex);
		const auto it = s_Entries.find(pso.GetHash());
		if (it == s_Entries.end() || !it->second.descriptorLayout)
			return nullptr;

		PipelineCacheEntry& entry = it->second;
		entry.lastUsedFrame = s_Frame;

		// A failed creation stays cached too: retrying every frame cannot succeed until a shader reload changes the hash.
		if (!entry.pipeline)
			entry.pipeline = CreateRef<Pipeline>(pso, entry.descriptorLayout.Get());

		return entry.pipeline;
	}

	void PipelineCache::Tick()
	{
		std::scoped_lock lock(s_Mutex);
		++s_Frame;
		if (s_Frame <= STALE_FRAME_COUNT)
			return;

		const uint64_t oldestAllowedFrame = s_Frame - STALE_FRAME_COUNT;
		size_t evicted = 0;
		for (auto it = s_Entries.begin(); it != s_Entries.end();)
		{
			if (it->second.lastUsedFrame < oldestAllowedFrame)
			{
				ReleaseEntry(it->second);
				it = s_Entries.erase(it);
				++evicted;
			}
			else
			{
				++it;
			}
		}

		if (evicted > 0)
			SEDX_CORE_TRACE_TAG("PipelineCache", "Evicted {} entries unused for {} frames ({} remaining)", evicted, STALE_FRAME_COUNT, s_Entries.size());
	}

	void PipelineCache::EvictShader(const Shader* shader)
	{
		std::scoped_lock lock(s_Mutex);
		size_t evicted = 0;
		for (auto it = s_Entries.begin(); it != s_Entries.end();)
		{
			const auto& shaders = it->second.shaders;
			if (std::ranges::find(shaders, shader) != shaders.end())
			{
				ReleaseEntry(it->second);
				it = s_Entries.erase(it);
				++evicted;
			}
			else
			{
				++it;
			}
		}

		SEDX_CORE_TRACE_TAG("PipelineCache", "Evicted {} entries after shader '{}' reloaded", evicted, shader ? shader->GetName() : "<null>");
	}

	void PipelineCache::Clear()
	{
		std::scoped_lock lock(s_Mutex);
		for (const auto& [hash, entry] : s_Entries)
			ReleaseEntry(entry);

		const size_t count = s_Entries.size();
		s_Entries.clear();
		s_WatchedShaders.clear();
		s_Frame = 0;

		SEDX_CORE_INFO_TAG("PipelineCache", "Pipeline cache cleared ({} entries)", count);
	}

	size_t PipelineCache::GetEntryCount()
	{
		std::scoped_lock lock(s_Mutex);
		return s_Entries.size();
	}

}

// -------------------------------------------------------
