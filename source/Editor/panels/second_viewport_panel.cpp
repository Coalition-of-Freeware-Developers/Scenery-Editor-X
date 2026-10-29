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
 * second_viewport_panel.cpp
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#include "second_viewport_panel.h"
#include <Editor/core/editor_camera.h>
#include <Editor/core/editor_theme.h>
#include <Editor/core/editor_ui.h>
#include <Editor/settings/editor_preferences.h>
#include <algorithm>
#include <cmath>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/font/font_awesome.h>
#include <SceneryEditorX/renderer/renderer.h>
#include <SceneryEditorX/renderer/ui/ui_impl.h>
#include <SceneryEditorX/scene/camera.h>
#include <SceneryEditorX/xplane/xp_airport_scene.h>
#include <SceneryEditorX/xplane/xp_terrain_scene.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		using ViewportCamera::Rig;
		using ViewportCamera::ViewPreset;

		constexpr uint32_t RESIZE_SETTLE_FRAMES = 3;	// The size must hold this long before the targets are made again
		constexpr uint32_t MIN_RENDER_SIZE = 16;
		constexpr float REMEMBER_DELAY = 1.0f;			// Seconds after the camera stops before it is saved

		Renderer::ViewCamera ToViewCamera(const Rig &rig, const double aspect)
		{
			Renderer::ViewCamera camera;
			const Point3 eye = rig.Eye();
			camera.position = Vec3(static_cast<float>(eye.x), static_cast<float>(eye.y), static_cast<float>(eye.z));
			camera.yaw = static_cast<float>(rig.yaw);
			camera.pitch = static_cast<float>(rig.pitch);
			camera.horizontalFov = static_cast<float>(std::clamp(rig.fovHorizontal, 0.05, 3.0));
			camera.orthographic = rig.orthographic;
			camera.orthographicHeight = static_cast<float>(rig.OrthographicHeight(aspect));
			return camera;
		}

		bool SameRig(const Rig &a, const Rig &b)
		{
			return a.pivot.x == b.pivot.x && a.pivot.y == b.pivot.y && a.pivot.z == b.pivot.z && a.distance == b.distance && a.yaw == b.yaw &&
				a.pitch == b.pitch && a.fovHorizontal == b.fovHorizontal && a.orthographic == b.orthographic;
		}
	}

	SecondViewportPanel::SecondViewportPanel(EditorContext *context) : m_Context(context)
	{
	}

	SecondViewportPanel::~SecondViewportPanel()
	{
		Release();
	}

	void SecondViewportPanel::Release()
	{
		m_NoView = false;	// Asked again when the panel opens
		if (m_View == 0)
			return;
		Remember();
		Renderer::DestroyView(m_View);
		m_View = 0;
		m_AppliedWidth = m_AppliedHeight = 0;
		m_PendingWidth = m_PendingHeight = 0;
		m_StableFrames = 0;
	}

	void SecondViewportPanel::InitializeRig()
	{
		// The camera it had last time; the first time, a top view of what the main viewport looks at
		const EditorPreferences::ViewportCameraState &saved = EditorPreferences::Get().viewport2;
		if (saved.saved)
		{
			m_Rig.pivot = { saved.pivot[0], saved.pivot[1], saved.pivot[2] };
			m_Rig.distance = saved.distance;
			m_Rig.yaw = saved.yaw;
			m_Rig.pitch = saved.pitch;
			m_Rig.fovHorizontal = saved.fovHorizontal;
			m_Rig.orthographic = saved.orthographic;
			m_Rig.preset = static_cast<ViewPreset>(std::clamp(saved.preset, 0, static_cast<int>(ViewPreset::Right)));
		}
		else
		{
			Rig main;
			if (EditorCamera::GetRig(main))
				m_Rig = main;
			m_Rig.fly = false;
			double yaw = 0.0, pitch = 0.0;
			if (ViewportCamera::GetPresetDirection(ViewPreset::Top, yaw, pitch))
			{
				m_Rig.yaw = yaw;
				m_Rig.pitch = pitch;
				m_Rig.preset = ViewPreset::Top;
			}
			ViewportCamera::Controller::SetOrthographic(m_Rig, true);
			m_FrameWhenReady = true;	// The main camera's pivot can be anywhere: frame the airport once it is there
			m_FrameWaitFrames = 0;
		}
		m_RigReady = true;
	}

	bool SecondViewportPanel::FrameFocus(const bool jump)
	{
		// The open airport's focus; without one (after a while), the ground under the main camera
		float x = 0.0f, z = 0.0f, radius = 0.0f;
		Point3 center;
		if (XPlane::SceneAirports::GetFocus(x, z, radius))
			center = { x, XPlane::SceneTerrain::GetHeight(x, z), z };
		else
		{
			Rig main;
			if (!jump || m_FrameWaitFrames < 120 || !EditorCamera::GetRig(main))
				return false;
			const Point3 eye = main.Eye();
			center = { eye.x, XPlane::SceneTerrain::GetHeight(eye.x, eye.z), eye.z };
			radius = static_cast<float>(std::clamp(eye.y, 50.0, 5000.0));
		}
		Rig target = m_Rig;
		target.pivot = center;
		target.distance = std::max(1.0, 1.1 * radius / std::max(target.TanHalfFovY(m_Aspect), 1e-3));
		if (jump)
			m_Rig = target;
		else
			m_Controller.MoveTo(m_Rig, target);
		return true;
	}

	void SecondViewportPanel::Remember()
	{
		EditorPreferences::ViewportCameraState &saved = EditorPreferences::Get().viewport2;
		saved.saved = true;
		saved.pivot[0] = m_Rig.pivot.x;
		saved.pivot[1] = m_Rig.pivot.y;
		saved.pivot[2] = m_Rig.pivot.z;
		saved.distance = m_Rig.distance;
		saved.yaw = m_Rig.yaw;
		saved.pitch = m_Rig.pitch;
		saved.fovHorizontal = m_Rig.fovHorizontal;
		saved.orthographic = m_Rig.orthographic;
		saved.preset = static_cast<int>(m_Rig.preset);
		EditorPreferences::Save();
		m_RememberIn = -1.0f;
	}

	void SecondViewportPanel::OnUIRender(bool &isOpen)
	{
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		ImGui::SetNextWindowSize(ImVec2(720.0f * dpi, 460.0f * dpi), ImGuiCond_FirstUseEver);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		const bool visible = EditorUI::BeginPanel("VIEWPORT 2###Viewport2", &isOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::PopStyleVar();
		m_Hovered = false;

		if (visible)
		{
			if (m_View == 0 && !m_NoView)
			{
				m_View = Renderer::CreateView();
				m_NoView = m_View == 0;	// The renderer has no view left: asked again when the panel opens next
			}
			if (!m_RigReady)
				InitializeRig();

			DrawToolbar();

			const ImVec2 imageMin = ImGui::GetCursorScreenPos();
			const ImVec2 imageSize = ImGui::GetContentRegionAvail();
			UpdateRenderSize(static_cast<uint32_t>(std::max(imageSize.x, 0.0f)), static_cast<uint32_t>(std::max(imageSize.y, 0.0f)));

			if (imageSize.x > 1.0f && imageSize.y > 1.0f)
			{
				ImDrawList *drawList = ImGui::GetWindowDrawList();
				const ImVec2 imageMax(imageMin.x + imageSize.x, imageMin.y + imageSize.y);
				ImageResource *output = Renderer::GetViewOutput(m_View);
				const ImTextureID texture = output ? UI::GetTextureID(output) : ImTextureID_Invalid;
				drawList->AddRectFilled(imageMin, imageMax, IM_COL32(0, 0, 0, 255));
				if (texture != ImTextureID_Invalid && m_AppliedWidth > 0)
					drawList->AddImage(texture, imageMin, imageMax);
				else
				{
					const char *message = m_NoView ? "The renderer has no view left: close another viewport" : "Starting the second view...";
					const ImVec2 textSize = ImGui::CalcTextSize(message);
					drawList->AddText(ImVec2(imageMin.x + (imageSize.x - textSize.x) * 0.5f, imageMin.y + (imageSize.y - textSize.y) * 0.5f),
						EditorTheme::ToU32(EditorTheme::Palette::TextDisabled), message);
				}

				// The view's name in the corner, like the main viewport's overlay
				const char *preset = m_Rig.preset == ViewPreset::Free ? "User" : ViewportCamera::GetPresetName(m_Rig.preset);
				const std::string label = std::string(preset) + (m_Rig.orthographic ? " Orthographic" : " Perspective");
				drawList->AddText(ImVec2(imageMin.x + 8.0f * dpi, imageMin.y + 6.0f * dpi), EditorTheme::ToU32(EditorTheme::Palette::TextLabel), label.c_str());

				// Navigation with the user's keymap over the image; a drag that starts here keeps the camera when it leaves
				ImGui::SetCursorScreenPos(imageMin);
				ImGui::InvisibleButton("##Viewport2Canvas", imageSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
				m_Hovered = ImGui::IsItemHovered();
				m_Aspect = m_AppliedWidth > 0 && m_AppliedHeight > 0 ? static_cast<double>(m_AppliedWidth) / m_AppliedHeight : imageSize.x / imageSize.y;

				ViewportCamera::Input input;
				input.hovered = m_Hovered;
				input.locked = Camera::IsInputLocked();
				input.imageSize = imageSize;
				input.aspect = m_Aspect;
				const Rig before = m_Rig;
				m_Controller.Navigate(m_Rig, input);
				if (m_Hovered)
					HandleShortcuts();
				if (!SameRig(before, m_Rig))
					m_FrameWhenReady = false;	// The user has the camera
				if (m_FrameWhenReady)
				{
					++m_FrameWaitFrames;
					if (FrameFocus(true))
						m_FrameWhenReady = false;
				}
				m_Controller.Animate(m_Rig, ImGui::GetIO().DeltaTime);
				if (!SameRig(before, m_Rig))
					m_RememberIn = REMEMBER_DELAY;
			}

			// The renderer draws the view this frame (a view nobody shows isn't drawn)
			Renderer::SetViewCamera(m_View, ToViewCamera(m_Rig, m_Aspect));

			if (m_RememberIn > 0.0f && !m_Controller.IsDragging())
			{
				m_RememberIn -= ImGui::GetIO().DeltaTime;
				if (m_RememberIn <= 0.0f)
					Remember();
			}
		}
		EditorUI::EndPanel();

		if (!isOpen)
			Release();
	}

	void SecondViewportPanel::DrawToolbar()
	{
		const float dpi = ImGui::GetStyle().FontScaleDpi;
		const float buttonSize = ImGui::GetFrameHeight();
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 4.0f * dpi, ImGui::GetCursorPosY() + 3.0f * dpi));
		ImGui::BeginGroup();

		const auto presetButton = [&](const char *label, const ViewPreset preset, const char *tooltip)
		{
			const bool active = m_Rig.preset == preset;
			if (active)
				ImGui::PushStyleColor(ImGuiCol_Button, EditorTheme::ToStyle(EditorTheme::Palette::Accent));
			if (ImGui::Button(label, ImVec2(0.0f, buttonSize)))
				m_Controller.SetPreset(m_Rig, preset);
			if (active)
				ImGui::PopStyleColor();
			EditorUI::ItemTooltip(tooltip);
			ImGui::SameLine(0.0f, 2.0f * dpi);
		};
		presetButton("Top", ViewPreset::Top, "Look straight down, north up (Numpad 7; Ctrl for the bottom)");
		presetButton("Front", ViewPreset::Front, "Look north (Numpad 1; Ctrl for the back)");
		presetButton("Right", ViewPreset::Right, "Look west (Numpad 3; Ctrl for the left side)");

		ImGui::SameLine(0.0f, 8.0f * dpi);
		if (EditorUI::StripButton(m_Rig.orthographic ? SEDX_ICON_SQUARE_O : SEDX_ICON_CUBE, m_Rig.orthographic ? "Orthographic\nClick for perspective (Numpad 5)" :
			"Perspective\nClick for orthographic (Numpad 5)", m_Rig.orthographic, buttonSize))
			ViewportCamera::Controller::SetOrthographic(m_Rig, !m_Rig.orthographic);
		ImGui::SameLine(0.0f, 2.0f * dpi);
		if (EditorUI::StripButton(SEDX_ICON_ARROWS_ALT, "Frame the airport (Home)", false, buttonSize))
			FrameFocus(false);
		ImGui::SameLine(0.0f, 2.0f * dpi);
		if (EditorUI::StripButton(SEDX_ICON_EXCHANGE, "Match the main viewport's camera", false, buttonSize))
		{
			Rig main;
			if (EditorCamera::GetRig(main))
			{
				main.fly = false;
				m_Controller.MoveTo(m_Rig, main);
			}
		}
		ImGui::EndGroup();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f * dpi);
	}

	void SecondViewportPanel::HandleShortcuts()
	{
		const ImGuiIO &io = ImGui::GetIO();
		if (io.WantTextInput || Camera::IsInputLocked())
			return;
		const bool ctrl = io.KeyCtrl;
		const auto pressed = [](const ImGuiKey key) { return ImGui::IsKeyPressed(key, false); };
		if (pressed(ImGuiKey_Keypad7))
			m_Controller.SetPreset(m_Rig, ctrl ? ViewPreset::Bottom : ViewPreset::Top);
		if (pressed(ImGuiKey_Keypad1))
			m_Controller.SetPreset(m_Rig, ctrl ? ViewPreset::Back : ViewPreset::Front);
		if (pressed(ImGuiKey_Keypad3))
			m_Controller.SetPreset(m_Rig, ctrl ? ViewPreset::Left : ViewPreset::Right);
		if (pressed(ImGuiKey_Keypad5))
			ViewportCamera::Controller::SetOrthographic(m_Rig, !m_Rig.orthographic);
		if (pressed(ImGuiKey_Keypad9))
			m_Controller.ViewFromOtherSide(m_Rig);
		if (ImGui::IsKeyPressed(ImGuiKey_Keypad4))
			m_Controller.Orbit(m_Rig, -15.0, 0.0);
		if (ImGui::IsKeyPressed(ImGuiKey_Keypad6))
			m_Controller.Orbit(m_Rig, 15.0, 0.0);
		if (ImGui::IsKeyPressed(ImGuiKey_Keypad8))
			m_Controller.Orbit(m_Rig, 0.0, -15.0);
		if (ImGui::IsKeyPressed(ImGuiKey_Keypad2))
			m_Controller.Orbit(m_Rig, 0.0, 15.0);
		if (pressed(ImGuiKey_Home) && !ctrl)
			FrameFocus(false);
	}

	void SecondViewportPanel::UpdateRenderSize(const uint32_t width, const uint32_t height)
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
		if ((width == m_AppliedWidth && height == m_AppliedHeight) || width < MIN_RENDER_SIZE || height < MIN_RENDER_SIZE)
			return;

		// Safe from the UI: the render thread is idle while the main thread builds the frame (it waits for the GPU)
		Renderer::SetViewResolution(m_View, width, height);
		m_AppliedWidth = width;
		m_AppliedHeight = height;
		EDITOR_INFO_TAG("Viewport", "Viewport 2 render resolution set to {} x {}", width, height);
	}

}

// -------------------------------------------------------
