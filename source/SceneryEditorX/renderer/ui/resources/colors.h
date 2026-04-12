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
 * colors.h
 * -------------------------------------------------------
 * Created: 28/3/2025
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/renderer/ui/source/imgui/imgui.h>

// -------------------------------------------------------

/* TODO: Add more colors and options here. */
/* TODO: Connect with user customization and config file. */

namespace Colors::Theme
{
	constexpr auto BACKGROUND			= IM_COL32(0, 0, 0, 255);
	constexpr auto BACKGROUND_DARK		= IM_COL32(45, 45, 45, 255);
	//constexpr auto backgroundPopup  = IM_COL32(55, 55, 55, 255);
	constexpr auto GROUP_HEADER			= IM_COL32(0, 0, 0, 255);
	constexpr auto HIGHLIGHT				= IM_COL32(39, 185, 242, 255);
	constexpr auto TEXT					= IM_COL32(192, 192, 192, 255);
	constexpr auto TEXT_BRIGHTER			= IM_COL32(210, 210, 210, 255);
	constexpr auto TEXT_DARKER			= IM_COL32(128, 128, 128, 255);
	constexpr auto TEXT_ERROR			= IM_COL32(230, 51, 51, 255);
	constexpr auto TITLEBAR				= IM_COL32(0, 0, 0, 255);
	constexpr auto ACCENT				= IM_COL32(236, 158, 36, 255);
	constexpr auto SELECTION				= IM_COL32(237, 192, 119, 255);
	constexpr auto SELECTION_MUTED		= IM_COL32(237, 201, 142, 23);
	constexpr auto BACKGROUND_POPUP		= IM_COL32(50, 50, 50, 255);
	constexpr auto PROPERTY_FIELD		= IM_COL32(15, 15, 15, 255);

}

// -------------------------------------------------------
