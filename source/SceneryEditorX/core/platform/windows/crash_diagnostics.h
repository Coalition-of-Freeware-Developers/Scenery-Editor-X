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
 * crash_diagnostics.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX::Diagnostics
{
	/**
	 * @brief Installs in-process handlers that write the reason and a symbolized call stack to the engine log
	 * (flushed) when the process is about to die: abort(), std::terminate (uncaught C++ exceptions), CRT
	 * assertion/error reports and unhandled SEH exceptions (access violations, ...). Crashes after Log::ShutDown()
	 * (static destructors) are appended to SceneryEditorX.log directly and sent to OutputDebugString.
	 * Every report includes the process and system commit headroom (CPU allocations fail at the commit limit). An
	 * unhandled exception also logs the faulting address of an access violation and what the address space holds
	 * there, the stacks the other threads had at the time, and writes SceneryEditorX_crash.dmp next to the log.
	 *
	 * Complements the out-of-process CrashHandler service: this needs no setup and puts the cause in
	 * SceneryEditorX.log, where it is otherwise lost with the debug console. Safe to call more than once.
	 * No-op on non-Windows platforms.
	 */
	void InstallCrashHandlers();
}

// -------------------------------------------------------
