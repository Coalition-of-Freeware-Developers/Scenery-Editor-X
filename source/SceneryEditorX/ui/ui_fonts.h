/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* ui_fonts.h
* -------------------------------------------------------
* Created: 31/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <imgui/imgui.h>

/// -------------------------------------------------------

namespace SceneryEditorX::UI
{
	
	struct FontConfiguration
	{
		std::string FontName;
		std::string_view FilePath;
		float Size = 16.0f;
		const ImWchar* GlyphRanges = nullptr;
		bool MergeWithLast = false;
	};

	class Fonts
	{
	public:
		static void Add(const FontConfiguration& config, bool isDefault = false);
		static void PushFont(const std::string& fontName);
		static void PopFont();
		static ImFont* Get(const std::string& fontName);
	};

}

/// -------------------------------------------------------
