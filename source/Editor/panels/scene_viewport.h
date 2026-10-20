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
 * scene_viewport.h
 * -------------------------------------------------------
 * Created: 01/08/2025
 * -------------------------------------------------------
 */
#pragma once
#include <array>
#include <filesystem>
#include <Editor/core/editor_context.h>
#include <SceneryEditorX/renderer/vulkan/image_resource.h>
#include <SceneryEditorX/renderer/ui/editor_panel.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Camera;

	/**
	 * @class SceneViewport
	 * @brief The central 3D viewport: shows the renderer's frame output and owns camera navigation.
	 *
	 * - Resizes the renderer (render + output resolution) to the panel once its size has settled,
	 *   so dragging a dock splitter doesn't recreate render targets every frame.
	 * - Enables camera input only while the image is hovered or a drag that started on it is active.
	 * - Toolbar row (camera mode, projection, reset) and an info overlay, after Substance's viewport header.
	 */
	class SceneViewport : public UI::EditorPanel
	{
	public:
		explicit SceneViewport(EditorContext *context);

		void OnUIRender(bool &isOpen) override;

		/* @brief Screen-space bounds (min, max) of the rendered image, as of the last frame it was drawn. */
		[[nodiscard]] std::array<Vec2, 2> GetViewportBounds() const { return m_ViewportBounds; }

		/* @brief Size in pixels the renderer was last resized to. */
		[[nodiscard]] Vec2 GetRenderSize() const { return { static_cast<float>(m_AppliedWidth), static_cast<float>(m_AppliedHeight) }; }

		/**
		 * @brief Renders at a fixed size instead of the panel's (parity captures: the same image size and aspect as the
		 * X-Plane reference); the image is shown fitted into the panel. 0 x 0 follows the panel again.
		 */
		static void SetFixedRenderSize(uint32_t width, uint32_t height);

		/* @brief What the REFERENCE menu shows over the rendered image. */
		enum class ReferenceView : uint8_t
		{
			Off,
			Split,			// Reference left of a draggable vertical line
			Difference,		// |frame - reference| x gain
			Flicker			// Reference and frame alternate at 1 Hz
		};

		/* @brief Loads a reference image (PNG) for the overlay; false when it can't be read. */
		bool LoadReference(const std::filesystem::path &png);
		void SetReferenceView(ReferenceView view) { m_ReferenceView = view; }

		/* @brief A reference the viewport loads when it is next drawn (--reference-image, --reference-view). */
		static void RequestReference(const std::filesystem::path &png, ReferenceView view);

	private:
		void DrawToolbar(Camera *camera);
		void DrawOverlay(Camera *camera, const ImVec2 &imageMin) const;
		void DrawReferenceMenu(float buttonSize);
		/* @brief The draggable split line; true while it has the mouse (the camera doesn't get it then). */
		bool DrawReferenceSplit(const ImVec2 &imageMin, const ImVec2 &imageSize);
		void UpdateReferenceOverlay() const;
		void UpdateRenderSize(uint32_t width, uint32_t height);

		/**
		 * @brief Intersects the camera ray through a viewport point with the ground plane (y = 0).
		 * @param u, v Point in the image, 0..1 from the top-left.
		 * @param aspect Image width / height.
		 * @param out Receives the world-space hit.
		 * @return False when the ray doesn't reach the ground.
		 */
		static bool PickGround(float u, float v, float aspect, float out[3]);

		EditorContext *m_Context = nullptr;

		Ref<ImageResource> m_Reference;
		std::filesystem::path m_ReferencePath;
		ReferenceView m_ReferenceView = ReferenceView::Off;
		float m_ReferenceSplit = 0.5f;
		float m_ReferenceGain = 4.0f;
		std::array<Vec2, 2> m_ViewportBounds = {};

		uint32_t m_PendingWidth		= 0;
		uint32_t m_PendingHeight	= 0;
		uint32_t m_StableFrames		= 0;
		uint32_t m_AppliedWidth		= 0;
		uint32_t m_AppliedHeight	= 0;
	};

}

// -------------------------------------------------------
