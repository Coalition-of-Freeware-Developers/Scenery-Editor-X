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
 * gpu_timer.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "gpu_timer.h"
#include "vulkan/command_list.h"
#include "vulkan/device.h"
#include "vulkan/render_context.h"
#include <mutex>
#include <set>
#include <unordered_map>
#include <SceneryEditorX/core/frame_profiler.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		constexpr uint32_t MAX_QUERIES = 128;	// Two per pass: 63 passes and the frame

		struct ListQueries
		{
			VkQueryPool pool = VK_NULL_HANDLE;
			std::vector<std::string> names;		// Scope names, recording order
			std::vector<int> scopeStart;		// Query index of each scope's start
			uint32_t used = 0;					// Queries written since BeginFrame
			bool recorded = false;				// A frame was recorded: results come back next time
		};

		struct State
		{
			std::mutex mutex;
			std::unordered_map<const CommandList *, ListQueries> lists;
			CommandList *current = nullptr;
			std::vector<GpuTimer::Result> results;
			double periodNs = 0.0;
			bool unsupported = false;
			std::set<std::string> counterNames;	// --perf-log counters keep the name pointer: these never move
		};

		State &S()
		{
			static State state;
			return state;
		}

		VkDevice LogicalDevice()
		{
			const Ref<RenderContext> context = RenderContext::Get();
			if (!context)
				return VK_NULL_HANDLE;
			const Ref<Device> device = context->GetDevice();
			return device ? device->GetDevice() : VK_NULL_HANDLE;
		}

		ListQueries *Current(State &s)
		{
			if (!s.current)
				return nullptr;
			const auto it = s.lists.find(s.current);
			return it == s.lists.end() || it->second.pool == VK_NULL_HANDLE ? nullptr : &it->second;
		}

		int Write(State &s, CommandList *cmdList)
		{
			ListQueries *queries = Current(s);
			if (!queries || cmdList != s.current || queries->used >= MAX_QUERIES)
				return -1;
			const uint32_t index = queries->used++;
			vkCmdWriteTimestamp2(cmdList->GetCommandBuffer(), VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, queries->pool, index);
			return static_cast<int>(index);
		}
	}

	void GpuTimer::BeginFrame(CommandList *cmdList)
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		s.current = nullptr;
		if (!cmdList || s.unsupported)
			return;
		const VkDevice device = LogicalDevice();
		if (device == VK_NULL_HANDLE)
			return;
		if (s.periodNs <= 0.0)
		{
			s.periodNs = Device::GetDeviceStatics().timestampPeriod;
			if (s.periodNs <= 0.0)
			{
				s.unsupported = true;	// The device has no timestamps on this queue
				return;
			}
		}

		ListQueries &queries = s.lists[cmdList];
		if (queries.pool == VK_NULL_HANDLE)
		{
			VkQueryPoolCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			info.queryType = VK_QUERY_TYPE_TIMESTAMP;
			info.queryCount = MAX_QUERIES;
			if (vkCreateQueryPool(device, &info, nullptr, &queries.pool) != VK_SUCCESS)
			{
				queries.pool = VK_NULL_HANDLE;
				return;
			}
		}
		else if (queries.recorded && queries.used >= 2)
		{
			// This list is being recorded again, so what it timed last time has completed
			std::vector<uint64_t> stamps(queries.used, 0);
			if (vkGetQueryPoolResults(device, queries.pool, 0, queries.used, stamps.size() * sizeof(uint64_t), stamps.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS)
			{
				std::vector<Result> results;
				results.reserve(queries.names.size() + 1);
				const auto ms = [&](const uint64_t from, const uint64_t to) { return to > from ? static_cast<double>(to - from) * s.periodNs * 1e-6 : 0.0; };
				for (size_t i = 0; i < queries.names.size(); ++i)
				{
					const int start = queries.scopeStart[i];
					if (start >= 0 && start + 1 < static_cast<int>(queries.used))
						results.push_back({ queries.names[i], ms(stamps[start], stamps[start + 1]) });
				}
				results.push_back({ "total", ms(stamps.front(), stamps[queries.used - 1]) });
				if (Perf::IsEnabled())
				{
					for (const Result &result : results)
						Perf::SetCounter(s.counterNames.insert("gpu ms " + result.name).first->c_str(), result.milliseconds);
				}
				s.results = std::move(results);
			}
		}

		vkCmdResetQueryPool(cmdList->GetCommandBuffer(), queries.pool, 0, MAX_QUERIES);
		queries.names.clear();
		queries.scopeStart.clear();
		queries.used = 0;
		queries.recorded = false;
		s.current = cmdList;
		Write(s, cmdList);	// Frame start
	}

	void GpuTimer::EndFrame(CommandList *cmdList)
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		if (Write(s, cmdList) >= 0)
		{
			if (ListQueries *queries = Current(s))
				queries->recorded = true;
		}
		s.current = nullptr;
	}

	GpuTimer::Scope::Scope(CommandList *cmdList, const char *name) : m_CmdList(cmdList)
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		ListQueries *queries = Current(s);
		if (!queries || cmdList != s.current || queries->used + 2 >= MAX_QUERIES)
			return;
		m_Index = Write(s, cmdList);
		if (m_Index >= 0)
		{
			queries->names.emplace_back(name);
			queries->scopeStart.push_back(m_Index);
		}
	}

	GpuTimer::Scope::~Scope()
	{
		if (m_Index < 0)
			return;
		State &s = S();
		std::lock_guard lock(s.mutex);
		// The end stamp must directly follow the start (scopes don't nest): move the scope's start if others came between
		ListQueries *queries = Current(s);
		if (!queries || m_CmdList != s.current)
			return;
		if (static_cast<uint32_t>(m_Index) + 1 != queries->used && !queries->scopeStart.empty())
		{
			queries->names.pop_back();
			queries->scopeStart.pop_back();
			return;
		}
		Write(s, m_CmdList);
	}

	std::vector<GpuTimer::Result> GpuTimer::GetResults()
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		return s.results;
	}

	void GpuTimer::Shutdown()
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		const VkDevice device = LogicalDevice();
		for (auto &[list, queries] : s.lists)
		{
			if (queries.pool != VK_NULL_HANDLE && device != VK_NULL_HANDLE)
				vkDestroyQueryPool(device, queries.pool, nullptr);
		}
		s.lists.clear();
		s.current = nullptr;
	}
}

// -------------------------------------------------------
