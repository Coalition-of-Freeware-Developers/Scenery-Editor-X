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
 * fonts.cpp
 * -------------------------------------------------------
 * Created: 23/03/2026
 * -------------------------------------------------------
 */
#include "fonts.h"
#include <Editor/ui/source/imgui/imgui.h>

// -------------------------------------------------------

namespace UI
{
	static std::unordered_map<std::string, ImFont*> s_Fonts;

	// -------------------------------------------------------

	void Fonts::Add(const FontConfiguration& config, bool isDefault)
	{
		if (s_Fonts.contains(config.FontName))
		{
			EDITOR_WARN_TAG("EditorUI", "Tried to add font with name '{0}' but that name is already taken!", config.FontName);
			return;
		}

		ImFontConfig imguiFontConfig;
		imguiFontConfig.MergeMode = config.MergeWithLast;
		auto& io = ImGui::GetIO();
		ImFont* font = io.Fonts->AddFontFromFileTTF(config.FilePath.data(), config.Size, &imguiFontConfig, config.GlyphRanges == nullptr ? io.Fonts->GetGlyphRangesDefault() : config.GlyphRanges);
		SEDX_CORE_VERIFY(font, "Failed to load font file!");
		s_Fonts[config.FontName] = font;

		if (isDefault)
			io.FontDefault = font;
	}

	ImFont* Fonts::Get(const std::string& fontName)
	{
		SEDX_CORE_VERIFY(s_Fonts.contains(fontName), "Failed to find font with that name!");
		return s_Fonts.at(fontName);
	}

	void Fonts::PushFont(const std::string& fontName)
	{
		const auto& io = ImGui::GetIO();

		if (!s_Fonts.contains(fontName))
		{
			ImGui::PushFont(io.FontDefault);
			return;
		}

		ImGui::PushFont(s_Fonts.at(fontName));
	}

	void Fonts::PopFont()
	{
		ImGui::PopFont();
	}

}

// -------------------------------------------------------
