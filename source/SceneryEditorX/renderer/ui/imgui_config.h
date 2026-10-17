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
 * imgui_config.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once

/**
 * Scenery Editor X configuration for Dear ImGui.
 *
 * Injected into every imgui.h include through the IMGUI_USER_CONFIG compile definition (AppCore, PUBLIC),
 * so the vendored ImGui sources stay untouched and can be upgraded by copying upstream files over them.
 */

namespace SceneryEditorX::UI
{
	/**
	 * @brief Logs a failed ImGui assertion to the engine log (flushed to disk), then breaks into an attached
	 * debugger or aborts. The default assert() only reaches a console that disappears with the process.
	 */
	[[noreturn]] void ReportImGuiAssert(const char *expression, const char *file, int line);
}

#define IM_ASSERT(_EXPR) do { if (!(_EXPR)) ::SceneryEditorX::UI::ReportImGuiAssert(#_EXPR, __FILE__, __LINE__); } while (0)
