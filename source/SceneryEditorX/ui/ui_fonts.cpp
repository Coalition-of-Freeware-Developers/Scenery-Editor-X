/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui_fonts.cpp
* -------------------------------------------------------
* Created: 31/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/ui/ui_fonts.h>

/// -------------------------------------------------------

namespace SceneryEditorX::UI
{
	
	LOCAL std::unordered_map<std::string, ImFont*> s_Fonts;

    /// -------------------------------------------------------

	void Fonts::Add(const FontConfiguration& config, bool isDefault)
	{
		if (s_Fonts.contains(config.FontName))
		{
			SEDX_CORE_WARN_TAG("EditorUI", "Tried to add font with name '{0}' but that name is already taken!", config.FontName);
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

/// -------------------------------------------------------
