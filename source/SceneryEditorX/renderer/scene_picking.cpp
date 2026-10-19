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
 * scene_picking.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "scene_picking.h"
#include "renderer.h"
#include "renderer_declarations.h"
#include "vulkan/buffer.h"
#include "vulkan/command_list.h"
#include "vulkan/image_resource.h"
#include "vulkan/memory_allocator.h"
#include "vulkan/push_constant_buffer.h"
#include "vulkan/pipeline/pipeline_state.h"
#include "vulkan/shader/shader_manager.h"
#include <algorithm>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/scene/scene.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		constexpr const char *LOG_TAG = "Picking";
		constexpr uint32_t MASK_ENTITIES = ScenePicking::PICK_ID_MASK + 1u;		// Every pick id has its 2 bits
		constexpr uint32_t MASK_WORDS = MASK_ENTITIES / 16u;
		constexpr size_t MAX_RESULTS = 32;										// Unclaimed results kept (oldest dropped)
		constexpr uint32_t MAX_REGION_PIXELS = 4096u * 4096u;
		constexpr float OUTLINE_RADIUS = 2.0f;
		constexpr float SELECTED_FILL = 0.12f;

		/* @brief Highlight states, two bits per entity (selection_outline.slang). */
		enum : uint32_t
		{
			STATE_SELECTED = 1u,
			STATE_HOVERED = 2u,
			STATE_ACTIVE = 3u
		};

		struct Queued
		{
			uint64_t ticket = 0;
			int32_t x = 0;
			int32_t y = 0;
			uint32_t width = 0;
			uint32_t height = 0;
		};

		struct InFlight
		{
			uint64_t ticket = 0;			// 0: cancelled, dropped on arrival
			const CommandList *list = nullptr;
			Ref<Buffer> buffer;
			ScenePicking::Result region;	// ids empty until read back
		};

		struct State
		{
			std::mutex mutex;
			uint64_t nextTicket = 1;
			std::vector<Queued> queued;
			std::vector<InFlight> inFlight;
			std::vector<std::pair<uint64_t, ScenePicking::Result>> results;

			// Highlight (2 bits per entity index), uploaded when it changed
			std::vector<uint32_t> mask;
			std::vector<uint32_t> touchedWords;	// Words with bits set, cleared before the next highlight is written
			uint64_t highlightHash = 0;
			bool highlightAny = false;
			bool maskDirty = true;
			bool outlineEnabled = true;

			Ref<Buffer> maskBuffer;
			std::unordered_map<const CommandList *, Ref<Buffer>> staging;
		};

		State &S()
		{
			static State state;
			return state;
		}

		uint64_t HashIds(uint64_t hash, const std::vector<uint32_t> &ids)
		{
			for (const uint32_t id : ids)
			{
				hash ^= id;
				hash *= 0x100000001b3ull;
			}
			hash ^= 0xffull;	// separator, so moving an id from one set to the next changes the hash
			return hash * 0x100000001b3ull;
		}

		/* @brief Host-cached, persistently mapped memory the CPU reads after the GPU copied into it. */
		Ref<Buffer> CreateReadback(const VkDeviceSize bytes)
		{
			VmaAllocationCreateInfo allocInfo{};
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			return CreateRef<Buffer>(MemoryAllocator::GetAllocator(), bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, allocInfo);
		}

		void MemoryBarrier(CommandList *cmdList, const VkPipelineStageFlags2 srcStages, const VkAccessFlags2 srcAccess, const VkPipelineStageFlags2 dstStages, const VkAccessFlags2 dstAccess)
		{
			VkMemoryBarrier2 barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
			barrier.srcStageMask = srcStages;
			barrier.srcAccessMask = srcAccess;
			barrier.dstStageMask = dstStages;
			barrier.dstAccessMask = dstAccess;

			VkDependencyInfo dependency{};
			dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependency.memoryBarrierCount = 1;
			dependency.pMemoryBarriers = &barrier;
			vkCmdPipelineBarrier2(cmdList->GetCommandBuffer(), &dependency);
		}

		void PushResult(State &s, const uint64_t ticket, ScenePicking::Result &&result)
		{
			if (s.results.size() >= MAX_RESULTS)
				s.results.erase(s.results.begin());
			s.results.emplace_back(ticket, std::move(result));
		}

		bool IsUsable(ImageResource *image)
		{
			return image && image->Get() && *image->Get() != VK_NULL_HANDLE;
		}
	}

	// -------------------------------------------------------

	entt::entity ScenePicking::Resolve(const entt::registry &registry, const uint32_t pickId)
	{
		if (pickId == 0u)
			return entt::null;
		using Traits = entt::entt_traits<entt::entity>;
		const auto index = static_cast<Traits::entity_type>(pickId - 1u);
		const entt::entity probe = Traits::construct(index, 0);
		const entt::entity handle = Traits::construct(index, registry.current(probe));
		return registry.valid(handle) ? handle : entt::null;
	}

	uint64_t ScenePicking::Request(const int32_t x, const int32_t y, const uint32_t width, const uint32_t height)
	{
		if (width == 0 || height == 0 || static_cast<uint64_t>(width) * height > MAX_REGION_PIXELS)
			return 0;
		State &s = S();
		std::lock_guard lock(s.mutex);
		const uint64_t ticket = s.nextTicket++;
		s.queued.push_back({ ticket, x, y, width, height });
		return ticket;
	}

	bool ScenePicking::TryGetResult(const uint64_t ticket, Result &out)
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		for (auto it = s.results.begin(); it != s.results.end(); ++it)
		{
			if (it->first != ticket)
				continue;
			out = std::move(it->second);
			s.results.erase(it);
			return true;
		}
		return false;
	}

	void ScenePicking::Cancel(const uint64_t ticket)
	{
		if (ticket == 0)
			return;
		State &s = S();
		std::lock_guard lock(s.mutex);
		std::erase_if(s.queued, [ticket](const Queued &queued) { return queued.ticket == ticket; });
		for (InFlight &flight : s.inFlight)
		{
			if (flight.ticket == ticket)
				flight.ticket = 0;
		}
		std::erase_if(s.results, [ticket](const auto &entry) { return entry.first == ticket; });
	}

	void ScenePicking::SetHighlight(const std::vector<uint32_t> &selected, const std::vector<uint32_t> &active, const std::vector<uint32_t> &hovered)
	{
		State &s = S();
		const uint64_t hash = HashIds(HashIds(HashIds(0xcbf29ce484222325ull, selected), active), hovered);
		std::lock_guard lock(s.mutex);
		if (hash == s.highlightHash && !s.mask.empty())
			return;
		s.highlightHash = hash;

		if (s.mask.empty())
			s.mask.assign(MASK_WORDS, 0u);
		for (const uint32_t word : s.touchedWords)
			s.mask[word] = 0u;
		s.touchedWords.clear();

		// Later sets win: hovered < selected < active
		const auto write = [&s](const std::vector<uint32_t> &ids, const uint32_t state)
		{
			for (const uint32_t id : ids)
			{
				if (id == 0u || id > PICK_ID_MASK)
					continue;
				const uint32_t index = id - 1u;
				const uint32_t word = index >> 4u;
				const uint32_t shift = (index & 15u) * 2u;
				if (s.mask[word] == 0u)
					s.touchedWords.push_back(word);
				s.mask[word] = (s.mask[word] & ~(3u << shift)) | (state << shift);
			}
		};
		write(hovered, STATE_HOVERED);
		write(selected, STATE_SELECTED);
		write(active, STATE_ACTIVE);

		s.highlightAny = !s.touchedWords.empty();
		s.maskDirty = true;
	}

	void ScenePicking::SetOutlineEnabled(const bool enabled)
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		s.outlineEnabled = enabled;
	}

	bool ScenePicking::IsOutlineEnabled()
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		return s.outlineEnabled;
	}

	void ScenePicking::RecordReadbacks(CommandList *cmdList)
	{
		ImageResource *pick = Renderer::GetRenderTarget(Renderer_RenderTarget::gbuffer_pick);
		if (!cmdList || cmdList->IsStickyInvalid() || !IsUsable(pick))
			return;

		State &s = S();
		std::lock_guard lock(s.mutex);

		// This command list is being recorded again, so what it copied last time has completed
		for (auto it = s.inFlight.begin(); it != s.inFlight.end();)
		{
			if (it->list != cmdList)
			{
				++it;
				continue;
			}
			if (it->ticket != 0 && it->buffer)
			{
				Result result = std::move(it->region);
				const size_t count = static_cast<size_t>(result.width) * result.height;
				result.ids.resize(count, 0u);
				if (VmaAllocation allocation = it->buffer->Allocation())
					vmaInvalidateAllocation(MemoryAllocator::GetAllocator(), allocation, 0, VK_WHOLE_SIZE);
				// The VMA-allocated buffer may not keep its persistent mapping in the Buffer: map it on demand (as GpuScene does)
				if (const void *mapped = it->buffer->GetMappedData() ? it->buffer->GetMappedData() : it->buffer->Map())
					std::memcpy(result.ids.data(), mapped, count * sizeof(uint32_t));
				else
					SEDX_CORE_WARN_TAG(LOG_TAG, "Could not map a pick readback buffer: the region reads as empty");
				PushResult(s, it->ticket, std::move(result));
			}
			if (it->buffer)
				it->buffer->Destroy();
			it = s.inFlight.erase(it);
		}

		if (s.queued.empty())
			return;

		const int32_t targetWidth = static_cast<int32_t>(pick->GetWidth());
		const int32_t targetHeight = static_cast<int32_t>(pick->GetHeight());
		bool copied = false;
		for (const Queued &queued : s.queued)
		{
			Result region;
			const int32_t x0 = std::clamp(queued.x, 0, targetWidth);
			const int32_t y0 = std::clamp(queued.y, 0, targetHeight);
			const int32_t x1 = std::clamp(queued.x + static_cast<int32_t>(queued.width), 0, targetWidth);
			const int32_t y1 = std::clamp(queued.y + static_cast<int32_t>(queued.height), 0, targetHeight);
			region.x = static_cast<uint32_t>(x0);
			region.y = static_cast<uint32_t>(y0);
			region.width = static_cast<uint32_t>(std::max(0, x1 - x0));
			region.height = static_cast<uint32_t>(std::max(0, y1 - y0));
			if (region.width == 0 || region.height == 0)
			{
				PushResult(s, queued.ticket, std::move(region));	// Outside the image: nothing there
				continue;
			}

			Ref<Buffer> buffer = CreateReadback(static_cast<VkDeviceSize>(region.width) * region.height * sizeof(uint32_t));
			if (!buffer || !buffer->Get())
			{
				SEDX_CORE_WARN_TAG(LOG_TAG, "Could not allocate a readback buffer for a {} x {} pick region", region.width, region.height);
				PushResult(s, queued.ticket, std::move(region));
				continue;
			}

			if (!copied)
			{
				// Ends the g-buffer's render pass too: copies can't be recorded inside one
				pick->SetLayout(Layout::ImageLayout::TransferSrc, cmdList, 0, 0);
				copied = true;
			}

			VkBufferImageCopy copy{};
			copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copy.imageSubresource.layerCount = 1;
			copy.imageOffset = { x0, y0, 0 };
			copy.imageExtent = { region.width, region.height, 1 };
			vkCmdCopyImageToBuffer(cmdList->GetCommandBuffer(), *pick->Get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer->Get(), 1, &copy);

			InFlight &flight = s.inFlight.emplace_back();
			flight.ticket = queued.ticket;
			flight.list = cmdList;
			flight.buffer = std::move(buffer);
			flight.region = std::move(region);
		}
		s.queued.clear();

		// The host reads the copies once the list's fence has signaled
		if (copied)
			MemoryBarrier(cmdList, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_HOST_BIT, VK_ACCESS_2_HOST_READ_BIT);
	}

	void ScenePicking::RecordOutline(CommandList *cmdList, ImageResource *target)
	{
		ImageResource *pick = Renderer::GetRenderTarget(Renderer_RenderTarget::gbuffer_pick);
		if (!cmdList || cmdList->IsStickyInvalid() || !IsUsable(pick) || !IsUsable(target))
			return;
		if (pick->GetWidth() != target->GetWidth() || pick->GetHeight() != target->GetHeight())
			return;	// The outline reads the pick ids pixel for pixel

		State &s = S();
		std::lock_guard lock(s.mutex);
		if (!s.outlineEnabled || !s.highlightAny)
			return;
		if (!ShaderManager::IsShaderAvailable(Renderer_Shader::selection_outline))
			return;

		if (!s.maskBuffer)
		{
			s.maskBuffer = CreateRef<Buffer>(sizeof(uint32_t), MASK_WORDS, nullptr, false, "selection_mask");
			if (!s.maskBuffer || !s.maskBuffer->Get())
			{
				SEDX_CORE_ERROR_TAG(LOG_TAG, "Could not allocate the selection mask: outlines are off");
				s.maskBuffer = nullptr;
				s.outlineEnabled = false;
				return;
			}
			s.maskDirty = true;
		}

		if (s.maskDirty && !s.mask.empty())
		{
			// Per command list: its previous use has completed (the list is being recorded again), so nothing reads it
			Ref<Buffer> &staging = s.staging[cmdList];
			const VkDeviceSize bytes = static_cast<VkDeviceSize>(MASK_WORDS) * sizeof(uint32_t);
			if (!staging)
				staging = CreateRef<Buffer>(1, static_cast<uint32_t>(bytes), nullptr, true, "selection_mask_staging");
			void *mapped = staging ? (staging->GetMappedData() ? staging->GetMappedData() : staging->Map()) : nullptr;
			if (!mapped)
				return;
			std::memcpy(mapped, s.mask.data(), static_cast<size_t>(bytes));

			cmdList->EndRenderPass();
			const VkBufferCopy copy{ 0, 0, bytes };
			vkCmdCopyBuffer(cmdList->GetCommandBuffer(), staging->Get(), s.maskBuffer->Get(), 1, &copy);
			MemoryBarrier(cmdList, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_READ_BIT);
			s.maskDirty = false;
		}

		pick->SetLayout(Layout::ImageLayout::General, cmdList, 0, 0);

		PipelineState pso;
		pso.name = "selection_outline";
		pso.shaders[static_cast<uint32_t>(StageType::Compute)] = ShaderManager::GetShader(Renderer_Shader::selection_outline);
		cmdList->SetPipelineState(pso);

		PushConstantBuffer_Pass constants{};
		constants.SetF4Value(OUTLINE_RADIUS, SELECTED_FILL, 0.0f, 0.0f);
		cmdList->PushConstants(constants);

		cmdList->SetTexture(Renderer_BindingsUav::tex, target);
		cmdList->SetTexture(Renderer_BindingsUav::tex_uint, pick);
		cmdList->SetBuffer(Renderer_BindingsUav::selection_mask, s.maskBuffer.Get());
		cmdList->Dispatch(target);
	}

	void ScenePicking::EndFrame(CommandList *cmdList)
	{
		ImageResource *pick = Renderer::GetRenderTarget(Renderer_RenderTarget::gbuffer_pick);
		if (cmdList && !cmdList->IsStickyInvalid() && IsUsable(pick))
			pick->SetLayout(Layout::ImageLayout::Attachment, cmdList, 0, 0);
	}

	void ScenePicking::Shutdown()
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		for (InFlight &flight : s.inFlight)
		{
			if (flight.buffer)
				flight.buffer->Destroy();
		}
		s.inFlight.clear();
		s.queued.clear();
		s.results.clear();
		for (auto &[list, buffer] : s.staging)
		{
			if (buffer)
				buffer->Destroy();
		}
		s.staging.clear();
		if (s.maskBuffer)
			s.maskBuffer->Destroy();
		s.maskBuffer = nullptr;
		s.maskDirty = true;
	}
}

// -------------------------------------------------------
