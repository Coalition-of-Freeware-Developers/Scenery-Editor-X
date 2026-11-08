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
 * ui.cpp
 * -------------------------------------------------------
 * Created: 25/3/2025
 * -------------------------------------------------------
 */
#include "ui.h"
#include "ui_impl.h"
#include "actions/drag_drop.h"
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/window/monitor_data.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imconfig.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui.h>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui_internal.h>
#include <SceneryEditorX/renderer/ui/source/imgui/backends/imgui_impl_vulkan.h>
#include <SceneryEditorX/renderer/vulkan/device.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	// Implementation of missing ImGui functions to fix linker errors
	// Note: This is a compatibility layer to ensure ImGui works correctly
	
	// This implementation may need to be adjusted based on your ImGui version
	// It provides stubs for functionality that might be missing in your current build
	extern "C"
	{
		// Implement missing ImGui functions referenced in the codebase
		IMGUI_API bool ImGui_BeginTable(const char *str_id,
										int columns,
										ImGuiTableFlags flags,
										const ImVec2 &outer_size,
										float inner_width)
		{
			// Simplified implementation - if you're not using tables extensively
			// Just return false or implement a basic version
			return false; // Or implement based on your needs
		}
	
		// Implementation for GetStyle
		IMGUI_API ImGuiStyle &ImGui_GetStyle(void)
		{
			static ImGuiStyle style;
			return style;
		}
	
		// Implementation for ColorConvertU32ToFloat4
		IMGUI_API ImVec4 ImGui_ColorConvertU32ToFloat4(unsigned int color)
		{
			// Convert RGBA color to ImVec4
			float r = (color >> 0 & 0xFF) / 255.0f;
			float g = (color >> 8 & 0xFF) / 255.0f;
			float b = (color >> 16 & 0xFF) / 255.0f;
			float a = (color >> 24 & 0xFF) / 255.0f;
			return {r, g, b, a};
		}
	}
	
	// Initialize static members
	//bool GUI::visible = true;
	//const std::string GUI::DEFAULT_FONT = "Roboto-Regular";
	
	// Additional ImGui initialization functions can be placed here if needed
	void InitImGuiExtensions()
	{
		// This function can be called from main ImGui setup to initialize any extensions
		// Currently empty, but could be expanded if more ImGui features need integration
	}
	
	namespace UI
	{
		// Collapsing header
		bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
			bool result = ImGui::CollapsingHeader(label, flags);
			ImGui::PopStyleVar();
			return result;
		}
	
		// Button
		bool Button(const char* label, const ImVec2& size)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
			// use label as the id - cursor position was causing id changes between
			// frames due to floating point precision
			bool result = ImGui::Button(label, size);
			ImGui::PopStyleVar();
			return result;
		}
	
		bool CheckBox(const char *label, bool *value)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
			bool result = ImGui::Checkbox(label, value);
			ImGui::PopStyleVar();
			return result;
		}
	
		bool ButtonCenteredOnLine(const char* label, float alignment)
		{
			ImGuiStyle& style = ImGui::GetStyle();
	
			float size  = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
			float avail = ImGui::GetContentRegionAvail().x;
	
			float off = (avail - size) * alignment;
			if (off > 0.0f)
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
			}
	
			return ImGui::Button(label);
		}
	
		bool ImageButton(ImageResource* texture, const xMath::Vec2& size, bool border, ImVec4 tint)
		{
			if (!border)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
			}
	
			// use the texture pointer as a stable id - cursor position was causing
			// id changes between frames due to floating point precision, which caused
			// clicks to not register properly (requiring multiple clicks)
			ImGui::PushID(texture);
			bool result = ImGui::ImageButton
			(
				"",                                     // str_id
				GetTextureID(texture),                  // user_texture_id
				ImVec2(size.x, size.y),                 // size
				ImVec2(0, 0),                           // uv0
				ImVec2(1, 1),                           // uv1
				ImColor(0, 0, 0, 0),                    // bg_col
				tint                                    // tint_col
			);
			ImGui::PopID();
	
			if (!border)
			{
				ImGui::PopStyleVar();
			}
	
			return result;
		}
	
		void Image(ImageResource* texture, const xMath::Vec2& size, bool border)
		{
			if (!border)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
			}
	
			ImGui::Image(
				GetTextureID(texture),
				ImVec2(size.x, size.y),
				ImVec2(0, 0),
				ImVec2(1, 1),
				DEFAULT_TINT,       // tint
				ImColor(0, 0, 0, 0) // border
			);
	
			if (!border)
			{
				ImGui::PopStyleVar();
			}
		}
	
		void Image(ImageResource* texture, const ImVec2& size, const ImVec4& tint, const ImColor& border)
		{
			ImGui::Image(
				GetTextureID(texture),
				size,
				ImVec2(0, 0),
				ImVec2(1, 1),
				tint,
				border
			);
		}
	
		void Image(const IconType icon, const float size)
		{
			ImGui::Image(
				GetTextureID(ResourceCache::GetIcon(icon)),
				ImVec2(size, size),
				ImVec2(0, 0),
				ImVec2(1, 1),
				DEFAULT_TINT,       // tint
				ImColor(0, 0, 0, 0) // border
			);
		}
	
		void Image(const IconType icon, const float size,const ImVec4 tint)
		{
			ImGui::Image(
				GetTextureID(ResourceCache::GetIcon(icon)),
				ImVec2(size, size),
				ImVec2(0, 0),
				ImVec2(1, 1),
				tint,       // tint
				ImColor(0, 0, 0, 0) // border
			);
		}
	
		bool ImageSlot(ImageResource *texture_in, const std::function<void(ImageResource *)> &setter)
		{
			const ImVec2 slot_size  = ImVec2(80 * Window::GetDpiScale(), 80 * Window::GetDpiScale());
			const float button_size = 15.0f * Window::GetDpiScale();
			bool clicked_for_browse = false;
	
			ImGui::BeginGroup();
			{
				ImageResource* texture   = texture_in;
				const ImVec2 pos_image          = {ImGui::GetCursorPos().x, ImGui::GetCursorPos().y};
				const ImVec2 screen_pos         = {ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y};
	
				// x button position (top-right corner)
				const float x_btn_offset_x = slot_size.x - button_size - 4.0f;
				const float x_btn_offset_y = 4.0f;
				ImVec2 x_btn_screen_min    = ImVec2(screen_pos.x + x_btn_offset_x, screen_pos.y + x_btn_offset_y);
				ImVec2 x_btn_screen_max    = ImVec2(x_btn_screen_min.x + button_size, x_btn_screen_min.y + button_size);
	
				// check x button click FIRST using manual hit test
				bool x_clicked = false;
				if (texture != nullptr)
				{
					bool x_hovered = ImGui::IsMouseHoveringRect(x_btn_screen_min, x_btn_screen_max);
					if (x_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					{
						setter(nullptr);
						x_clicked = true;
					}
				}
	
				// main slot interaction (only if x wasn't clicked)
				ImGui::SetCursorPos(pos_image);
				ImGui::InvisibleButton("##slot_click", slot_size);
				bool is_hovered = ImGui::IsItemHovered();
				
				if (!x_clicked && ImGui::IsItemClicked(ImGuiMouseButton_Left))
				{
					clicked_for_browse = true;
				}
	
				// draw the image
				ImVec4 color_tint   = (texture != nullptr) ? ImVec4(1, 1, 1, 1) : ImVec4(0, 0, 0, 0);
				ImVec4 color_border = is_hovered ? ImVec4(0.4f, 0.6f, 1.0f, 1.0f) : ImVec4(1, 1, 1, 0.5f);
				ImGui::SetCursorPos(pos_image);
				Image(texture, slot_size, color_tint, color_border);
	
				// drag source
				if (texture != nullptr && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
				{
					ImGui::EndDragDropSource();
				}
	
				// draw x button (visual only - click handled above)
				if (texture != nullptr)
				{
					ImVec2 pos_button = ImVec2(pos_image.x + x_btn_offset_x, pos_image.y + x_btn_offset_y);
					ImGui::SetCursorPos(pos_button);
					
					// draw button background on hover
					bool x_hovered = ImGui::IsMouseHoveringRect(x_btn_screen_min, x_btn_screen_max);
					if (x_hovered)
					{
						ImGui::GetWindowDrawList()->AddRectFilled(x_btn_screen_min, x_btn_screen_max, IM_COL32(255, 80, 80, 180), 3.0f);
					}
					
					// draw x icon
					Image(ResourceCache::GetIcon(IconType::Close), ImVec2(button_size, button_size));
				}
			}
			ImGui::EndGroup();
	
			// drop target
			if (auto payload = DragDropPayload::ReceiveDragDropPayload(DragPayloadType::Texture))
			{
				try
				{
					if (const auto tex = ResourceCache::Load<ImageResource>(std::get<const char*>(payload->GetData())).Get())
					{
						setter(tex);
					}
				}
				catch (const std::bad_variant_access& e)
				{
					EDITOR_ERROR_TAG("UI", "%s", e.what());
				}
			}
	
			return clicked_for_browse;
		}
	
		void Tooltip(const char* text)
		{
			SEDX_CORE_ASSERT(text != nullptr, "Text is null");
	
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text(text);
				ImGui::EndTooltip();
			}
		}
	
		// a drag float which will wrap the mouse cursor around the edges of the screen
		bool DrawFloatWrap(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, const ImGuiSliderFlags flags)
		{
			static const uint32_t SCREEN_EDGE_PADDING = 10;
			ImGuiIO& io = ImGui::GetIO();
	
			static ImVec2 lastMousePos = io.MousePos;
	
			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
			{
				ImVec2 mouse_pos = io.MousePos;
				bool wrapped = false;
	
				float left  = static_cast<float>(SCREEN_EDGE_PADDING);
				float right = static_cast<float>(MonitorData::GetWidth() - SCREEN_EDGE_PADDING);
	
				if (mouse_pos.x >= right)
				{
					mouse_pos.x = left + 1;
					wrapped = true;
				}
				else if (mouse_pos.x <= left)
				{
					mouse_pos.x = right - 1;
					wrapped = true;
				}
	
				if (wrapped)
				{
					io.MousePos        = mouse_pos;
					io.WantSetMousePos = true;
					io.MouseDelta.x    = 0.0f;
					io.MouseDelta.y    = 0.0f;
	
					// update last_mouse_pos to avoid delta spikes in the next frame
					lastMousePos = mouse_pos;
				}
				else
				{
					// update last position normally
					lastMousePos = mouse_pos;
				}
			}
	
			ImGui::PushID(static_cast<int>(ImGui::GetCursorPosX() + ImGui::GetCursorPosY()));
			bool changed = ImGui::DragFloat(label, v, v_speed, v_min, v_max, format, flags);
			ImGui::PopID();
	
			return changed;
		}
	
		bool ComboBox(const char *label, const std::vector<std::string> &options, uint32_t *selectionIndex)
		{
			const uint32_t optionCount = static_cast<uint32_t>(options.size());
	
			// clamp index
			if (*selectionIndex >= optionCount)
			{
				*selectionIndex = optionCount ? optionCount - 1 : 0;
			}
	
			bool selectionMade = false;
	
			// preview: direct pointer into existing string buffer
			const char* preview = optionCount ? options[*selectionIndex].data() : "";
	
			if (ImGui::BeginCombo(label, preview))
			{
				for (uint32_t i = 0; i < optionCount; ++i)
				{
					const bool is_selected = (*selectionIndex == i);
					// direct data() — null-terminated, no copy
					if (ImGui::Selectable(options[i].data(), is_selected))
					{
						*selectionIndex = i;
						selectionMade     = true;
					}
					if (is_selected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			return selectionMade;
		}
		
		void Vec3(const char *label, xMath::Vec3 &vector, bool vertical)
		{
			// configuration
			const float label_indent = 15.0f * Window::GetDpiScale();
			const float axis_spacing = 15.0f * Window::GetDpiScale();
			const float step         = 0.01f;
		
			ImGui::PushID(label);
			ImGui::BeginGroup();
		
			// label
			ImGui::Indent(label_indent);
			ImGui::TextUnformatted(label);
			ImGui::Unindent(label_indent);
		
			// layout calculation
			float item_width = 128.0f;
			if (!vertical)
			{
				float avail_x       = ImGui::GetContentRegionAvail().x;
				float spacing       = ImGui::GetStyle().ItemSpacing.x;
				float total_spacing = spacing * 2.0f;
				item_width          = (avail_x - total_spacing) / 3.0f;
				item_width          -= axis_spacing;
		
				if (item_width < 1.0f)
					item_width = 1.0f;
			}
		
			float* values[3]           = { &vector.x, &vector.y, &vector.z };
			const char* axis_labels[3] = { "X", "Y", "Z" };
			const ImU32 axis_colors[3] = {
				IM_COL32(168, 46, 2, 255),
				IM_COL32(112, 162, 22, 255),
				IM_COL32(51, 122, 210, 255)
			};
		
			// components
			for (int i = 0; i < 3; ++i)
			{
				ImGui::PushID(i);
		
				// horizontal layout
				if (!vertical && i > 0)
				{
					ImGui::SameLine();
				}
		
				// axis label
				ImGui::TextUnformatted(axis_labels[i]);
				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + axis_spacing - ImGui::CalcTextSize(axis_labels[i]).x);
				xMath::Vec2 pos_post_label = {ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y};
		
				// float input
				ImGui::PushItemWidth(item_width);
				DrawFloatWrap("##v", values[i], step, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max(), "%.4f");
				ImGui::PopItemWidth();
		
				// color bar decoration
				static const xMath::Vec2 size   = xMath::Vec2(4.0f, 19.0f);
				static const xMath::Vec2 offset = xMath::Vec2(-7.0f, 4.0f);
				xMath::Vec2 draw_pos            = pos_post_label + offset;
				ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(draw_pos.x, draw_pos.y), ImVec2(draw_pos.x + size.x, draw_pos.y + size.y), axis_colors[i]);
		
				ImGui::PopID();
			}
		
			ImGui::EndGroup();
			ImGui::PopID();
		}
	
	}

}	


// -------------------------------------------------------
