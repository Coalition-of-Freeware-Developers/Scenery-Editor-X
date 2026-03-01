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
#include <resource.h>
#include <SceneryEditorX/core/base.h>
#include <SceneryEditorX/core/memory/memory.h>
#include <SceneryEditorX/logging/logging.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
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
	        constexpr const char prefix[] = "--verbose=";
	        if (arg.starts_with(prefix))
	        {
	            std::string val = arg.substr(sizeof(prefix)-1);
	            Log::SetInitialLevel(Log::LevelFromString(val));
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
