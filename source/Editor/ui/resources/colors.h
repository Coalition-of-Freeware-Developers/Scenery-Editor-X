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
#include <imgui/imgui.h>

/// -------------------------------------------------------

/// TODO: 1) Add more colors and options here.
///		 2) Connect with user customization and config file.

namespace Colors::Theme
{
	constexpr auto background       = IM_COL32(0, 0, 0, 255);
	constexpr auto backgroundDark   = IM_COL32(45, 45, 45, 255);
	//constexpr auto backgroundPopup  = IM_COL32(55, 55, 55, 255);
	constexpr auto groupHeader		= IM_COL32(0, 0, 0, 255);
	constexpr auto highlight		= IM_COL32(39, 185, 242, 255);
	constexpr auto text				= IM_COL32(192, 192, 192, 255);
	constexpr auto textBrighter		= IM_COL32(210, 210, 210, 255);
	constexpr auto textDarker		= IM_COL32(128, 128, 128, 255);
	constexpr auto textError		= IM_COL32(230, 51, 51, 255);
	constexpr auto titlebar			= IM_COL32(0, 0, 0, 255);
	constexpr auto accent			= IM_COL32(236, 158, 36, 255);
	constexpr auto selection        = IM_COL32(237, 192, 119, 255);
	constexpr auto selectionMuted	= IM_COL32(237, 201, 142, 23);
	constexpr auto backgroundPopup	= IM_COL32(50, 50, 50, 255);
	constexpr auto propertyField    = IM_COL32(15, 15, 15, 255);

}

/// -------------------------------------------------------
