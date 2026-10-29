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
 * viewport_camera.h
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <Editor/core/viewport_view.h>
#include <Editor/settings/editor_preferences.h>

// -------------------------------------------------------

namespace SceneryEditorX::ViewportCamera
{
	/* @brief The axis-aligned views (numpad 1 / 3 / 7 and their Ctrl variants), or a free one. */
	enum class ViewPreset : uint8_t
	{
		Free, Top, Bottom, Front, Back, Left, Right
	};

	const char *GetPresetName(ViewPreset preset);

	/* @brief The yaw / pitch (radians) a preset looks along, north up in plan views. False for Free. */
	bool GetPresetDirection(ViewPreset preset, double &yaw, double &pitch);

	/**
	 * @struct Rig
	 * @brief A viewport camera: the pivot it orbits, its distance and direction (yaw / pitch in radians, forward =
	 * (cos p sin y, sin p, cos p cos y), +Y up, -Z north), its field of view and projection. Orthographic views show
	 * 2 * distance * tan(vertical fov / 2) meters top to bottom, so switching projection keeps the framing at the pivot.
	 */
	struct Rig
	{
		Point3 pivot;
		double distance = 10.0;
		double yaw = 0.0;
		double pitch = 0.0;
		double fovHorizontal = 1.0471975511965976;	// 60 degrees
		bool orthographic = false;
		bool fly = false;							// FLY navigation: the wheel moves the camera instead of zooming
		ViewPreset preset = ViewPreset::Free;

		[[nodiscard]] Point3 Forward() const;
		[[nodiscard]] Point3 Right() const;			// From the yaw alone, so straight-down views keep their bearing
		[[nodiscard]] Point3 Up() const;
		[[nodiscard]] Point3 Eye() const;
		[[nodiscard]] double TanHalfFovY(double aspect) const;
		[[nodiscard]] double OrthographicHeight(double aspect) const;

		/* @brief Moves the pivot so the eye stays where it is (after the direction changed around the eye). */
		void KeepEye(const Point3 &eye);
	};

	/* @brief What the viewport tells the controller about this frame. */
	struct Input
	{
		bool hovered = false;	// The mouse is over the image
		bool blocked = false;	// A tool, gizmo or menu has the mouse: no new camera drag starts
		bool locked = false;	// --lock-camera: nothing moves the camera
		ImVec2 imageSize{ 1.0f, 1.0f };
		double aspect = 1.0;
	};

	/**
	 * @class Controller
	 * @brief Turns mouse and keyboard input into camera moves with the user's keymap (EditorPreferences), and glides
	 * between views. One per viewport; it keeps no camera state of its own besides the drag and the transition.
	 */
	class Controller
	{
	public:
		/* @brief Mouse drags, the wheel and the fly keys. True when the rig changed. */
		bool Navigate(Rig &rig, const Input &input);

		/* @brief Advances a running view transition. True when the rig changed. */
		bool Animate(Rig &rig, float deltaTime);

		[[nodiscard]] bool IsDragging() const { return m_Drag != Drag::None; }
		[[nodiscard]] bool IsAnimating() const { return m_Transition.active; }

		/* @brief Looks along a preset axis around the current pivot (Free leaves the direction alone). */
		void SetPreset(Rig &rig, ViewPreset preset);

		/* @brief Switches projection, keeping the framing at the pivot. */
		static void SetOrthographic(Rig &rig, bool orthographic);

		/* @brief Fits a sphere in view without turning (tilting down if the camera looks at the horizon). */
		void Frame(Rig &rig, const Point3 &center, double radius, double aspect);

		/* @brief Orbits by a step (numpad 2 / 4 / 6 / 8): degrees of yaw to the right, of pitch upward. */
		void Orbit(Rig &rig, double yawDegrees, double pitchDegrees);

		/* @brief Looks from the opposite side (numpad 9): top becomes bottom, front becomes back. */
		void ViewFromOtherSide(Rig &rig);

		/* @brief Glides (or jumps, without smooth views) to a rig. */
		void MoveTo(Rig &rig, const Rig &target);

		void Cancel();

	private:
		enum class Drag : uint8_t { None, Orbit, Pan, Dolly, Look };

		struct Transition
		{
			bool active = false;
			double elapsed = 0.0;
			double duration = 0.25;
			Rig from;
			Rig to;
		};

		Drag m_Drag = Drag::None;
		int m_Button = 0;		// The mouse button holding the drag
		Transition m_Transition;
	};

	/* @brief One line of a keymap's reference: what it does and the keys / buttons that do it. */
	struct Binding
	{
		std::string action;
		std::string keys;
	};

	/* @brief The navigation bindings of a keymap, for the welcome screen and the Controls window. */
	std::vector<Binding> GetBindings(NavigationKeymap keymap);

	/* @brief A one-line summary of a keymap (orbit / pan / zoom). */
	const char *GetKeymapSummary(NavigationKeymap keymap);
}

// -------------------------------------------------------
