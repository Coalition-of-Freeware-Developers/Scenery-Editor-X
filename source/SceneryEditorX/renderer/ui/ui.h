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
#include "source/imgui/imgui.h"
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>

// -------------------------------------------------------

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

/**
 * @namespace UI
 * @brief User interface utilities and helper functions for ImGui rendering.
 */
namespace SceneryEditorX::UI
{
	/* @brief Default tint color for images (white with full opacity). */
	static const ImVec4 DEFAULT_TINT(1, 1, 1, 1);

	/**
	 * @brief Creates an image button with the specified texture, size, border, and tint color.
	 * @param texture Pointer to the image resource.
	 * @param size Size of the image button.
	 * @param border Whether to draw a border around the image button.
	 * @param tint  Tint color for the image.
	 * @return True if the image button is clicked, false otherwise.
	 */
	bool ImageButton(ImageResource *texture, const Vec2& size, bool border, ImVec4 tint = {1,1,1,1});

	/**
	 * @brief Creates an image with the specified icon and size.
	 * @param icon The icon type to display.
	 * @param size The size of the icon.
	 */
	void Image(const IconType icon, const float size);

	/**
	 * @brief Creates an image with the specified texture, size, tint color, and border color.
	 * @param texture Pointer to the image resource.
	 * @param size Size of the image.
	 * @param tint  Tint color for the image.
	 * @param border Border color for the image.
	 */
	void Image(ImageResource *texture, const ImVec2& size, const ImVec4& tint = DEFAULT_TINT, const ImColor& border = ImColor(0, 0, 0, 0));

	/**
	 * @brief Creates an image with the specified texture, size, and border option.
	 * @param texture Pointer to the image resource.
	 * @param size Size of the image.
	 * @param border Whether to draw a border around the image.
	 */
	void Image(ImageResource *texture, const Vec2 &size, bool border = false);

	/**
	 * @brief Creates a button centered on the current line with the specified label and alignment.
	 * @param label The text to display on the button.
	 * @param alignment The alignment of the button on the line (0.0 = left, 0.5 = center, 1.0 = right).
	 * @return True if the button is clicked, false otherwise.
	 */
	bool ButtonCenteredOnLine(const char *label, float alignment = 0.5f);

	/**
	 * @brief Creates a collapsing header with the specified label and flags.
	 * @param label The text to display on the header.
	 * @param flags ImGuiTreeNodeFlags to customize the header behavior.
	 * @return True if the header is open, false otherwise.
	 */
	bool CollapsingHeader(const char *label, ImGuiTreeNodeFlags flags = 0);

	/**
	 * @brief Creates a button with the specified label and size.
	 * @param label The text to display on the button.
	 * @param size The size of the button.
	 * @return True if the button is clicked, false otherwise.
	 */
	bool Button(const char *label, const ImVec2 &size = ImVec2(0, 0));

	/**
	 * @brief Creates a checkbox with the specified label and value.
	 * @param label The text to display next to the checkbox.
	 * @param value Pointer to the boolean value to be modified by the checkbox.
	 * @return True if the checkbox value was changed, false otherwise.
	 */
	bool CheckBox(const char *label, bool *value);

	/**
	 * @brief Creates an image with the specified icon, size, and tint color.
	 * @param icon The icon type to display.
	 * @param size The size of the icon.
	 * @param tint  Tint color for the icon.
	 */
	void Image(const IconType icon, const float size, const ImVec4 tint);

	/**
	 * @brief Creates an image slot with the specified texture and setter function.
	 * @param texture_in Pointer to the image resource.
	 * @param setter Function to set the image resource.
	 * @return True if the user clicked on the slot, false otherwise.
	 */
	bool ImageSlot(ImageResource *texture_in, const std::function<void(ImageResource *)> &setter);

	/**
	 * @brief Displays a tooltip with the specified text.
	 * @param text The text to display in the tooltip.
	 */
	void Tooltip(const char *text);

	/**
	 * @brief Creates a draggable float input that wraps the mouse cursor around the edges of the screen.
	 * @param label The label for the draggable float input.
	 * @param v Pointer to the float value to be modified.
	 * @param v_speed The speed at which the value changes when dragged.
	 * @param v_min The minimum value for the float.
	 * @param v_max The maximum value for the float.
	 * @param format The format string for displaying the float value.
	 * @param flags ImGuiSliderFlags to customize the behavior of the draggable float input.
	 * @return True if the float value was changed, false otherwise.
	 */
	bool DrawFloatWrap(const char *label, float *v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char *format = "%.3f", const ImGuiSliderFlags flags = 0);

	/**
	 * @brief Creates a combo box with the specified label and options.
	 * @param label The label for the combo box.
	 * @param options The list of options to display in the combo box.
	 * @param selectionIndex Pointer to the index of the currently selected option.
	 * @return True if the selected option was changed, false otherwise.
	 */
	bool ComboBox(const char *label, const std::vector<std::string> &options, uint32_t *selectionIndex);

	/**
	 * @brief Creates a vector input with the specified label and vector.
	 * @param label The label for the vector input.
	 * @param vector The vector to be modified.
	 * @param vertical True to display the vector components vertically, false for horizontal.
	 */
	void Vec3(const char *label, Vec3 &vector, bool vertical = true);

	/* @brief Initialize custom ImGui extensions */
	void InitImGuiExtensions();

}

// -------------------------------------------------------
