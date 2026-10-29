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
 * gpu_timer.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <string>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class CommandList;

	/**
	 * @class GpuTimer
	 * @brief GPU time per frame-graph pass (plan TASK-027): timestamp queries around each pass, read back when the
	 * command list comes round again (no stall), shown in the RENDERER panel and logged with --perf-log.
	 *
	 * Render thread: BeginFrame() once per frame on the list the passes record into, then a Scope per pass.
	 * Any thread: GetResults().
	 */
	class GpuTimer
	{
	public:
		struct Result
		{
			std::string name;
			double milliseconds = 0.0;
		};

		/* @brief Collects the timings this command list recorded last time, then resets its queries. */
		static void BeginFrame(CommandList *cmdList);

		/* @brief Marks the end of the frame's passes (the total). */
		static void EndFrame(CommandList *cmdList);

		/* @brief Times what is recorded between construction and destruction. */
		class Scope
		{
		public:
			Scope(CommandList *cmdList, const char *name);
			~Scope();
			Scope(const Scope &) = delete;
			Scope &operator=(const Scope &) = delete;

		private:
			CommandList *m_CmdList = nullptr;
			int m_Index = -1;
			const char *m_EndBreadcrumb = nullptr;	// GPU crash breadcrumb after the scope (Debugging::SetBreadcrumb)
		};

		/* @brief The last frame that finished on the GPU: one entry per pass in recording order, then "total". */
		static std::vector<Result> GetResults();

		/* @brief Releases the query pools. Call before the device goes away. */
		static void Shutdown();
	};
}

// -------------------------------------------------------
