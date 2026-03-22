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
 * ui_impl.h
 * -------------------------------------------------------
 * Created: 15/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <vk_mem_alloc.h>
#include <Editor/ui/source/imgui/imgui.h>
#include <SceneryEditorX/renderer/vulkan/push_constant_buffer.h>
#include <SceneryEditorX/utils/pointers.h>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class ImageResource;
	class Buffer;
	class DepthStencilState;
	class RasterizerState;
	class BlendState;
	class Shader;
	class Swapchain;
	class CommandList;
}

namespace UI
{
	const uint32_t BUFFER_COUNT = 8;

	/**
	 * @struct ViewportResources
	 * @brief Per-window ring-buffered vertex/index buffers and push constants for ImGui rendering.
	 */
	struct ViewportResources
	{
		std::array<SceneryEditorX::Scope<SceneryEditorX::Buffer>, BUFFER_COUNT> index_buffers;
		std::array<SceneryEditorX::Scope<SceneryEditorX::Buffer>, BUFFER_COUNT> vertex_buffers;
		std::array<uint32_t, BUFFER_COUNT> index_counts = {};
		std::array<uint32_t, BUFFER_COUNT> vertex_counts = {};
		SceneryEditorX::PushConstantBuffer_Pass pushConstantBuffer_Pass;
		uint32_t bufferIndex = 0;

		ViewportResources() = default;

		explicit ViewportResources(const char* name)
		{
			VmaAllocationCreateInfo allocCI{};
			allocCI.usage = VMA_MEMORY_USAGE_AUTO;
			allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
							VMA_ALLOCATION_CREATE_MAPPED_BIT;

			for (uint32_t i = 0; i < BUFFER_COUNT; i++)
			{
				vertex_counts[i] = 50000;
				index_counts[i]  = 100000;

				vertex_buffers[i] = SceneryEditorX::CreateScope<SceneryEditorX::Buffer>(
					static_cast<VmaAllocator>(nullptr),
					static_cast<VkDeviceSize>(sizeof(ImDrawVert) * vertex_counts[i]),
					static_cast<VkBufferUsageFlags>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
					allocCI);

				index_buffers[i] = SceneryEditorX::CreateScope<SceneryEditorX::Buffer>(
					static_cast<VmaAllocator>(nullptr),
					static_cast<VkDeviceSize>(sizeof(ImDrawIdx) * index_counts[i]),
					static_cast<VkBufferUsageFlags>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
					allocCI);
			}
		}

		ViewportResources(const char* name, SceneryEditorX::Swapchain* /*swapchain*/) : ViewportResources(name) {}
	};

	/**
	 * @struct WindowData
	 * @brief Per-window data for ImGui rendering, including viewport resources, swapchain, and command list.
	 */
	struct WindowData
	{
		SceneryEditorX::Scope<ViewportResources> viewportResources;
		SceneryEditorX::Ref<SceneryEditorX::Swapchain> swapchain;
		SceneryEditorX::CommandList *cmdList = nullptr;
	};
	
	// main window resources
	extern ViewportResources g_ViewportData;

	// shared resources (between all windows)
	extern SceneryEditorX::Ref<SceneryEditorX::ImageResource>     g_FontAtlas;
	extern SceneryEditorX::Ref<SceneryEditorX::DepthStencilState> g_DepthStencil_State;
	extern SceneryEditorX::Ref<SceneryEditorX::RasterizerState>   g_Rasterizer_State;
	extern SceneryEditorX::Ref<SceneryEditorX::BlendState>        g_BlendState;
	extern SceneryEditorX::Ref<SceneryEditorX::Shader>            g_VertexShader;
	extern SceneryEditorX::Ref<SceneryEditorX::Shader>            g_FragmentShader;

	/* @brief Initializes the platform-specific interface for ImGui rendering. */
	void InitializePlatformInterface();

	/* @brief Destroys all allocated resources for ImGui rendering. */
	void DestroyResources();

	/* @brief Initializes the ImGui rendering system. */
	void Initialize();

	/* @brief Shuts down the ImGui rendering system. */
	void Shutdown();

	/**
	 * @brief Renders the ImGui draw data.
	 * @param drawData Pointer to the ImGui draw data.
	 * @param windowData Pointer to the window data, or nullptr for the main window.
	 * @param clear Whether to clear the screen before rendering.
	 */
	void Render(ImDrawData *drawData, WindowData *windowData = nullptr, const bool clear = true);

	/**
	 * @brief Creates a new window for the specified ImGui viewport.
	 * @param viewport Pointer to the ImGui viewport.
	 */
	void WindowCreate(ImGuiViewport *viewport);

	/**
	 * @brief Destroys the window for the specified ImGui viewport.
	 * @param viewport Pointer to the ImGui viewport.
	 */
	void WindowDestroy(ImGuiViewport *viewport);

	/**
	 * @brief Resizes the window for the specified ImGui viewport.
	 * @param viewport Pointer to the ImGui viewport.
	 * @param size New size for the window.
	 */
   void WindowResize(ImGuiViewport *viewport, const ImVec2 size);

	/**
	 * @brief Renders the ImGui draw data for the specified viewport.
	 * @param viewport Pointer to the ImGui viewport.
	 */
	void WindowRender(ImGuiViewport *viewport, void*);

	/**
	 * @brief Presents the rendered ImGui draw data for the specified viewport.
	 * @param viewport Pointer to the ImGui viewport.
	 */
	void WindowPresent(ImGuiViewport *viewport, void*);

}

// -------------------------------------------------------
