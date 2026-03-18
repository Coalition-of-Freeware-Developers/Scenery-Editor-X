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
#include <imgui.h>
#include <SceneryEditorX/renderer/vulkan/push_constant_buffer.h>

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
	 * @brief 
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
		ViewportResources(const char* name)
		{

			// allocate buffers
			for (uint32_t i = 0; i < BUFFER_COUNT; i++)
			{
				vertex_counts[i]  = 50000;
				index_counts[i]   = 100000;
				vertex_buffers[i] = SceneryEditorX::CreateScope<SceneryEditorX::Buffer>(sizeof(ImDrawVert), vertex_counts[i], nullptr, true, name);
				index_buffers[i]  = SceneryEditorX::CreateScope<SceneryEditorX::Buffer>(sizeof(ImDrawIdx), index_counts[i], nullptr, true, name);
			}
		}
	};

	/**
	 * @struct WindowData
	 * @brief 
	 */
	struct WindowData
	{
		SceneryEditorX::Ref<ViewportResources> viewportResources;
		SceneryEditorX::Ref<SceneryEditorX::Swapchain> swapchain;
		SceneryEditorX::CommandList *cmdList = nullptr;
	};
	
	// main window resources
	ViewportResources g_ViewportData;
	
	// shared resources (between all windows)
	SceneryEditorX::Ref<SceneryEditorX::ImageResource>     g_FontAtlas;
	SceneryEditorX::Ref<SceneryEditorX::DepthStencilState> g_DepthStencil_State;
	SceneryEditorX::Ref<SceneryEditorX::RasterizerState>   g_Rasterizer_State;
	SceneryEditorX::Ref<SceneryEditorX::BlendState>        g_BlendState;
	SceneryEditorX::Ref<SceneryEditorX::Shader>            g_VertexShader;
	SceneryEditorX::Ref<SceneryEditorX::Shader>            g_FragmentShader;

	/**
	 * @brief 
	 */
	static void InitializePlatformInterface();

	/**
	 * @brief 
	 */
	static void DestroyResources();

	/**
	 * @brief 
	 */
	static void Initialize();

	/**
	 * @brief 
	 */
	static void Shutdown();

	/**
	 * @brief 
	 * @param drawData 
	 * @param windowData 
	 * @param clear 
	 */
	static void Render(ImDrawData *drawData, WindowData *windowData = nullptr, const bool clear = true);

	/**
	 * @brief 
	 * @param viewport 
	 */
	static void WindowCreate(ImGuiViewport *viewport);

	/**
	 * @brief 
	 * @param viewport 
	 */
	static void WindowDestroy(ImGuiViewport *viewport);

	/**
	 * @brief 
	 * @param viewport 
	 * @param size 
	 */
	static void WindowResize(ImGuiViewport *viewport, const ImVec2 size);

	/**
	 * @brief 
	 * @param viewport 
	 */
	static void WindowRender(ImGuiViewport *viewport, void *);

	/**
	 * @brief 
	 * @param viewport 
	 */
	static void WindowPresent(ImGuiViewport *viewport, void *);

}

// -------------------------------------------------------
