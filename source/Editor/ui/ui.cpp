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
#include <SceneryEditorX/ui/ui.h>
#include "actions/drag_drop.h"
#include <Editor/ui/source/imgui/imconfig.h>
#include <Editor/ui/source/imgui/imgui.h>
#include <Editor/ui/source/imgui/imgui_internal.h>
#include <Editor/ui/source/imgui/backends/imgui_impl_sdl3.h>
#include <Editor/ui/source/imgui/backends/imgui_impl_vulkan.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/core/resource/resource_cache.h>
#include <SceneryEditorX/core/window/monitor_data.h>
#include <SceneryEditorX/core/window/window.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/vulkan/device.h>
#include <SceneryEditorX/renderer/vulkan/render_context.h>

// -------------------------------------------------------

using namespace SceneryEditorX;

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
bool GUI::visible = true;
const std::string GUI::DEFAULT_FONT = "Roboto-Regular";

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
			reinterpret_cast<ImTextureID>(texture), // user_texture_id
			ImVec2(size.x, size.y),                                   // size
			ImVec2(0, 0),                      // uv0
			ImVec2(1, 1),                      // uv1
			ImColor(0, 0, 0, 0),          // bg_col
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
			reinterpret_cast<ImTextureID>(texture),
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
			reinterpret_cast<ImTextureID>(texture),
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
			reinterpret_cast<ImTextureID>(ResourceCache::GetIcon(icon)),
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
			reinterpret_cast<ImTextureID>(ResourceCache::GetIcon(icon)),
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
				SEDX_CORE_ERROR_TAG("UI", "%s", e.what());
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

GUI::GUI() = default;

GUI::~GUI()
{
	CleanUp();
}

bool GUI::CreateDescriptorPool()
{
	// Create separate descriptor pool for ImGui with FREE_DESCRIPTOR_SET_BIT
	const VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 100},
										{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100},
										{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100},
										{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100},
										{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100},
										{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100},
										{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
										{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
										{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100},
										{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100},
										{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100}};

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1000; /// Increased from 100 to handle more UI elements
	poolInfo.poolSizeCount = std::size(poolSizes);
	poolInfo.pPoolSizes = poolSizes;

	if (vkCreateDescriptorPool(m_Device->GetDevice(), &poolInfo, nullptr /* context.allocatorCallback*/, &imguiPool) != VK_SUCCESS)
	{
		EDITOR_ERROR("Failed to create ImGui descriptor pool!");
		return false;
	}

	return true;
}

void GUI::UpdateDpiScale()
{
	auto &app = Application::Get();
	Window window = app.GetWindow();

	// Get monitor DPI info if available
	/*
	if (SDL_Display *monitor = SDL_GetPrimaryMonitor())
	{
		float xDpi, yDpi;
		SDL_GetMonitorContentScale(monitor, &xDpi, &yDpi);
		dpiFactor = xDpi;
	}
	else
	{
		dpiFactor = xScale;
	}
	*/

	// Tick ImGui style to reflect DPI changes
	ImGuiStyle &style = ImGui::GetStyle();
	style.ScaleAllSizes(Window::GetDpiScale());
	
	EDITOR_INFO("Updated DPI scale: {}", dpiFactor);
}

bool GUI::InitGUI()
{
	Ref<Device> device = RenderContext::Get()->GetDevice();
	if (initialized)
	{
		EDITOR_WARN("GUI already initialized");
		return true;
	}

	if (!m_Device)
	{
		EDITOR_ERROR("Failed to get valid Vulkan device");
		return false;
	}

	// Create descriptor pool
	if (!CreateDescriptorPool())
		return false;

	// Initialize ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();

	// Configure ImGui features
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   /// Enable docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; /// Enable multi-viewport
	io.ConfigDockingWithShift = false;                  /// Don't require shift for docking
	io.ConfigWindowsResizeFromEdges = true;             /// Enable resizing windows from edges
	io.ConfigWindowsMoveFromTitleBarOnly = false;       /// Allow moving windows from anywhere

	// Initialize SDL3 backend
	ImGui_ImplSDL3_InitForVulkan(Window::GetWindow());

	/*
	// Get queue family info
	RenderData renderData;*/

	// Initialize Vulkan backend
	ImGui_ImplVulkan_InitInfo info{};
	info.Instance = RenderContext::GetInstance();
	info.PhysicalDevice = RenderContext::Get()->GetDevice()->GetPhysicalDevice();
	info.QueueFamily = m_Device->GetQueueManager()->GetFamilyIndexByType(QueueType::Graphics);
	info.Queue = m_Device->GetQueueManager()->GetQueueHandleByType(QueueType::Graphics);
	info.DescriptorPool = imguiPool;
	//info.RenderPass = renderer.GetRenderPass();
	info.MinImageCount = 2;
	//info.ImageCount = renderData.imageIndex;
	info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT; // Use MSAA samples from renderer later
	info.Allocator = nullptr;
	info.CheckVkResultFn = [](const VkResult result)
	{
		if (result != VK_SUCCESS)
			EDITOR_ERROR("ImGui Vulkan Error: {}", static_cast<int>(result));
	};

	// Initialize Vulkan implementation
	if (!ImGui_ImplVulkan_Init(&info))
	{
		EDITOR_ERROR("Failed to initialize ImGui Vulkan implementation");
		return false;
	}

	// Upload fonts to GPU

	//VkCommandBuffer commandBuffer = renderer.BeginSingleTimeCommands();
	//renderer.EndSingleTimeCommands(commandBuffer);

	// Wait for font upload to complete
	//vkDeviceWaitIdle(m_Device->GetDevice());
	//Renderer::WaitAndRender();

	// Font upload objects are now handled by ImGui internally

	// Set ImGui style and fonts
	SetStyle();
	SetFonts();

	// Tick DPI scale
	UpdateDpiScale();

	initialized = true;
	EDITOR_INFO("ImGui initialized successfully");
	return true;
}

void GUI::BeginFrame() const
{
	if (!initialized || !visible)
		return;

	// Start the ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
}

void GUI::EndFrame() const
{
	if (!initialized || !visible)
		return;

	// Render the ImGui frame
	ImGui::Render();

	if (activeCommandBuffer != VK_NULL_HANDLE)
	{
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), activeCommandBuffer);
	}

	// Tick and render additional platform windows
	ImGuiIO &io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
}

void GUI::CleanUp()
{
	if (!initialized)
		return;

	vkDeviceWaitIdle(m_Device->GetDevice());
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL3_Shutdown();

	/*
	if (imguiPool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(m_Device->GetDevice(), imguiPool, nullptr);
		imguiPool = VK_NULL_HANDLE;
	}
	*/

	ImGui::DestroyContext();
	initialized = false;

	EDITOR_INFO("ImGui resources cleaned up");
}

void GUI::Resize(uint32_t width, uint32_t height)
{
	if (!initialized)
		return;

	// Tick DPI scale if needed
	UpdateDpiScale();

	EDITOR_INFO("GUI resized to {}x{}", width, height);
}

void GUI::Update(float deltaTime) const
{
	if (!initialized || !visible)
		return;

}

void GUI::ShowDemoWindow(bool *open) const
{
	if (!initialized || !visible)
		return;

	ImGui::ShowDemoWindow(open);
}

/*
void GUI::InitGUI(SDL_Window *window, SceneryEditorX::GraphicsEngine &renderer)
{
	// Store the engine reference
	this->renderer = &renderer;
	this->window = window;

	// Create separate descriptor pool for ImGui with FREE_DESCRIPTOR_SET_BIT
	VkDescriptorPoolSize pool_sizes[] = {
		{VK_DESCRIPTOR_TYPE_SAMPLER, 100},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100},
		{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100},
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100},
		{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100},
		{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100},
		{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100}
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 100;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	if (vkCreateDescriptorPool(device->GetDevice(), &pool_info, nullptr, &imguiPool) != VK_SUCCESS)
	{
		EDITOR_ERROR("Failed to create ImGui descriptor pool!");
		return;
	}

	SceneryEditorX::QueueFamilyIndices indices = device->GetDevice()->GetQueueFamilyIndices();

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable multi-viewport / platform windows
	io.ConfigDockingWithShift = false;                     // Don't require shift for docking
	io.ConfigWindowsResizeFromEdges = true;                // Enable resizing windows from edges

	ImGui_ImplGlfw_InitForVulkan(Window::GetWindow(), true);

	ImGui_ImplVulkan_InitInfo info{};
	info.ApiVersion = VK_API_VERSION_1_3;
	info.Instance = SceneryEditorX::GraphicsEngine::GetInstance();
	info.PhysicalDevice = device->GetDevice()->GetGPUDevice();
	info.Device = device->GetDevice();
	info.QueueFamily = indices.graphicsFamily.value();
	info.Queue = device->GetGraphicsQueue();
	info.DescriptorPool = imguiPool;
	info.RenderPass = renderer->GetRenderPass();
	//info.Subpass = 0;
	info.MinImageCount = swapchain->GetSwapChainImages().size();
	info.ImageCount = swapchain->GetSwapChainImages().size();
	info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;  /* TODO: Replace when MSAA is implemented properly. #1#
	//info.MSAASamples = renderer->msaaSamples;
	//info.Allocator = nullptr;
	info.UseDynamicRendering = false;
	info.CheckVkResultFn = [](const VkResult result)
	{
		if (result != VK_SUCCESS)
		{
			EDITOR_ERROR("ImGui Vulkan Error: {}", ToString(result));
		}
	};

	ImGui_ImplVulkan_Init(&info);

	// Upload fonts to GPU
	VkCommandBuffer commandBuffer = renderer->beginSingleTimeCommands();
	ImGui_ImplVulkan_CreateFontsTexture(); // Removed Nov 10, 2023 Commit #79a9e2f
	renderer->endSingleTimeCommands(commandBuffer);

	vkDeviceWaitIdle(device->GetDevice());// Wait for font upload to complete
	//ImGui_ImplVulkan_DestroyFontUploadObjects(); // This is no longer needed in newer versions of ImGui

	SetStyle();

	initialized = true;
	EDITOR_INFO("ImGui initialized successfully");
}
*/

/*
void GUI::ShowAppInfo(const std::string &appName) const
{
	if (!initialized || !visible)
		return;

	ImGui::Begin("Application Info", nullptr, commonFlags | infoFlags);

	ImGui::Text("Application: %s", appName.c_str());
	ImGui::Text("Renderer: %s", "X-Plane 12 Graphics Emulator");
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);

	/// Add system info
	ImGui::Separator();
	ImGui::Text("Window Size: %d x %d",
				static_cast<int>(ImGui::GetIO().DisplaySize.x),
				static_cast<int>(ImGui::GetIO().DisplaySize.y));
	ImGui::Text("DPI Scale: %.2f", dpiFactor);

	ImGui::End();
}
*/

/*
bool GUI::InitViewport(const Viewport &size, VkImageView imageView)
{
	if (!initialized)
		return false;

	viewportInitialized = true;
	return true;
}
*/

/*
void GUI::ViewportWindow(Viewport &size, bool &hovered, VkImageView imageView)
{
	if (!initialized || !visible || !viewportInitialized)
		return;

	/// Start viewport window with dockable behavior
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	/// Get content region size
	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	size = {viewportSize.x, viewportSize.y};
	hovered = ImGui::IsWindowHovered();

	/// Display the image view as a texture
	if (imageView != VK_NULL_HANDLE)
	{
		ImTextureID texID = GetTextureID(imageView, VK_NULL_HANDLE);
		ImGui::Image(texID, viewportSize);
	}

	ImGui::End();
	ImGui::PopStyleVar();
}
*/

/*
ImTextureID GUI::GetTextureID(const VkImageView imageView, VkSampler sampler, const VkImageLayout layout) const
{
	auto device = RenderContext::GetCurrentDevice();
	if (!initialized || imageView == VK_NULL_HANDLE)
		return reinterpret_cast<ImTextureID>(nullptr);

	/// Check if device is valid
	if (device == nullptr)
	{
		EDITOR_ERROR("Cannot get texture ID: device is null");
		return reinterpret_cast<ImTextureID>(nullptr);
	}

	/// Get a sampler if none was provided
	VkSampler actualSampler = sampler;
	if (actualSampler == VK_NULL_HANDLE)
	{
		/// If we have a valid device, use its sampler
		actualSampler = device->GetSampler();

		/// If we still don't have a valid sampler, we can't proceed
		if (actualSampler == VK_NULL_HANDLE)
		{
			EDITOR_ERROR("Cannot get texture ID: no valid sampler available");
			return reinterpret_cast<ImTextureID>(nullptr);
		}
	}

	/// Now we have a valid sampler and imageView
	/// ImGui_ImplVulkan_AddTexture returns VkDescriptorSet which needs to be cast to ImTextureID
	VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(actualSampler, imageView, layout);
	return reinterpret_cast<ImTextureID>(descriptorSet);
}
*/

void GUI::SetStyle()
{
	constexpr auto ColorFromBytes = [](uint8_t r, uint8_t g, uint8_t b)
	{
		return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f);
	};

	auto &style = ImGui::GetStyle();
	ImVec4 *colors = style.Colors;

	constexpr ImVec4 bgColor = ColorFromBytes(37, 37, 38);
	constexpr ImVec4 lightBgColor = ColorFromBytes(82, 82, 85);
	constexpr ImVec4 veryLightBgColor = ColorFromBytes(90, 90, 95);

	constexpr ImVec4 panelColor = ColorFromBytes(51, 51, 55);
	constexpr ImVec4 panelHoverColor = ColorFromBytes(29, 151, 236);
	constexpr ImVec4 panelActiveColor = ColorFromBytes(0, 119, 200);

	constexpr ImVec4 textColor = ColorFromBytes(255, 255, 255);
	constexpr ImVec4 textDisabledColor = ColorFromBytes(151, 151, 151);
	constexpr ImVec4 borderColor = ColorFromBytes(78, 78, 78);

	colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.95f); // More opaque
	colors[ImGuiCol_Text] = textColor;
	colors[ImGuiCol_TextDisabled] = textDisabledColor;
	colors[ImGuiCol_TextSelectedBg] = panelActiveColor;
	colors[ImGuiCol_ChildBg] = bgColor;
	colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.98f); // More opaque
	colors[ImGuiCol_Border] = borderColor;
	colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.25f);
	colors[ImGuiCol_FrameBg] = panelColor;
	colors[ImGuiCol_FrameBgHovered] = panelHoverColor;
	colors[ImGuiCol_FrameBgActive] = panelActiveColor;
	colors[ImGuiCol_TitleBg] = bgColor;
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.15f, 0.75f);
	colors[ImGuiCol_MenuBarBg] = panelColor;
	colors[ImGuiCol_ScrollbarBg] = panelColor;
	colors[ImGuiCol_ScrollbarGrab] = lightBgColor;
	colors[ImGuiCol_ScrollbarGrabHovered] = veryLightBgColor;
	colors[ImGuiCol_ScrollbarGrabActive] = veryLightBgColor;
	colors[ImGuiCol_CheckMark] = ColorFromBytes(0, 213, 255); // Brighter check mark
	colors[ImGuiCol_SliderGrab] = panelHoverColor;
	colors[ImGuiCol_SliderGrabActive] = panelActiveColor;
	colors[ImGuiCol_Button] = panelColor;
	colors[ImGuiCol_ButtonHovered] = panelHoverColor;
	colors[ImGuiCol_ButtonActive] = panelActiveColor;
	colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	colors[ImGuiCol_HeaderHovered] = panelHoverColor;
	colors[ImGuiCol_HeaderActive] = panelActiveColor;
	colors[ImGuiCol_Separator] = borderColor;
	colors[ImGuiCol_SeparatorHovered] = ColorFromBytes(95, 95, 95);
	colors[ImGuiCol_SeparatorActive] = ColorFromBytes(120, 120, 120);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.2f, 0.2f, 0.2f, 0.5f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.3f, 0.3f, 0.3f, 0.75f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	colors[ImGuiCol_PlotLines] = panelActiveColor;
	colors[ImGuiCol_PlotLinesHovered] = panelHoverColor;
	colors[ImGuiCol_PlotHistogram] = panelActiveColor;
	colors[ImGuiCol_PlotHistogramHovered] = panelHoverColor;
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.0f, 0.5f, 1.0f, 0.9f);
	colors[ImGuiCol_NavHighlight] = panelActiveColor;
	colors[ImGuiCol_DockingPreview] = ImVec4(0.0f, 0.5f, 1.0f, 0.7f);
	colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 0.9f);
	colors[ImGuiCol_TabActive] = panelActiveColor;
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.15f, 0.15f, 0.8f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	colors[ImGuiCol_TabHovered] = panelHoverColor;

	// Improved style settings
	style.WindowRounding = 2.0f;
	style.ChildRounding = 2.0f;
	style.FrameRounding = 2.0f;
	style.GrabRounding = 2.0f;
	style.PopupRounding = 2.0f;
	style.ScrollbarRounding = 2.0f;
	style.TabRounding = 2.0f;

	// Better sizing and spacing
	style.WindowPadding = ImVec2(10, 10);
	style.WindowBorderSize = 1.0f;
	style.FramePadding = ImVec2(8, 4);
	style.ItemSpacing = ImVec2(10, 4);
	style.ItemInnerSpacing = ImVec2(4, 4);

	// Improve usability
	style.TouchExtraPadding = ImVec2(0, 0);
	style.IndentSpacing = 21.0f;
	style.ScrollbarSize = 14.0f;
	style.GrabMinSize = 10.0f;
}

void GUI::SetFonts() const
{
	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->Clear(); // Clear existing fonts

	// Default font size based on DPI
	const float fontSize = 16.0f * dpiFactor;

	// Add default font
	io.Fonts->AddFontDefault();

	// Add roboto font if available
	// (path would need to be determined based on your asset system)
	std::string fontPath = "assets/fonts/Roboto-Regular.ttf";

	// Example: Add font with icon merging
	// Configure icon font
	static constexpr ImWchar ICON_RANGES[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
	ImFontConfig iconConfig;
	iconConfig.MergeMode = true;
	iconConfig.PixelSnapH = true;
	iconConfig.GlyphMinAdvanceX = fontSize;

	// Standard font
	ImFontConfig fontConfig;
	fontConfig.SizePixels = fontSize;

	ImFont *mainFont = nullptr; // Default to using a built-in font if loading fails

	// Try to load custom font
	// mainFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, &fontConfig);

	// If not available, use default
	if (!mainFont)
	{
		mainFont = io.Fonts->AddFontDefault();
	}

	// Add icons to the font
	// io.Fonts->AddFontFromMemoryCompressedTTF(
	//     FontAwesome_compressed_data,
	//     FontAwesome_compressed_size,
	//     fontSize * 0.8f,
	//     &iconConfig,
	//     iconRanges);

	io.FontDefault = mainFont; // Set default font
	io.Fonts->Build();	// Build font atlas
}

// -------------------------------------------------------
