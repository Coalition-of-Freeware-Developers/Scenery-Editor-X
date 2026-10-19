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
 * frame_profiler.cpp
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#include "frame_profiler.h"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>
#include <SceneryEditorX/logging/logging.hpp>

// -------------------------------------------------------

namespace SceneryEditorX::Perf
{
	namespace
	{
		constexpr double REPORT_INTERVAL_SECONDS = 2.0;

		struct ScopeStats
		{
			const char *name = nullptr;
			double totalMs = 0.0;		// Over the report interval
			double frameMs = 0.0;		// This frame so far
			double maxFrameMs = 0.0;	// Worst frame of the interval
			uint32_t samples = 0;
		};

		struct Counter
		{
			const char *name = nullptr;
			double value = 0.0;
		};

		std::atomic<bool> s_Enabled = false;
		std::mutex s_Mutex;
		std::vector<ScopeStats> s_Scopes;
		std::vector<Counter> s_Counters;
		std::chrono::steady_clock::time_point s_IntervalStart;
		std::chrono::steady_clock::time_point s_FrameStart;
		double s_MaxFrameMs = 0.0;
		uint32_t s_Frames = 0;

		ScopeStats &FindScope(const char *name)
		{
			for (ScopeStats &scope : s_Scopes)
			{
				if (scope.name == name)
					return scope;
			}
			return s_Scopes.emplace_back(ScopeStats{ name });
		}
	}

	void SetEnabled(const bool enabled)
	{
		std::scoped_lock lock(s_Mutex);
		s_Enabled = enabled;
		s_Scopes.clear();
		s_Counters.clear();
		s_Frames = 0;
		s_MaxFrameMs = 0.0;
		s_IntervalStart = s_FrameStart = std::chrono::steady_clock::now();
	}

	bool IsEnabled()
	{
		return s_Enabled.load(std::memory_order_relaxed);
	}

	void AddSample(const char *name, const double milliseconds)
	{
		if (!IsEnabled() || !name)
			return;

		std::scoped_lock lock(s_Mutex);
		ScopeStats &scope = FindScope(name);
		scope.totalMs += milliseconds;
		scope.frameMs += milliseconds;
		++scope.samples;
	}

	void SetCounter(const char *name, const double value)
	{
		if (!IsEnabled() || !name)
			return;

		std::scoped_lock lock(s_Mutex);
		for (Counter &counter : s_Counters)
		{
			if (counter.name == name)
			{
				counter.value = value;
				return;
			}
		}
		s_Counters.push_back({ name, value });
	}

	void EndFrame()
	{
		if (!IsEnabled())
			return;

		std::scoped_lock lock(s_Mutex);
		const auto now = std::chrono::steady_clock::now();
		const double frameMs = std::chrono::duration<double, std::milli>(now - s_FrameStart).count();
		s_FrameStart = now;
		s_MaxFrameMs = std::max(s_MaxFrameMs, frameMs);
		++s_Frames;

		for (ScopeStats &scope : s_Scopes)
		{
			scope.maxFrameMs = std::max(scope.maxFrameMs, scope.frameMs);
			scope.frameMs = 0.0;
		}

		const double elapsedSeconds = std::chrono::duration<double>(now - s_IntervalStart).count();
		if (elapsedSeconds < REPORT_INTERVAL_SECONDS || s_Frames == 0)
			return;

		// Heaviest scopes first: average per frame, worst frame in brackets
		std::vector<const ScopeStats *> order;
		order.reserve(s_Scopes.size());
		for (const ScopeStats &scope : s_Scopes)
			order.push_back(&scope);
		std::ranges::sort(order, [](const ScopeStats *a, const ScopeStats *b) { return a->totalMs > b->totalMs; });

		std::string scopes;
		for (const ScopeStats *scope : order)
		{
			char entry[160];
			std::snprintf(entry, sizeof(entry), "%s%s %.2f ms (max %.1f)", scopes.empty() ? "" : " | ", scope->name,
				scope->totalMs / s_Frames, scope->maxFrameMs);
			scopes += entry;
		}

		std::string counters;
		for (const Counter &counter : s_Counters)
		{
			char entry[96];
			std::snprintf(entry, sizeof(entry), "%s%s %.0f", counters.empty() ? "" : ", ", counter.name, counter.value);
			counters += entry;
		}

		const double averageMs = elapsedSeconds * 1000.0 / s_Frames;
		SEDX_CORE_INFO_TAG("Perf", "{:.1f} fps ({:.1f} ms/frame, worst {:.1f} ms) | {}", 1000.0 / averageMs, averageMs, s_MaxFrameMs, scopes);
		if (!counters.empty())
			SEDX_CORE_INFO_TAG("Perf", "Counters: {}", counters);

		for (ScopeStats &scope : s_Scopes)
		{
			scope.totalMs = 0.0;
			scope.maxFrameMs = 0.0;
			scope.samples = 0;
		}
		s_Frames = 0;
		s_MaxFrameMs = 0.0;
		s_IntervalStart = now;
	}
}

// -------------------------------------------------------
