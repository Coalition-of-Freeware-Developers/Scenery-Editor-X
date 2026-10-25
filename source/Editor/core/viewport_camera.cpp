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
 * viewport_camera.cpp
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#include "viewport_camera.h"
#include <algorithm>
#include <cmath>
#include <numbers>

// -------------------------------------------------------

namespace SceneryEditorX::ViewportCamera
{
	namespace
	{
		constexpr double kPi = std::numbers::pi;
		constexpr double kHalfPi = kPi * 0.5;
		constexpr double MOUSE_RADIANS_PER_PIXEL = 0.005;
		constexpr double WHEEL_STEP = 1.2;				// Distance ratio per wheel notch
		constexpr double DOLLY_PER_PIXEL = 0.01;		// Log distance per pixel of a zoom drag
		constexpr double MIN_DISTANCE = 0.5;			// m
		constexpr double MAX_DISTANCE = 5000.0;			// m: half the far plane, beyond it the culling drops everything

		Point3 Add(const Point3 &a, const Point3 &b, const double scale = 1.0)
		{
			return { a.x + b.x * scale, a.y + b.y * scale, a.z + b.z * scale };
		}

		Point3 Cross(const Point3 &a, const Point3 &b)
		{
			return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
		}

		double WrapAngle(double radians)
		{
			radians = std::fmod(radians + kPi, 2.0 * kPi);
			return radians < 0.0 ? radians + kPi : radians - kPi;
		}

		double SmoothStep(const double t)
		{
			const double x = std::clamp(t, 0.0, 1.0);
			return x * x * (3.0 - 2.0 * x);
		}

		/* @brief The yaw / pitch a preset looks along: north up in plan views, as a map is drawn. */
		void PresetDirection(const ViewPreset preset, double &yaw, double &pitch)
		{
			switch (preset)
			{
				case ViewPreset::Top:		yaw = kPi;		pitch = -kHalfPi;	break;
				case ViewPreset::Bottom:	yaw = kPi;		pitch = kHalfPi;	break;
				case ViewPreset::Front:		yaw = kPi;		pitch = 0.0;		break;	// Looking north
				case ViewPreset::Back:		yaw = 0.0;		pitch = 0.0;		break;	// Looking south
				case ViewPreset::Right:		yaw = -kHalfPi;	pitch = 0.0;		break;	// From the east, looking west
				case ViewPreset::Left:		yaw = kHalfPi;	pitch = 0.0;		break;	// From the west, looking east
				default: break;
			}
		}

		ViewPreset OppositePreset(const ViewPreset preset)
		{
			switch (preset)
			{
				case ViewPreset::Top:		return ViewPreset::Bottom;
				case ViewPreset::Bottom:	return ViewPreset::Top;
				case ViewPreset::Front:		return ViewPreset::Back;
				case ViewPreset::Back:		return ViewPreset::Front;
				case ViewPreset::Left:		return ViewPreset::Right;
				case ViewPreset::Right:		return ViewPreset::Left;
				default:					return ViewPreset::Free;
			}
		}

		/* @brief What a press of a mouse button starts, with the modifiers held, in a keymap. */
		int Classify(const NavigationKeymap keymap, const ImGuiMouseButton button, const bool alt, const bool shift, const bool ctrl)
		{
			// 0 none, 1 orbit, 2 pan, 3 dolly, 4 look (Controller::Drag order)
			const bool left = button == ImGuiMouseButton_Left, middle = button == ImGuiMouseButton_Middle, right = button == ImGuiMouseButton_Right;
			switch (keymap)
			{
				case NavigationKeymap::Blender:
					// MMB orbits, Shift pans, Ctrl zooms; Alt+LMB does the same (emulated three-button mouse)
					if (middle || (left && alt))
						return shift ? 2 : ctrl ? 3 : 1;
					if (right && !alt)
						return 4;
					return 0;
				case NavigationKeymap::Max:
					// MMB pans, Alt+MMB orbits, Ctrl+Alt+MMB zooms
					if (middle)
						return alt && ctrl ? 3 : alt ? 1 : 2;
					if (right && !alt)
						return 4;
					return 0;
				case NavigationKeymap::Maya:
					// Alt+LMB tumbles, Alt+MMB tracks, Alt+RMB dollies; MMB alone pans and RMB alone looks around
					if (left && alt)
						return 1;
					if (middle)
						return 2;
					if (right)
						return alt ? 3 : 4;
					return 0;
				default:
					return 0;
			}
		}
	}

	bool GetPresetDirection(const ViewPreset preset, double &yaw, double &pitch)
	{
		if (preset == ViewPreset::Free)
			return false;
		PresetDirection(preset, yaw, pitch);
		return true;
	}

	const char *GetPresetName(const ViewPreset preset)
	{
		switch (preset)
		{
			case ViewPreset::Top:		return "Top";
			case ViewPreset::Bottom:	return "Bottom";
			case ViewPreset::Front:		return "Front";
			case ViewPreset::Back:		return "Back";
			case ViewPreset::Left:		return "Left";
			case ViewPreset::Right:		return "Right";
			default:					return "User";
		}
	}

	// -------------------------------------------------------

	Point3 Rig::Forward() const
	{
		const double cosPitch = std::cos(pitch);
		return { cosPitch * std::sin(yaw), std::sin(pitch), cosPitch * std::cos(yaw) };
	}

	Point3 Rig::Right() const
	{
		return { -std::cos(yaw), 0.0, std::sin(yaw) };
	}

	Point3 Rig::Up() const
	{
		return Cross(Right(), Forward());
	}

	Point3 Rig::Eye() const
	{
		return Add(pivot, Forward(), -distance);
	}

	double Rig::TanHalfFovY(const double aspect) const
	{
		return std::tan(fovHorizontal * 0.5) / std::max(aspect, 1e-3);
	}

	double Rig::OrthographicHeight(const double aspect) const
	{
		return 2.0 * distance * TanHalfFovY(aspect);
	}

	void Rig::KeepEye(const Point3 &eye)
	{
		pivot = Add(eye, Forward(), distance);
	}

	// -------------------------------------------------------

	bool Controller::Navigate(Rig &rig, const Input &input)
	{
		const ImGuiIO &io = ImGui::GetIO();
		const EditorPreferences &preferences = EditorPreferences::Get();
		if (input.locked)
		{
			m_Drag = Drag::None;
			return false;
		}

		// A press over the image starts the drag its keymap gives it; the drag then holds until that button is let go
		if (m_Drag == Drag::None && input.hovered && !input.blocked)
		{
			for (const ImGuiMouseButton button : { ImGuiMouseButton_Left, ImGuiMouseButton_Middle, ImGuiMouseButton_Right })
			{
				if (!ImGui::IsMouseClicked(button))
					continue;
				const int kind = Classify(preferences.keymap, button, io.KeyAlt, io.KeyShift, io.KeyCtrl);
				if (kind != 0)
				{
					m_Drag = static_cast<Drag>(kind);
					m_Button = button;
					Cancel();
					break;
				}
			}
		}
		else if (m_Drag != Drag::None && !ImGui::IsMouseDown(m_Button))
			m_Drag = Drag::None;

		bool changed = false;
		const double dx = io.MouseDelta.x, dy = io.MouseDelta.y;
		const double sensitivity = MOUSE_RADIANS_PER_PIXEL * preferences.orbitSpeed;
		const double imageHeight = std::max(1.0f, input.imageSize.y);
		if (m_Drag != Drag::None && (dx != 0.0 || dy != 0.0))
		{
			switch (m_Drag)
			{
				case Drag::Orbit:
				{
					// The scene turns with the mouse: drag right and it spins right, drag down and the view rises over it
					rig.yaw = WrapAngle(rig.yaw + (preferences.invertOrbitX ? dx : -dx) * sensitivity);
					rig.pitch = std::clamp(rig.pitch + (preferences.invertOrbitY ? dy : -dy) * sensitivity, -kHalfPi, kHalfPi);
					rig.preset = ViewPreset::Free;
					break;
				}
				case Drag::Pan:
				{
					// The point under the cursor follows it: world meters per pixel at the pivot
					const double metersPerPixel = rig.OrthographicHeight(input.aspect) / imageHeight;
					rig.pivot = Add(Add(rig.pivot, rig.Right(), -dx * metersPerPixel), rig.Up(), dy * metersPerPixel);
					break;
				}
				case Drag::Dolly:
				{
					// Right or up moves in
					const double amount = (dx - dy) * DOLLY_PER_PIXEL * preferences.zoomSpeed;
					rig.distance = std::clamp(rig.distance * std::exp(-amount), MIN_DISTANCE, MAX_DISTANCE);
					break;
				}
				case Drag::Look:
				{
					// Turning from where the camera is: toward the mouse
					const Point3 eye = rig.Eye();
					rig.yaw = WrapAngle(rig.yaw - dx * sensitivity);
					rig.pitch = std::clamp(rig.pitch + (preferences.invertLookY ? dy : -dy) * sensitivity, -kHalfPi, kHalfPi);
					rig.preset = ViewPreset::Free;
					rig.KeepEye(eye);
					break;
				}
				default:
					break;
			}
			changed = true;
		}

		// The wheel zooms toward the pivot (in FLY navigation it moves the camera along its view instead)
		if (input.hovered && !input.blocked && io.MouseWheel != 0.0f && m_Drag != Drag::Look)
		{
			if (rig.fly && !rig.orthographic)
			{
				const double step = std::clamp(rig.distance * 0.25, 1.0, 500.0) * io.MouseWheel * preferences.zoomSpeed;
				rig.pivot = Add(rig.pivot, rig.Forward(), step);
			}
			else
				rig.distance = std::clamp(rig.distance * std::pow(WHEEL_STEP, -io.MouseWheel * preferences.zoomSpeed), MIN_DISTANCE, MAX_DISTANCE);
			Cancel();
			changed = true;
		}

		// Flying: WASD / Q E while the right button looks around, the arrows and Page Up / Down whenever over the image.
		// Faster the higher the camera and the farther the pivot (scenery is kilometers across); Shift hurries, Ctrl creeps.
		if (!io.WantTextInput && (m_Drag == Drag::Look || input.hovered))
		{
			const bool flyKeys = m_Drag == Drag::Look;
			Point3 move{};
			const Point3 forward = rig.Forward(), right = rig.Right();
			const auto held = [](const ImGuiKey key) { return ImGui::IsKeyDown(key); };
			if ((flyKeys && held(ImGuiKey_W)) || held(ImGuiKey_UpArrow))		move = Add(move, forward);
			if ((flyKeys && held(ImGuiKey_S)) || held(ImGuiKey_DownArrow))		move = Add(move, forward, -1.0);
			if ((flyKeys && held(ImGuiKey_D)) || held(ImGuiKey_RightArrow))		move = Add(move, right);
			if ((flyKeys && held(ImGuiKey_A)) || held(ImGuiKey_LeftArrow))		move = Add(move, right, -1.0);
			if ((flyKeys && (held(ImGuiKey_E) || held(ImGuiKey_Space))) || held(ImGuiKey_PageUp))	move.y += 1.0;
			if ((flyKeys && held(ImGuiKey_Q)) || held(ImGuiKey_PageDown))		move.y -= 1.0;
			if (move.x != 0.0 || move.y != 0.0 || move.z != 0.0)
			{
				const Point3 eye = rig.Eye();
				const double altitude = std::abs(eye.y);
				double speed = std::clamp(5.0 + std::max(altitude, rig.distance * 0.5) * 1.2, 5.0, 4000.0);
				if (io.KeyShift)
					speed *= 4.0;
				if (io.KeyCtrl)
					speed *= 0.25;
				rig.pivot = Add(rig.pivot, move, speed * std::min(io.DeltaTime, 0.1f));
				Cancel();
				changed = true;
			}
		}
		return changed;
	}

	bool Controller::Animate(Rig &rig, const float deltaTime)
	{
		if (!m_Transition.active)
			return false;
		m_Transition.elapsed += deltaTime;
		const double t = SmoothStep(m_Transition.elapsed / std::max(m_Transition.duration, 1e-3));
		const Rig &a = m_Transition.from, &b = m_Transition.to;
		rig = b;
		if (t < 1.0)
		{
			rig.pivot = { a.pivot.x + (b.pivot.x - a.pivot.x) * t, a.pivot.y + (b.pivot.y - a.pivot.y) * t, a.pivot.z + (b.pivot.z - a.pivot.z) * t };
			rig.distance = std::exp(std::log(a.distance) + (std::log(b.distance) - std::log(a.distance)) * t);
			rig.yaw = WrapAngle(a.yaw + WrapAngle(b.yaw - a.yaw) * t);
			rig.pitch = a.pitch + (b.pitch - a.pitch) * t;
		}
		else
			m_Transition.active = false;
		return true;
	}

	void Controller::MoveTo(Rig &rig, const Rig &target)
	{
		if (!EditorPreferences::Get().smoothViews || m_Drag != Drag::None)
		{
			rig = target;
			m_Transition.active = false;
			return;
		}
		m_Transition.active = true;
		m_Transition.elapsed = 0.0;
		m_Transition.from = rig;
		m_Transition.to = target;
	}

	void Controller::Cancel()
	{
		m_Transition.active = false;
	}

	void Controller::SetPreset(Rig &rig, const ViewPreset preset)
	{
		Rig target = m_Transition.active ? m_Transition.to : rig;
		PresetDirection(preset, target.yaw, target.pitch);
		target.preset = preset;
		MoveTo(rig, target);
	}

	void Controller::SetOrthographic(Rig &rig, const bool orthographic)
	{
		rig.orthographic = orthographic;
	}

	void Controller::Frame(Rig &rig, const Point3 &center, const double radius, const double aspect)
	{
		Rig target = m_Transition.active ? m_Transition.to : rig;
		target.pivot = center;

		// Looking at or above the horizon in a free view: tilt down so the ground under the selection shows
		if (target.preset == ViewPreset::Free && target.Forward().y > -0.2)
			target.pitch = -0.6;

		// The narrower field of view decides; a little margin around
		const double tanHalf = std::min(std::tan(target.fovHorizontal * 0.5), target.TanHalfFovY(aspect));
		target.distance = std::clamp(std::max(radius, 0.5) / std::max(tanHalf, 1e-3) * 1.15, 2.0, MAX_DISTANCE);
		MoveTo(rig, target);
	}

	void Controller::Orbit(Rig &rig, const double yawDegrees, const double pitchDegrees)
	{
		Rig target = m_Transition.active ? m_Transition.to : rig;
		target.yaw = WrapAngle(target.yaw - yawDegrees * kPi / 180.0);	// Positive: the camera moves right around the pivot
		target.pitch = std::clamp(target.pitch - pitchDegrees * kPi / 180.0, -kHalfPi, kHalfPi);
		target.preset = ViewPreset::Free;
		MoveTo(rig, target);
	}

	void Controller::ViewFromOtherSide(Rig &rig)
	{
		Rig target = m_Transition.active ? m_Transition.to : rig;
		target.yaw = WrapAngle(target.yaw + kPi);
		target.pitch = -target.pitch;
		target.preset = OppositePreset(target.preset);
		if (target.preset != ViewPreset::Free)
			PresetDirection(target.preset, target.yaw, target.pitch);
		MoveTo(rig, target);
	}

	// -------------------------------------------------------

	const char *GetKeymapSummary(const NavigationKeymap keymap)
	{
		switch (keymap)
		{
			case NavigationKeymap::Blender:	return "Middle mouse or Alt + left mouse orbits, add Shift to pan, the wheel zooms";
			case NavigationKeymap::Max:		return "Alt + middle mouse orbits, middle mouse pans, the wheel zooms";
			case NavigationKeymap::Maya:	return "Alt + left mouse tumbles, Alt + middle mouse tracks, Alt + right mouse dollies";
			default:						return "";
		}
	}

	std::vector<Binding> GetBindings(const NavigationKeymap keymap)
	{
		std::vector<Binding> bindings;
		switch (keymap)
		{
			case NavigationKeymap::Blender:
				bindings = {
					{ "Orbit", "Middle mouse drag  /  Alt + left drag" },
					{ "Pan", "Shift + middle drag  /  Shift + Alt + left drag" },
					{ "Zoom", "Wheel  /  Ctrl + middle drag  /  Ctrl + Alt + left drag" },
					{ "Frame selection", "Numpad .  /  F" },
					{ "Frame everything", "Home" },
				};
				break;
			case NavigationKeymap::Max:
				bindings = {
					{ "Orbit", "Alt + middle drag" },
					{ "Pan", "Middle mouse drag" },
					{ "Zoom", "Wheel  /  Ctrl + Alt + middle drag" },
					{ "Frame selection (or everything)", "Z  /  Numpad ." },
					{ "Frame everything", "Home" },
					{ "Top / front / left view", "T  /  F  /  L" },
					{ "Perspective / orthographic", "P  /  U" },
				};
				break;
			case NavigationKeymap::Maya:
				bindings = {
					{ "Tumble (orbit)", "Alt + left drag" },
					{ "Track (pan)", "Alt + middle drag  /  middle drag" },
					{ "Dolly (zoom)", "Alt + right drag  /  wheel" },
					{ "Frame selection", "F  /  Numpad ." },
					{ "Frame everything", "A  /  Home" },
				};
				break;
			default:
				break;
		}

		// The same in every keymap
		bindings.insert(bindings.end(), {
			{ "Look around", "Right mouse drag" },
			{ "Fly", "Hold right mouse + W A S D, Q / E down / up (Shift faster, Ctrl slower)" },
			{ "Move", "Arrow keys, Page Up / Page Down" },
			{ "Top / bottom view", "Numpad 7  /  Ctrl + Numpad 7" },
			{ "Front / back view", "Numpad 1  /  Ctrl + Numpad 1" },
			{ "Right / left view", "Numpad 3  /  Ctrl + Numpad 3" },
			{ "Perspective / orthographic", "Numpad 5" },
			{ "Opposite side", "Numpad 9" },
			{ "Orbit in steps", "Numpad 4 / 6 (around), 8 / 2 (over)" },
		});
		return bindings;
	}
}

// -------------------------------------------------------
