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
 * scene_viewport.cpp
 * -------------------------------------------------------
 * Created: 01/08/2025
 * -------------------------------------------------------
 */
#include "scene_viewport.h"
#include <Editor/core/editor_camera.h>
#include <Editor/core/editor_theme.h>
#include <Editor/core/editor_ui.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/ui/ui_impl.h>
#include <SceneryEditorX/scene/camera.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		/* Frames the panel size must stay unchanged before render targets are recreated at the new size. */
		constexpr uint32_t RESIZE_SETTLE_FRAMES = 6;

		/* Below this the renderer refuses to render (see Renderer::BeginFrame). */
		constexpr uint32_t MIN_RENDER_SIZE = 64;

		constexpr float TOOLBAR_HEIGHT = 30.0f;
	}

	SceneViewport::SceneViewport(EditorContext *context) : m_Context(context)
	{
		m_Title = "VIEWPORT";
	}

	void SceneViewport::OnUIRender(bool &isOpen)
	{
		// The viewport fills its dock node edge to edge; its tab bar only shows if other panels get docked into it.
		ImGuiWindowClass windowClass;
		windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_AutoHideTabBar;
		ImGui::SetNextWindowClass(&windowClass);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		const bool visible = EditorUI::BeginPanel("VIEWPORT###Viewport", &isOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::PopStyleVar();

		bool hovered = false;
		bool dragging = false;

		if (visible)
		{
			Camera *camera = EditorCamera::Get();
			DrawToolbar(camera);

			const ImVec2 imageMin = ImGui::GetCursorScreenPos();
			const ImVec2 imageSize = ImGui::GetContentRegionAvail();
			const uint32_t width  = static_cast<uint32_t>(std::max(imageSize.x, 0.0f));
			const uint32_t height = static_cast<uint32_t>(std::max(imageSize.y, 0.0f));
			UpdateRenderSize(width, height);

			m_ViewportBounds[0] = Vec2(imageMin.x, imageMin.y);
			m_ViewportBounds[1] = Vec2(imageMin.x + imageSize.x, imageMin.y + imageSize.y);

			if (imageSize.x > 0.0f && imageSize.y > 0.0f)
			{
				// The whole image is one button so a drag that starts here keeps camera control after leaving it.
				ImGui::InvisibleButton("##ViewportCanvas", imageSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
				hovered = ImGui::IsItemHovered();
				dragging = ImGui::IsItemActive();

				ImDrawList *drawList = ImGui::GetWindowDrawList();
				const ImVec2 imageMax(imageMin.x + imageSize.x, imageMin.y + imageSize.y);
				const ImTextureID texture = UI::GetTextureID(Renderer::GetRenderTarget(Renderer_RenderTarget::frame_output));
				if (texture != ImTextureID_Invalid)
				{
					// Opaque backdrop: ImGui alpha-blends images, and pixels the renderer leaves transparent must
					// show as black (what the renderer produced), not as the panel colour behind the viewport.
					drawList->AddRectFilled(imageMin, imageMax, IM_COL32(0, 0, 0, 255));
					drawList->AddImage(texture, imageMin, imageMax);
				}
				else
				{
					drawList->AddRectFilled(imageMin, imageMax, EditorTheme::ToU32(EditorTheme::Palette::Viewport));
					constexpr const char *message = "Waiting for the renderer output...";
					const ImVec2 textSize = ImGui::CalcTextSize(message);
					drawList->AddText(ImVec2(imageMin.x + (imageSize.x - textSize.x) * 0.5f, imageMin.y + (imageSize.y - textSize.y) * 0.5f),
						EditorTheme::ToU32(EditorTheme::Palette::TextDisabled), message);
				}

				DrawOverlay(camera, imageMin);
			}
		}
		EditorUI::EndPanel();

		if (m_Context)
		{
			m_Context->viewportHovered = hovered;
			m_Context->viewportFocused = visible && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		}

		// Camera input: while the image is hovered (wheel, keyboard fly) or a drag that started on it continues.
		const bool typing = ImGui::GetIO().WantTextInput;
		Camera::SetInputEnabled(isOpen && (dragging || (hovered && !typing)));
	}

	void SceneViewport::DrawToolbar(Camera *camera)
	{
		using namespace EditorTheme;
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const float height = TOOLBAR_HEIGHT * dpi;

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ToStyle(Palette::PanelHeader));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f * dpi, 3.0f * dpi));
		ImGui::BeginChild("##ViewportToolbar", ImVec2(0.0f, height), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar);
		ImGui::PopStyleVar();

		const float buttonSize = height - 6.0f * dpi;
		const float rightEdge = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
		const bool hasCamera = camera != nullptr;

		ImGui::BeginDisabled(!hasCamera);

		// Left: navigation mode and projection, as toggle buttons.
		if (EditorUI::StripButton(SEDX_ICON_REFRESH, "Orbit navigation\nLMB orbit, RMB pan, wheel zoom", EditorCamera::IsMode(ORBIT), buttonSize))
			EditorCamera::SetMode(ORBIT);
		ImGui::SameLine(0.0f, 2.0f * dpi);
		if (EditorUI::StripButton(SEDX_ICON_VIDEO_CAMERA, "Fly navigation\nRMB look, WASD move, Q/E down/up", EditorCamera::IsMode(FLY), buttonSize))
			EditorCamera::SetMode(FLY);

		ImGui::SameLine(0.0f, 10.0f * dpi);
		if (EditorUI::StripButton(SEDX_ICON_CUBE, "Perspective projection", EditorCamera::IsProjection(CameraType::Perspective), buttonSize))
			EditorCamera::SetProjection(CameraType::Perspective);
		ImGui::SameLine(0.0f, 2.0f * dpi);
		if (EditorUI::StripButton(SEDX_ICON_TH_LARGE, "Orthographic projection", EditorCamera::IsProjection(CameraType::Orthographic), buttonSize))
			EditorCamera::SetProjection(CameraType::Orthographic);

		// Right: reset view.
		ImGui::SameLine(rightEdge - buttonSize);
		if (EditorUI::StripButton(SEDX_ICON_CROSSHAIRS, "Reset camera to the startup view", false, buttonSize))
			EditorCamera::Reset();

		ImGui::EndDisabled();

		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

	void SceneViewport::DrawOverlay(Camera *camera, const ImVec2 &imageMin) const
	{
		using namespace EditorTheme;
		const float dpi = ImGui::GetStyle().FontScaleDpi;

		char text[128];
		if (camera)
		{
			snprintf(text, sizeof(text), "%s  |  %s  |  %u x %u", EditorCamera::GetModeName(),
				EditorCamera::IsProjection(CameraType::Orthographic) ? "Orthographic" : "Perspective", m_AppliedWidth, m_AppliedHeight);
		}
		else
		{
			snprintf(text, sizeof(text), "No camera  |  %u x %u", m_AppliedWidth, m_AppliedHeight);
		}

		const ImVec2 padding(8.0f * dpi, 4.0f * dpi);
		const ImVec2 textSize = ImGui::CalcTextSize(text);
		const ImVec2 boxMin(imageMin.x + 8.0f * dpi, imageMin.y + 8.0f * dpi);
		const ImVec2 boxMax(boxMin.x + textSize.x + padding.x * 2.0f, boxMin.y + textSize.y + padding.y * 2.0f);

		ImDrawList *drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(boxMin, boxMax, ToU32(ImVec4(Palette::Separator.x, Palette::Separator.y, Palette::Separator.z, 0.75f)), 2.0f * dpi);
		drawList->AddText(ImVec2(boxMin.x + padding.x, boxMin.y + padding.y), ToU32(Palette::TextLabel), text);
	}

	void SceneViewport::UpdateRenderSize(const uint32_t width, const uint32_t height)
	{
		if (width != m_PendingWidth || height != m_PendingHeight)
		{
			m_PendingWidth = width;
			m_PendingHeight = height;
			m_StableFrames = 0;
			return;
		}

		if (m_StableFrames < RESIZE_SETTLE_FRAMES)
		{
			++m_StableFrames;
			return;
		}

		if (width == m_AppliedWidth && height == m_AppliedHeight)
			return;

		if (width < MIN_RENDER_SIZE || height < MIN_RENDER_SIZE)
			return;

		// Safe from the UI: the render thread is idle while the main thread builds the frame.
		// SetRendererResolution waits for the GPU before recreating, which also covers the output targets.
		Renderer::SetViewport(static_cast<float>(width), static_cast<float>(height));
		Renderer::SetRendererResolution(width, height);
		Renderer::SetOutputResolution(width, height);

		m_AppliedWidth = width;
		m_AppliedHeight = height;
		EDITOR_INFO_TAG("Viewport", "Render resolution set to {} x {}", width, height);
	}

}

// -------------------------------------------------------
