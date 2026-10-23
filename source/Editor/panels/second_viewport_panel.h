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
 * second_viewport_panel.h
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <Editor/core/editor_context.h>
#include <Editor/core/viewport_camera.h>
#include <SceneryEditorX/renderer/ui/editor_panel.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class SecondViewportPanel
	 * @brief "VIEWPORT 2" (Window > New Viewport): a second camera on the scene, drawn by the renderer as an extra view
	 * (Renderer::CreateView) through the same X-Plane 12 passes as the main viewport. It has its own camera and
	 * navigation (the user's keymap, numpad views, the projection), docks like any panel or floats on another monitor,
	 * and remembers its camera between sessions. Picking and the editing tools stay in the main viewport; selection
	 * outlines show in both. Closing it frees the view's render targets.
	 */
	class SecondViewportPanel : public UI::EditorPanel
	{
	public:
		explicit SecondViewportPanel(EditorContext *context);
		~SecondViewportPanel() override;

		void OnUIRender(bool &isOpen) override;

		/* @brief Frees the renderer view (the panel is closed). It comes back when the panel opens again. */
		void Release();

		/* @brief The renderer view (0 while closed). */
		[[nodiscard]] uint32_t GetView() const { return m_View; }

	private:
		void InitializeRig();
		/* @brief Fits the airport (or, without one, the ground under the main camera) keeping the direction. False when there is nothing yet. */
		bool FrameFocus(bool jump);
		void DrawToolbar();
		void HandleShortcuts();
		void UpdateRenderSize(uint32_t width, uint32_t height);
		void Remember();

		EditorContext *m_Context = nullptr;
		uint32_t m_View = 0;
		ViewportCamera::Rig m_Rig;
		ViewportCamera::Controller m_Controller;
		bool m_RigReady = false;
		double m_Aspect = 1.0;
		bool m_Hovered = false;
		bool m_NoView = false;			// Renderer::CreateView refused (every extra view in use)
		bool m_FrameWhenReady = false;	// The first time: frame the airport once it has loaded
		int m_FrameWaitFrames = 0;

		uint32_t m_PendingWidth = 0;
		uint32_t m_PendingHeight = 0;
		uint32_t m_StableFrames = 0;
		uint32_t m_AppliedWidth = 0;
		uint32_t m_AppliedHeight = 0;
		float m_RememberIn = -1.0f;		// Seconds until the moved camera is saved (-1: nothing to save)
	};

}

// -------------------------------------------------------
