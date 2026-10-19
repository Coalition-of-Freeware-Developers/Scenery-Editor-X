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
#include <vulkan/vulkan.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class ImageResource;
}

namespace SceneryEditorX::UI
{
	/**
	 * Renderer backend for Dear ImGui.
	 *
	 * Wraps the official imgui_impl_vulkan backend (dynamic rendering, multi-viewport platform windows and
	 * ImGuiBackendFlags_RendererHasTextures for dynamic font atlases). The main viewport is recorded by the
	 * render thread inside the swapchain pass; secondary viewports are rendered by ImGui itself from
	 * UILayer::EndRendering() on the main thread, while the render thread is idle.
	 */

	/**
	 * @brief Initializes the Vulkan renderer backend. Requires an ImGui context and an initialized Renderer swapchain.
	 * @return True when the backend is ready to render.
	 */
	bool InitializeBackend();

	/* @brief Waits for the GPU and releases every backend resource (pipelines, descriptors, platform windows). */
	void ShutdownBackend();

	/* @brief True once InitializeBackend() succeeded and until ShutdownBackend(). */
	bool IsBackendInitialized();

	/* @brief Starts a backend frame and recycles image descriptors that the GPU is guaranteed to be done with. */
	void NewFrame();

	/**
	 * @brief True when the main swapchain format encodes sRGB on write.
	 * ImGui colors are authored in sRGB, so themes must convert them to linear in that case.
	 */
	bool IsLinearColorTarget();

	/**
	 * @brief Records the main viewport's draw data. Call inside an active dynamic rendering pass on the swapchain image.
	 * @param commandBuffer The command buffer that is recording the swapchain pass.
	 */
	void RenderMainViewport(VkCommandBuffer commandBuffer);

	/**
	 * @brief Returns an ImGui texture handle for an engine image, valid for the current UI frame only.
	 * The image must be in SHADER_READ_ONLY_OPTIMAL layout when the UI is recorded.
	 * @param image The image to display (mip 0 is sampled).
	 * @return The texture handle, or ImTextureID_Invalid when the image has no view or the backend is down.
	 */
	ImTextureID GetTextureID(ImageResource *image);

	/**
	 * @brief Checks if the specified window is focused.
	 * @param windowName Name of the window to check for focus.
	 * @param checkWindow When true, compares against the root of the focused window (ignores child windows and tables).
	 * @return True if the window is focused, false otherwise.
	 */
	bool IsWindowFocused(const char *windowName, bool checkWindow = true);

}

// -------------------------------------------------------
