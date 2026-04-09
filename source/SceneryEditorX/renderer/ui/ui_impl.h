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
#include "source/imgui/imgui.h"
#include <vk_mem_alloc.h>
#include <SceneryEditorX/renderer/vulkan/buffer.h>
#include <SceneryEditorX/renderer/vulkan/push_constant_buffer.h>
#include <SceneryEditorX/renderer/vulkan/swapchain.h>
#include <SceneryEditorX/utils/pointers.h>
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX::UI
{
	const uint32_t BUFFER_COUNT = 8;

	/**
	 * @struct ViewportResources
	 * @brief Per-window ring-buffered vertex/index buffers and push constants for ImGui rendering.
	 */
	struct ViewportResources
	{
		std::array<Scope<Buffer>, BUFFER_COUNT> indexBuffers;
		std::array<Scope<Buffer>, BUFFER_COUNT> vertexBuffers;
		std::array<uint32_t, BUFFER_COUNT> indexCounts = {};
		std::array<uint32_t, BUFFER_COUNT> vertexCounts = {};
		PushConstantBuffer_Pass pushConstantBuffer_Pass;
		uint32_t bufferIndex = 0;
		Swapchain *swapchain;

		ViewportResources() = default;

		/**
		 * @brief Initializes the viewport resources by creating ring-buffered vertex and index buffers for ImGui rendering.
		 * @param name Name prefix for the buffers (used for debugging purposes).
		 * @param swapchain Pointer to the swapchain associated with the viewport, used for buffer creation and synchronization.
		 */
		explicit ViewportResources(const char* name, Swapchain *swapchain) : swapchain(swapchain)
		{
			VmaAllocationCreateInfo allocCI{};
			allocCI.usage = VMA_MEMORY_USAGE_AUTO;
			allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

			for (uint32_t i = 0; i < BUFFER_COUNT; i++)
			{
				vertexCounts[i] = 50000;
				indexCounts[i] = 100000;

				vertexBuffers[i] = SceneryEditorX::CreateScope<Buffer>(
					static_cast<VmaAllocator>(nullptr),
					static_cast<VkDeviceSize>(sizeof(ImDrawVert) * vertexCounts[i]),
					static_cast<VkBufferUsageFlags>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
					allocCI);

				indexBuffers[i] = SceneryEditorX::CreateScope<Buffer>(
					static_cast<VmaAllocator>(nullptr),
					static_cast<VkDeviceSize>(sizeof(ImDrawIdx) * indexCounts[i]),
					static_cast<VkBufferUsageFlags>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
					allocCI);
			}
		}

	};

	/**
	 * @struct WindowData
	 * @brief Per-window data for ImGui rendering, including viewport resources, swapchain, and command list.
	 */
	struct WindowData
	{
		Scope<ViewportResources> viewportResources;
		Ref<Swapchain> swapchain;
		CommandList *cmdList = nullptr;
	};
	
	/* Main window resources */
	extern ViewportResources g_ViewportData;

	/* Shared resources (between all windows) */
	extern Ref<ImageResource>     g_FontAtlas;
	extern Ref<DepthStencilState> g_DepthStencil_State;
	extern Ref<RasterizerState>   g_Rasterizer_State;
	extern Ref<BlendState>        g_BlendState;
	extern Ref<Shader>            g_VertexShader;
	extern Ref<Shader>            g_FragmentShader;

	/**
	 * @brief Raw Vulkan objects owned by the ImGui backend.
	 *        These provide the font-sampler descriptor set (Set 0, Binding 0)
	 *        and the pipeline layout that matches ui.slang's push constant layout
	 *        (scale: float2, translate: float2).
	 */
	extern VkDescriptorPool       g_ImGuiDescriptorPool;
	extern VkDescriptorSetLayout  g_ImGuiDescriptorSetLayout;
	extern VkDescriptorSet        g_ImGuiFontDescriptorSet;
	extern VkSampler              g_ImGuiFontSampler;
	extern VkPipelineLayout       g_ImGuiPipelineLayout;
	extern VkPipeline             g_ImGuiPipeline;

	/* @brief Initializes the platform-specific interface for ImGui rendering. */
	void InitializePlatformInterface();

	/* @brief Destroys all allocated resources for ImGui rendering. */
	void DestroyResources();

	/* @brief Initializes the ImGui rendering system. */
	void Initialize();

	/* @brief Shuts down the ImGui rendering system. */
	void Shutdown();

	/**
	 * @brief Checks if the specified window is focused.
	 * @param windowName Name of the window to check for focus. If nullptr, checks the currently focused window. 
	 * @param checkWindow Whether to check if the window is focused (true) or if any child window is focused (false).
	 * @return True if the window is focused, false otherwise.
	 */
	bool IsWindowFocused(const char *windowName, const bool checkWindow = true);

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
