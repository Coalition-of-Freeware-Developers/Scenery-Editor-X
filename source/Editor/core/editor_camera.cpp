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
 * editor_camera.cpp
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#include "editor_camera.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <Editor/core/editor_airports.h>
#include <Editor/core/editor_context.h>
#include <Editor/core/editor_project.h>
#include <Editor/core/editor_scene.h>
#include <Editor/core/editor_tools.h>
#include <SceneryEditorX/core/application/application.h>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/xplane/xp_airport_scene.h>

// -------------------------------------------------------

namespace SceneryEditorX::EditorCamera
{
	namespace
	{
		using ViewportCamera::Rig;
		using ViewportCamera::ViewPreset;

		struct State
		{
			bool homeCaptured = false;
			Rig home;
			ViewportCamera::Controller controller;
			ViewPreset preset = ViewPreset::Free;	// The entity keeps no preset: the last one set, while the camera looks along it
			double aspect = 16.0 / 9.0;				// The viewport's, for the orthographic height and framing

			// --camera-view / --camera-ortho, waiting for the startup airport or project
			bool startupPending = false;
			ViewPreset startupPreset = ViewPreset::Free;
			bool startupOrthographic = false;
			int startupFrames = 0;
		};

		State &S()
		{
			static State state;
			return state;
		}

		/* @brief The primary camera entity's transform and camera component (the renderer draws through them). */
		bool GetPrimary(TransformComponent *&transform, CameraComponent *&component)
		{
			entt::registry *registry = EditorScene::GetRegistry();
			if (!registry)
				return false;
			auto view = registry->view<TransformComponent, CameraComponent>();
			for (const entt::entity handle : view)
			{
				if (!view.get<CameraComponent>(handle).isPrimary)
					continue;
				transform = &view.get<TransformComponent>(handle);
				component = &view.get<CameraComponent>(handle);
				return true;
			}
			return false;
		}

		Point3 ToPoint(const Vec3 &v)
		{
			return { v.x, v.y, v.z };
		}

		Vec3 ToVec(const Point3 &p)
		{
			return Vec3(static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z));
		}

		/* @brief Whether the camera still looks along a preset (it stays the active one until the camera turns away). */
		bool LooksAlong(const Rig &rig, const ViewPreset preset)
		{
			double yaw = 0.0, pitch = 0.0;
			if (!ViewportCamera::GetPresetDirection(preset, yaw, pitch))
				return true;
			return std::abs(std::remainder(rig.yaw - yaw, 6.283185307179586)) < 1e-3 && std::abs(rig.pitch - pitch) < 1e-3;
		}
	}

	Camera *Get()
	{
		return Scene::GetCamera();
	}

	bool GetRig(Rig &rig)
	{
		Camera *camera = Get();
		TransformComponent *transform = nullptr;
		CameraComponent *component = nullptr;
		if (!camera || !GetPrimary(transform, component))
			return false;

		const Vec3 euler = transform->GetRotationEuler();
		rig.pitch = euler.x;
		rig.yaw = euler.y;
		rig.distance = std::max(0.1, static_cast<double>(camera->GetOrbitDistance()));
		rig.fovHorizontal = component->horizontalFov_Rad;
		rig.orthographic = component->projectionType == CameraComponent::ProjectionType::Orthographic;
		rig.fly = camera->mode == FLY;

		// The pivot, unless something moved the eye without it: then the eye wins (it is what the user sees)
		const Point3 eye = ToPoint(transform->translation);
		rig.pivot = ToPoint(camera->center);
		const Point3 expected = rig.Eye();
		const double drift = std::hypot(expected.x - eye.x, expected.y - eye.y, expected.z - eye.z);
		if (drift > 1e-3 * std::max(1.0, rig.distance))
			rig.KeepEye(eye);

		rig.preset = LooksAlong(rig, S().preset) ? S().preset : ViewPreset::Free;
		return true;
	}

	void SetRig(const Rig &rig)
	{
		Camera *camera = Get();
		TransformComponent *transform = nullptr;
		CameraComponent *component = nullptr;
		if (!camera || !GetPrimary(transform, component))
			return;

		const Vec3 eye = ToVec(rig.Eye());
		const Vec3 rotation(static_cast<float>(rig.pitch), static_cast<float>(rig.yaw), 0.0f);
		transform->translation = eye;
		transform->SetRotationEuler(rotation);
		component->projectionType = rig.orthographic ? CameraComponent::ProjectionType::Orthographic : CameraComponent::ProjectionType::Perspective;
		component->orthographicHeight = static_cast<float>(rig.OrthographicHeight(S().aspect));
		component->horizontalFov_Rad = static_cast<float>(std::clamp(rig.fovHorizontal, 0.05, 3.0));

		camera->eye = eye;
		camera->rotation = rotation;
		camera->center = ToVec(rig.pivot);
		camera->SetOrbitDistance(static_cast<float>(rig.distance));
		camera->mode = rig.fly ? FLY : ORBIT;
		camera->cameraType = rig.orthographic ? CameraType::Orthographic : CameraType::Perspective;
		camera->SetProjection(camera->cameraType);
		S().preset = rig.preset;
	}

	void CaptureHome()
	{
		Rig rig;
		if (!GetRig(rig))
			return;
		S().homeCaptured = true;
		S().home = rig;
	}

	void ReadCommandLine()
	{
		const PlatformContext *platform = Application::Get().GetPlatformContext();
		if (!platform)
			return;
		State &s = S();
		const std::vector<std::string> &args = platform->GetCommandLineArgs();
		for (size_t i = 0; i < args.size(); ++i)
		{
			if (args[i] == "--camera-ortho")
			{
				s.startupOrthographic = true;
				s.startupPending = true;
			}
			else if (args[i] == "--camera-view" && i + 1 < args.size())
			{
				const std::string &name = args[++i];
				for (const ViewPreset preset : { ViewPreset::Top, ViewPreset::Bottom, ViewPreset::Front, ViewPreset::Back, ViewPreset::Left, ViewPreset::Right })
				{
					std::string lower = ViewportCamera::GetPresetName(preset);
					std::ranges::transform(lower, lower.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
					if (lower == name)
						s.startupPreset = preset;
				}
				if (s.startupPreset == ViewPreset::Free)
					EDITOR_WARN_TAG("Camera", "--camera-view wants top, bottom, front, back, left or right (got '{}')", name);
				s.startupPending = true;
			}
		}
	}

	void Reset()
	{
		State &s = S();
		Rig rig;
		if (!s.homeCaptured || !GetRig(rig))
			return;
		Rig target = s.home;
		target.orthographic = rig.orthographic;	// The projection stays the user's choice
		target.fly = rig.fly;
		s.controller.MoveTo(rig, target);
		SetRig(rig);
	}

	void SetMode(const CameraMode mode)
	{
		Rig rig;
		if (!GetRig(rig))
			return;
		rig.fly = mode == FLY;
		SetRig(rig);
	}

	bool IsMode(const CameraMode mode)
	{
		const Camera *camera = Get();
		return camera && camera->mode == mode;
	}

	void SetProjection(const CameraType projection)
	{
		Rig rig;
		if (!GetRig(rig))
			return;
		ViewportCamera::Controller::SetOrthographic(rig, projection == CameraType::Orthographic);
		SetRig(rig);
	}

	bool IsProjection(const CameraType projection)
	{
		TransformComponent *transform = nullptr;
		CameraComponent *component = nullptr;
		if (!GetPrimary(transform, component))
			return false;
		const bool orthographic = component->projectionType == CameraComponent::ProjectionType::Orthographic;
		return orthographic == (projection == CameraType::Orthographic);
	}

	void ToggleProjection()
	{
		SetProjection(IsProjection(CameraType::Orthographic) ? CameraType::Perspective : CameraType::Orthographic);
	}

	const char *GetModeName()
	{
		const Camera *camera = Get();
		if (!camera)
			return "None";

		switch (camera->mode)
		{
			case ORBIT:	return "Orbit";
			case FLY:	return "Fly";
			case FREE:	return "Free";
			case FIXED:	return "Fixed";
			default:	return "None";
		}
	}

	void SetViewPreset(const ViewPreset preset)
	{
		Rig rig;
		if (!GetRig(rig))
			return;
		S().controller.SetPreset(rig, preset);
		SetRig(rig);
	}

	ViewPreset GetViewPreset()
	{
		Rig rig;
		return GetRig(rig) ? rig.preset : ViewPreset::Free;
	}

	void Frame(const Point3 &center, const double radius)
	{
		Rig rig;
		if (!GetRig(rig))
			return;
		S().controller.Frame(rig, center, radius, S().aspect);
		SetRig(rig);
	}

	bool FrameAll()
	{
		// The open airport's focus (its outliers left out), else everything the scene draws
		float x = 0.0f, z = 0.0f, radius = 0.0f;
		if (XPlane::SceneAirports::GetFocus(x, z, radius))
		{
			Frame({ x, 0.0, z }, radius);
			return true;
		}

		bool any = false;
		Vec3 lo(0.0f), hi(0.0f);
		for (Entity *entity : Scene::GetRenderableEntities())
		{
			const Renderable *renderable = entity ? entity->GetComponent<Renderable>() : nullptr;
			if (!renderable || !entity->GetActive())
				continue;
			const BoundingBox &box = renderable->GetBoundingBox();
			const Vec3 bmin = box.GetMin(), bmax = box.GetMax();
			if (!(bmin.x <= bmax.x) || std::abs(bmin.x) > 1e7f || std::abs(bmax.x) > 1e7f)
				continue;
			lo = any ? Vec3(std::min(lo.x, bmin.x), std::min(lo.y, bmin.y), std::min(lo.z, bmin.z)) : bmin;
			hi = any ? Vec3(std::max(hi.x, bmax.x), std::max(hi.y, bmax.y), std::max(hi.z, bmax.z)) : bmax;
			any = true;
		}
		if (!any)
			return false;
		const Point3 center{ (lo.x + hi.x) * 0.5, (lo.y + hi.y) * 0.5, (lo.z + hi.z) * 0.5 };
		const double extent = std::hypot(static_cast<double>(hi.x - lo.x), static_cast<double>(hi.y - lo.y), static_cast<double>(hi.z - lo.z)) * 0.5;
		Frame(center, std::min(extent, 4000.0));
		return true;
	}

	void Orbit(const double yawDegrees, const double pitchDegrees)
	{
		Rig rig;
		if (!GetRig(rig))
			return;
		S().controller.Orbit(rig, yawDegrees, pitchDegrees);
		SetRig(rig);
	}

	void ViewFromOtherSide()
	{
		Rig rig;
		if (!GetRig(rig))
			return;
		S().controller.ViewFromOtherSide(rig);
		SetRig(rig);
	}

	void Update(const ViewportCamera::Input &input)
	{
		State &s = S();
		Rig rig;
		if (!GetRig(rig))
			return;

		const double aspect = std::max(input.aspect, 1e-3);
		const bool shapeChanged = std::abs(aspect - s.aspect) > 1e-6;
		s.aspect = aspect;

		// Startup view flags, a few frames after the startup airport or project framed the camera
		bool changed = false;
		if (s.startupPending && (EditorAirports::GetDocument() || EditorProjects::GetDocument()) && ++s.startupFrames > 10)
		{
			s.startupPending = false;
			double yaw = rig.yaw, pitch = rig.pitch;
			if (ViewportCamera::GetPresetDirection(s.startupPreset, yaw, pitch))
			{
				rig.yaw = yaw;
				rig.pitch = pitch;
				rig.preset = s.startupPreset;
			}
			rig.orthographic = rig.orthographic || s.startupOrthographic;
			changed = true;
			EDITOR_INFO_TAG("Camera", "Startup view: {} {}", ViewportCamera::GetPresetName(rig.preset), rig.orthographic ? "orthographic" : "perspective");
		}

		ViewportCamera::Input frame = input;
		frame.locked = input.locked || Camera::IsInputLocked();
		changed |= s.controller.Navigate(rig, frame);
		changed |= s.controller.Animate(rig, ImGui::GetIO().DeltaTime);
		if (changed || (shapeChanged && rig.orthographic))
			SetRig(rig);
	}

	bool IsNavigating()
	{
		return S().controller.IsDragging();
	}

	void HandleShortcuts(EditorContext *context)
	{
		const ImGuiIO &io = ImGui::GetIO();
		if (io.WantTextInput || !context || !(context->viewportHovered || context->viewportFocused) || !Get() || Camera::IsInputLocked())
			return;

		// Numpad views in every keymap (Blender's layout)
		const bool ctrl = io.KeyCtrl;
		const auto pressed = [](const ImGuiKey key) { return ImGui::IsKeyPressed(key, false); };
		if (pressed(ImGuiKey_Keypad7))
			SetViewPreset(ctrl ? ViewPreset::Bottom : ViewPreset::Top);
		if (pressed(ImGuiKey_Keypad1))
			SetViewPreset(ctrl ? ViewPreset::Back : ViewPreset::Front);
		if (pressed(ImGuiKey_Keypad3))
			SetViewPreset(ctrl ? ViewPreset::Left : ViewPreset::Right);
		if (pressed(ImGuiKey_Keypad5))
			ToggleProjection();
		if (pressed(ImGuiKey_Keypad9))
			ViewFromOtherSide();
		if (ImGui::IsKeyPressed(ImGuiKey_Keypad4))
			Orbit(-15.0, 0.0);
		if (ImGui::IsKeyPressed(ImGuiKey_Keypad6))
			Orbit(15.0, 0.0);
		if (ImGui::IsKeyPressed(ImGuiKey_Keypad8))
			Orbit(0.0, -15.0);
		if (ImGui::IsKeyPressed(ImGuiKey_Keypad2))
			Orbit(0.0, 15.0);
		if (pressed(ImGuiKey_KeypadDecimal))
			EditorTools::FrameSelection();
		if (pressed(ImGuiKey_Home) && !ctrl)
			FrameAll();

		// Letter keys: no modifiers, and not while the right button flies the camera (WASD) or the middle one drags it
		if (io.KeyCtrl || io.KeyAlt || io.KeyShift || io.KeySuper || ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle))
			return;
		switch (EditorPreferences::Get().keymap)
		{
			case NavigationKeymap::Max:
				if (pressed(ImGuiKey_T))
					SetViewPreset(ViewPreset::Top);
				if (pressed(ImGuiKey_F))
					SetViewPreset(ViewPreset::Front);
				if (pressed(ImGuiKey_L))
					SetViewPreset(ViewPreset::Left);
				if (pressed(ImGuiKey_P))
					SetProjection(CameraType::Perspective);
				if (pressed(ImGuiKey_U))
					SetProjection(CameraType::Orthographic);
				if (pressed(ImGuiKey_Z) && !EditorTools::FrameSelection())
					FrameAll();
				break;
			case NavigationKeymap::Maya:
				if (pressed(ImGuiKey_F))
					EditorTools::FrameSelection();
				if (pressed(ImGuiKey_A))
					FrameAll();
				break;
			default:
				if (pressed(ImGuiKey_F))
					EditorTools::FrameSelection();
				break;
		}
	}
}

// -------------------------------------------------------
