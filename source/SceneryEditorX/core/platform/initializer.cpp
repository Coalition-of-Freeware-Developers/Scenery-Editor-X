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
 * initializer.cpp
 * -------------------------------------------------------
 * Created: 25/5/2025
 * -------------------------------------------------------
 */
#include "initializer.h"
#include <SceneryEditorX/core/memory/memory.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/vulkan/debug/graphics_debug.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Apply command line arguments to configure logging options before Log::Init() is called. This allows users to set the desired logging level via CLI.
	 * @param args The command line arguments to parse for logging options.
	 */
	static void ApplyCliLoggingOptions(const std::vector<std::string>& args)
	{
		// simple handling: --verbose or --verbose=<Level>
		for (const auto &arg : args)
		{
			if (arg == "--verbose")
			{
				Log::SetInitialLevel(Log::Level::Trace);
				return;
			}

			constexpr const char logPrefix[] = "--verbose=";
			if (arg.starts_with(logPrefix))
			{
				std::string val = arg.substr(sizeof(logPrefix)-1);
				Log::SetInitialLevel(Log::LevelFromString(val));
				return;
			}

			if (arg == "--renderdoc")
			{
				Debugging::SetRenderdocEnabled();
				if (!Debugging::IsRenderdocEnabled())
				{
					SEDX_CORE_TRACE_TAG("Initializer","RenderDoc is not enabled");
					return;
				}
				return;
			}

		    constexpr const char renderdocPrefix[] = "--renderdoc=";
			if (arg.starts_with(renderdocPrefix))
			{
				std::string val = arg.substr(sizeof(renderdocPrefix)-1);
				Debugging::SetRenderdocEnabled();
				if (!Debugging::IsRenderdocEnabled())
				{
					SEDX_CORE_TRACE_TAG("Initializer","RenderDoc is not enabled");
					return;
				}
				return;
			}

		}
	}

	void InitCore(const PlatformContext& ctx)
	{
		Allocator::Init();

		// parse CLI and configure desired logging level BEFORE Log::Init()
		ApplyCliLoggingOptions(ctx.GetCommandLineArgs());

		Log::Init();
		Log::LogHeader();
	}

	void Shutdown()
	{
		Log::ShutDown();
	}

}

// -------------------------------------------------------
