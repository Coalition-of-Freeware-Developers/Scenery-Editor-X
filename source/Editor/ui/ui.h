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
 * ui.h
 * -------------------------------------------------------
 * Created: 25/3/2025
 * -------------------------------------------------------
 */
#pragma once
#include <Editor/ui/source/imgui/imgui.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>
#include <SceneryEditorX/ui/ui_manager.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	enum class IconType : uint8_t;
	class Renderer;
}

struct Image
{
	//std::shared_ptr<ImageResource> resource;
	uint32_t width = 0;
	uint32_t height = 0;
	//ImageUsageFlags usage;
	//Format format;
	//Layout::ImageLayout layout;
	//AspectFlags aspect;
	uint32_t layers = 1;
	uint32_t RID();
	ImTextureID ImGuiRID();
	ImTextureID ImGuiRID(uint32_t layer);
};

//struct Font
//{
//  /**
//	 * @brief Constructor
//	 * @param name The name of the font file that exists within 'assets/fonts' (without extension)
//	 * @param size The font size, scaled by DPI
//	 */
//    Font(const std::string &name, float size)
//        : name{name}, data{SceneryEditorX::fs::read_asset("fonts/" + name + ".ttf")}, size{size}
//    {
//        // Keep ownership of the font data to avoid a double delete
//        ImFontConfig font_config{};
//        font_config.FontDataOwnedByAtlas = false;
//
//        if (size < 1.0f)
//        {
//            size = 20.0f;
//        }
//
//        ImGuiIO &io = ImGui::GetIO();
//        handle = io.Fonts->AddFontFromMemoryTTF(data.data(), static_cast<int>(data.size()), size, &font_config);
//    }
//
//    ImFont *handle{nullptr};
//
//    std::string name;
//
//    std::vector<uint8_t> data;
//
//    float size{};
//};

// -------------------------------------------------------

namespace UI
{
	static const ImVec4 DEFAULT_TINT(1, 1, 1, 1);

	/**
	 * @brief 
	 * @param texture 
	 * @param size 
	 * @param border 
	 * @param tint 
	 * @return 
	 */
	bool ImageButton(SceneryEditorX::ImageResource *texture, const xMath::Vec2& size, bool border, ImVec4 tint = {1,1,1,1});

	/**
	 * @brief 
	 * @param icon 
	 * @param size 
	 */
	void Image(const SceneryEditorX::IconType icon, const float size);

	/**
	 * @brief 
	 * @param texture 
	 * @param size 
	 * @param tint 
	 * @param border 
	 */
	void Image(SceneryEditorX::ImageResource *texture, const ImVec2& size, const ImVec4& tint = DEFAULT_TINT, const ImColor& border = ImColor(0, 0, 0, 0));

	/**
	 * @brief 
	 * @param texture 
	 * @param size 
	 * @param border 
	 */
	void Image(SceneryEditorX::ImageResource *texture, const xMath::Vec2 &size, bool border = false);

	/**
	 * @brief 
	 * @param label 
	 * @param alignment 
	 * @return 
	 */
	bool ButtonCenteredOnLine(const char *label, float alignment = 0.5f);

	/**
	 * @brief 
	 * @param label 
	 * @param flags 
	 * @return 
	 */
	bool CollapsingHeader(const char *label, ImGuiTreeNodeFlags flags = 0);

	/**
	 * @brief 
	 * @param label 
	 * @param size 
	 * @return 
	 */
	bool Button(const char *label, const ImVec2 &size = ImVec2(0, 0));

	/**
	 * @brief 
	 * @param label 
	 * @param value 
	 * @return 
	 */
	bool CheckBox(const char *label, bool *value);

	/**
	 * @brief 
	 * @param icon 
	 * @param size 
	 * @param tint 
	 */
	void Image(const SceneryEditorX::IconType icon, const float size, const ImVec4 tint);

	// image slot - returns true if the user clicked on the slot (for browse functionality)
	/**
	 * @brief 
	 * @param texture_in 
	 * @param setter 
	 * @return 
	 */
	bool ImageSlot(SceneryEditorX::ImageResource *texture_in, const std::function<void(SceneryEditorX::ImageResource *)> &setter);

	/**
	 * @brief 
	 * @param text 
	 */
	void Tooltip(const char *text);

	// a drag float which will wrap the mouse cursor around the edges of the screen
	/**
	 * @brief 
	 * @param label 
	 * @param v 
	 * @param v_speed 
	 * @param v_min 
	 * @param v_max 
	 * @param format 
	 * @param flags 
	 * @return 
	 */
	bool DrawFloatWrap(const char *label, float *v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char *format = "%.3f", const ImGuiSliderFlags flags = 0);

	/**
	 * @brief 
	 * @param label 
	 * @param options 
	 * @param selectionIndex 
	 * @return 
	 */
	bool ComboBox(const char *label, const std::vector<std::string> &options, uint32_t *selectionIndex);

	/**
	 * @brief 
	 * @param label 
	 * @param vector 
	 * @param vertical 
	 */
	void Vec3(const char *label, xMath::Vec3 &vector, bool vertical = true);

	/* @brief Initialize custom ImGui extensions */
	void InitImGuiExtensions();


} // namespace UI

// -------------------------------------------------------
