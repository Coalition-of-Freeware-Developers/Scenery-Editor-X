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
 * editor_theme.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/renderer/ui/source/imgui/imgui.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorTheme
{
	/**
	 * Dark, flat, low-contrast palette modelled on Adobe Substance 3D Painter: near-black separators between
	 * panels, mid-grey panel bodies, uppercase panel tabs underlined in blue, and a single blue accent.
	 * Values are sRGB as picked from the reference; Apply() and ToU32() convert them for sRGB swapchains.
	 */
	namespace Palette
	{
		inline constexpr ImVec4 MenuBar			= { 0.118f, 0.118f, 0.118f, 1.0f };	// #1E1E1E
		inline constexpr ImVec4 Separator		= { 0.098f, 0.098f, 0.098f, 1.0f };	// #191919 gaps between docked panels
		inline constexpr ImVec4 PanelHeader		= { 0.153f, 0.153f, 0.153f, 1.0f };	// #272727 tab bar / panel title row
		inline constexpr ImVec4 Panel			= { 0.169f, 0.169f, 0.169f, 1.0f };	// #2B2B2B panel body
		inline constexpr ImVec4 Viewport		= { 0.180f, 0.180f, 0.180f, 1.0f };	// #2E2E2E empty viewport
		inline constexpr ImVec4 ToolStrip		= { 0.149f, 0.149f, 0.149f, 1.0f };	// #262626 left / right icon strips
		inline constexpr ImVec4 Frame			= { 0.212f, 0.212f, 0.212f, 1.0f };	// #363636 inputs, combos
		inline constexpr ImVec4 FrameHovered	= { 0.251f, 0.251f, 0.251f, 1.0f };	// #404040
		inline constexpr ImVec4 FrameActive		= { 0.282f, 0.282f, 0.282f, 1.0f };	// #484848
		inline constexpr ImVec4 Button			= { 0.231f, 0.231f, 0.231f, 1.0f };	// #3B3B3B
		inline constexpr ImVec4 ButtonHovered	= { 0.286f, 0.286f, 0.286f, 1.0f };	// #494949
		inline constexpr ImVec4 Border			= { 0.267f, 0.267f, 0.267f, 1.0f };	// #444444 frame outlines
		inline constexpr ImVec4 Accent			= { 0.122f, 0.541f, 0.839f, 1.0f };	// #1F8AD6 selection, active tab underline
		inline constexpr ImVec4 AccentHovered	= { 0.212f, 0.616f, 0.902f, 1.0f };	// #369DE6
		inline constexpr ImVec4 AccentDim		= { 0.122f, 0.541f, 0.839f, 0.35f };
		inline constexpr ImVec4 Text			= { 0.847f, 0.847f, 0.847f, 1.0f };	// #D8D8D8
		inline constexpr ImVec4 TextLabel		= { 0.616f, 0.616f, 0.616f, 1.0f };	// #9D9D9D property labels, inactive tabs
		inline constexpr ImVec4 TextDisabled	= { 0.420f, 0.420f, 0.420f, 1.0f };	// #6B6B6B
		inline constexpr ImVec4 Error			= { 0.890f, 0.267f, 0.267f, 1.0f };	// #E34444 status bar errors
		inline constexpr ImVec4 Warning			= { 0.929f, 0.706f, 0.259f, 1.0f };	// #EDB442
	}

	/**
	 * @brief Applies the editor style and colors to the current ImGui context.
	 * @param dpiScale Monitor content scale; sizes and paddings are multiplied by it.
	 */
	void Apply(float dpiScale = 1.0f);

	/**
	 * @brief Converts a palette color for direct draw-list use (custom widgets, underlines, strips).
	 * Applies the same sRGB-to-linear conversion as the style colors when the swapchain encodes sRGB.
	 */
	ImU32 ToU32(const ImVec4 &srgbColor);

	/* @brief Same as ToU32() but returns the converted float color, e.g. for PushStyleColor(). */
	ImVec4 ToStyle(const ImVec4 &srgbColor);

	/* @brief Height of the bottom status bar, before DPI scaling. */
	inline constexpr float StatusBarHeight = 24.0f;

	/* @brief Width of the left tool strip and right panel strip, before DPI scaling. */
	inline constexpr float ToolStripWidth = 38.0f;

}

// -------------------------------------------------------
