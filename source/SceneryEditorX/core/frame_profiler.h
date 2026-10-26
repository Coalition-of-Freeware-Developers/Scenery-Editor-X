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
 * frame_profiler.h
 * -------------------------------------------------------
 * Created: 18/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <chrono>
#include <cstdint>

// -------------------------------------------------------

namespace SceneryEditorX::Perf
{
	/**
	 * @brief Lightweight CPU frame profiler: named scopes are summed per frame and logged as averages every couple
	 * of seconds (tag "Perf"). Off unless enabled (the editor's --perf-log flag), so scopes cost one branch.
	 * Scope names must be string literals: they are keyed by pointer.
	 */
	void SetEnabled(bool enabled);
	[[nodiscard]] bool IsEnabled();

	/* @brief Adds one sample of a named scope to the current frame. */
	void AddSample(const char *name, double milliseconds);

	/* @brief Sets a named value shown with the next report (draw counts, instance counts, ...). */
	void SetCounter(const char *name, double value);

	/* @brief Closes a frame of the main loop; logs the report when the interval has passed. */
	void EndFrame();

	/**
	 * @class ScopedTimer
	 * @brief Times its own lifetime into AddSample() when the profiler is enabled.
	 */
	class ScopedTimer
	{
	public:
		explicit ScopedTimer(const char *name) : m_Name(IsEnabled() ? name : nullptr)
		{
			if (m_Name)
				m_Start = std::chrono::steady_clock::now();
		}

		~ScopedTimer()
		{
			if (m_Name)
				AddSample(m_Name, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - m_Start).count());
		}

		ScopedTimer(const ScopedTimer &) = delete;
		ScopedTimer &operator=(const ScopedTimer &) = delete;

	private:
		const char *m_Name;
		std::chrono::steady_clock::time_point m_Start{};
	};
}

#define SEDX_PERF_CONCAT_INNER(a, b) a##b
#define SEDX_PERF_CONCAT(a, b) SEDX_PERF_CONCAT_INNER(a, b)
/* @brief Times the enclosing scope under a string-literal name (see Perf::ScopedTimer). */
#define SEDX_PERF_SCOPE(name) ::SceneryEditorX::Perf::ScopedTimer SEDX_PERF_CONCAT(sedxPerfScope_, __LINE__)(name)

// -------------------------------------------------------
