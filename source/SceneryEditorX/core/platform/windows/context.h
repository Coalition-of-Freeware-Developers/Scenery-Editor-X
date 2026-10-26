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
 * context.h
 * -------------------------------------------------------
 * Windows application context
 * -------------------------------------------------------
 * Created: 25/5/2025
 * -------------------------------------------------------
 */
#pragma once
#include <Windows.h>
#include <SceneryEditorX/core/platform/platform_context.h>

// -----------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class WindowsContext
	 * @brief Represents the Windows-specific application context.
	 */
	class WindowsContext final : public PlatformContext
	{
	public:
		/**
		 * @brief Constructs a WindowsContext object.
		 * @param hInstance A handle to the current instance of the application.
		 * @param hPrevInstance A handle to the previous instance of the application (always NULL in modern Windows).
		 * @param lpCmdLine The command line for the application, excluding the program name.
		 * @param nCmdShow Controls how the window is to be shown.
		 */
		WindowsContext(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow);
		~WindowsContext() override = default;
	};

} // namespace SceneryEditorX

// -----------------------------------------------
