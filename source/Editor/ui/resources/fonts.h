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
 * fonts.h
 * -------------------------------------------------------
 * Created: 29/3/2025
 * -------------------------------------------------------
 */
#pragma once
#include "Editor/ui/source/imgui/imgui.h"

// -------------------------------------------------------

namespace  UI
{

	/**
	 * @struct FontConfiguration
	 * @brief 
	 */
	struct FontConfiguration
	{
		std::string FontName;
		std::string_view FilePath;
		float Size = 16.0f;
		const ImWchar* GlyphRanges = nullptr;
		bool MergeWithLast = false;
	};
	
	/**
	 * @class Fonts
	 * @brief 
	 */
	class Fonts
	{
	public:
		static void Add(const FontConfiguration& config, bool isDefault = false);
		static void PushFont(const std::string& fontName);
		static void PopFont();
		static ImFont* Get(const std::string& fontName);
	};

}

// -------------------------------------------------------

// Load Fonts
/*
 - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
 - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
 - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
 - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
 - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
 - Read 'docs/FONTS.md' for more instructions and details.
 - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
 - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
io.Fonts->AddFontDefault();
io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
IM_ASSERT(font != nullptr);
*/

// TODO: Fully integrate the font system into the engine

/*
ImGuiIO &io = ImGui::GetIO();
(void)io;
ImFont *font1 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Black.otf", 15.0f);
ImFont *font2 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-BlackItalic.otf", 15.0f);
ImFont *font3 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Bold.otf", 15.0f);
ImFont *font4 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-BoldItalic.otf", 15.0f);
ImFont *font5 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Book.otf", 15.0f);
ImFont *font6 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-BookItalic.otf", 15.0f);
ImFont *font7 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Demi.otf", 15.0f);
ImFont *font8 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-DemiItalic.otf", 15.0f);
ImFont *font9 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Light.otf", 15.0f);
ImFont *font10 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-LightItalic.otf", 15.0f);
ImFont *font11 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Medium.otf", 15.0f);
ImFont *font12 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-MediumItalic.otf", 15.0f);
ImFont *font13 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Thin.otf", 15.0f);
ImFont *font14 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-ThinItalic.otf", 15.0f);
ImFont *font15 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-Ultra.otf", 15.0f);
ImFont *font16 = io.Fonts->AddFontFromFileTTF("../assets/fonts/industry/Industry-UltraItalic.otf", 15.0f);
*/
