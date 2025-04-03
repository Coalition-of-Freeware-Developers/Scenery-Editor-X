/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* fonts.h
* -------------------------------------------------------
* Created: 29/3/2025
* -------------------------------------------------------
*/
#pragma once
#include <imgui/imgui.h>

// ---------------------------------------------------------

struct Font
{
    /**
	 * @brief Constructor
	 * @param name The name of the font file that exists within 'assets/fonts' (without extension)
	 * @param size The font size, scaled by DPI
	 */

	/*
    Font(const std::string &name, float size) : name{name}, data{SceneryEditorX::fs::read_asset("fonts/" + name + ".ttf")}, size{size}
    {
        // Keep ownership of the font data to avoid a double delete
        ImFontConfig font_config{};
        font_config.FontDataOwnedByAtlas = false;

        if (size < 1.0f)
        {
            size = 20.0f;
        }

        ImGuiIO &io = ImGui::GetIO();
        handle = io.Fonts->AddFontFromMemoryTTF(data.data(), static_cast<int>(data.size()), size, &font_config);
    }

    ImFont *handle{nullptr};

    std::string name;

    std::vector<uint8_t> data;

    float size{};
	*/
};


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
 */

/*
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
