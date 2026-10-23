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
 * editor_theme.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "editor_theme.h"
#include <cmath>
#include <SceneryEditorX/renderer/ui/ui_impl.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorTheme
{
	namespace
	{
		float SRGBToLinear(const float c)
		{
			return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
		}

		ImVec4 WithAlpha(ImVec4 color, const float alpha)
		{
			color.w = alpha;
			return color;
		}
	}

	ImVec4 ToStyle(const ImVec4 &srgbColor)
	{
		if (!UI::IsLinearColorTarget())
			return srgbColor;

		return { SRGBToLinear(srgbColor.x), SRGBToLinear(srgbColor.y), SRGBToLinear(srgbColor.z), srgbColor.w };
	}

	ImU32 ToU32(const ImVec4 &srgbColor)
	{
		return ImGui::ColorConvertFloat4ToU32(ToStyle(srgbColor));
	}

	void Apply(const float dpiScale)
	{
		using namespace Palette;

		// Start from a fresh style so re-applying (e.g. after a DPI change) never compounds scaling,
		// but keep the font sizing the UI layer already configured.
		const ImGuiStyle &current = ImGui::GetStyle();
		ImGuiStyle style;
		ImGui::StyleColorsDark(&style);
		style.FontSizeBase				= current.FontSizeBase;
		style.FontScaleMain				= current.FontScaleMain;
		style._NextFrameFontSizeBase	= current._NextFrameFontSizeBase;

		// Flat panels: no rounding on windows or tabs, light rounding on controls.
		style.WindowPadding					= ImVec2(10.0f, 8.0f);
		style.FramePadding					= ImVec2(7.0f, 4.0f);
		style.CellPadding					= ImVec2(6.0f, 3.0f);
		style.ItemSpacing					= ImVec2(6.0f, 6.0f);
		style.ItemInnerSpacing				= ImVec2(6.0f, 4.0f);
		style.IndentSpacing					= 14.0f;
		style.ScrollbarSize					= 10.0f;
		style.GrabMinSize					= 8.0f;

		style.WindowBorderSize				= 0.0f;
		style.ChildBorderSize				= 0.0f;
		style.PopupBorderSize				= 1.0f;
		style.FrameBorderSize				= 1.0f;
		style.TabBorderSize					= 0.0f;
		style.TabBarBorderSize				= 1.0f;
		style.TabBarOverlineSize			= 0.0f;	// The active tab gets a blue underline instead (EditorUI::BeginPanel)
		style.DockingSeparatorSize			= 2.0f;
		style.SeparatorTextBorderSize		= 1.0f;

		style.WindowRounding				= 0.0f;
		style.ChildRounding					= 0.0f;
		style.FrameRounding					= 2.0f;
		style.PopupRounding					= 2.0f;
		style.ScrollbarRounding				= 2.0f;
		style.GrabRounding					= 2.0f;
		style.TabRounding					= 0.0f;

		style.WindowTitleAlign				= ImVec2(0.0f, 0.5f);
		style.WindowMenuButtonPosition		= ImGuiDir_None;
		style.ColorButtonPosition			= ImGuiDir_Left;
		style.TabCloseButtonMinWidthSelected   = 0.0f;	// Close button appears on hover, like Substance's panel headers
		style.TabCloseButtonMinWidthUnselected = FLT_MAX;

		ImVec4 *colors = style.Colors;
		colors[ImGuiCol_Text]					= Text;
		colors[ImGuiCol_TextDisabled]			= TextDisabled;
		colors[ImGuiCol_WindowBg]				= Panel;
		colors[ImGuiCol_ChildBg]				= WithAlpha(Panel, 0.0f);
		colors[ImGuiCol_PopupBg]				= PanelHeader;
		colors[ImGuiCol_Border]					= Border;
		colors[ImGuiCol_BorderShadow]			= WithAlpha(Separator, 0.0f);
		colors[ImGuiCol_FrameBg]				= Frame;
		colors[ImGuiCol_FrameBgHovered]			= FrameHovered;
		colors[ImGuiCol_FrameBgActive]			= FrameActive;
		colors[ImGuiCol_TitleBg]				= PanelHeader;
		colors[ImGuiCol_TitleBgActive]			= PanelHeader;
		colors[ImGuiCol_TitleBgCollapsed]		= PanelHeader;
		colors[ImGuiCol_MenuBarBg]				= MenuBar;
		colors[ImGuiCol_ScrollbarBg]			= WithAlpha(Separator, 0.0f);
		colors[ImGuiCol_ScrollbarGrab]			= FrameActive;
		colors[ImGuiCol_ScrollbarGrabHovered]	= ButtonHovered;
		colors[ImGuiCol_ScrollbarGrabActive]	= Accent;
		colors[ImGuiCol_CheckMark]				= Accent;
		colors[ImGuiCol_SliderGrab]				= Accent;
		colors[ImGuiCol_SliderGrabActive]		= AccentHovered;
		colors[ImGuiCol_Button]					= Button;
		colors[ImGuiCol_ButtonHovered]			= ButtonHovered;
		colors[ImGuiCol_ButtonActive]			= Accent;
		colors[ImGuiCol_Header]					= AccentDim;
		colors[ImGuiCol_HeaderHovered]			= WithAlpha(FrameHovered, 0.8f);
		colors[ImGuiCol_HeaderActive]			= WithAlpha(Accent, 0.6f);
		colors[ImGuiCol_Separator]				= Separator;
		colors[ImGuiCol_SeparatorHovered]		= AccentDim;
		colors[ImGuiCol_SeparatorActive]		= Accent;
		colors[ImGuiCol_ResizeGrip]				= WithAlpha(Accent, 0.0f);
		colors[ImGuiCol_ResizeGripHovered]		= AccentDim;
		colors[ImGuiCol_ResizeGripActive]		= Accent;
		colors[ImGuiCol_InputTextCursor]		= Text;
		colors[ImGuiCol_TabHovered]				= Panel;
		colors[ImGuiCol_Tab]					= PanelHeader;
		colors[ImGuiCol_TabSelected]			= PanelHeader;
		colors[ImGuiCol_TabSelectedOverline]	= Accent;
		colors[ImGuiCol_TabDimmed]				= PanelHeader;
		colors[ImGuiCol_TabDimmedSelected]		= PanelHeader;
		colors[ImGuiCol_TabDimmedSelectedOverline] = AccentDim;
		colors[ImGuiCol_DockingPreview]			= AccentDim;
		colors[ImGuiCol_DockingEmptyBg]			= Separator;
		colors[ImGuiCol_PlotLines]				= TextLabel;
		colors[ImGuiCol_PlotLinesHovered]		= Accent;
		colors[ImGuiCol_PlotHistogram]			= Accent;
		colors[ImGuiCol_PlotHistogramHovered]	= AccentHovered;
		colors[ImGuiCol_TableHeaderBg]			= PanelHeader;
		colors[ImGuiCol_TableBorderStrong]		= Separator;
		colors[ImGuiCol_TableBorderLight]		= PanelHeader;
		colors[ImGuiCol_TableRowBg]				= WithAlpha(Panel, 0.0f);
		colors[ImGuiCol_TableRowBgAlt]			= ImVec4(1.0f, 1.0f, 1.0f, 0.02f);
		colors[ImGuiCol_TextLink]				= AccentHovered;
		colors[ImGuiCol_TextSelectedBg]			= AccentDim;
		colors[ImGuiCol_TreeLines]				= Border;
		colors[ImGuiCol_DragDropTarget]			= Accent;
		colors[ImGuiCol_DragDropTargetBg]		= AccentDim;
		colors[ImGuiCol_UnsavedMarker]			= Text;
		colors[ImGuiCol_NavCursor]				= Accent;
		colors[ImGuiCol_NavWindowingHighlight]	= ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
		colors[ImGuiCol_NavWindowingDimBg]		= ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
		colors[ImGuiCol_ModalWindowDimBg]		= ImVec4(0.0f, 0.0f, 0.0f, 0.55f);

		for (ImVec4 &color : style.Colors)
			color = ToStyle(color);

		style.ScaleAllSizes(dpiScale);
		style.FontScaleDpi = dpiScale;

		// Floating panels in their own OS windows must stay opaque and square to match docked ones.
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui::GetStyle() = style;
	}

}

// -------------------------------------------------------
