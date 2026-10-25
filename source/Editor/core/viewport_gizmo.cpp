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
 * viewport_gizmo.cpp
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 */
#include "viewport_gizmo.h"
#include "editor_theme.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::ViewportGizmo
{
	namespace
	{
		constexpr double PI = 3.14159265358979323846;
		constexpr double DEGREES = 180.0 / PI;

		// Pixels at 100% UI scale
		constexpr float SIZE = 110.0f;				// How far the axes reach on screen
		constexpr float CENTER_RADIUS = 9.0f;		// Move: the center circle; scale: the center box (a little smaller)
		constexpr float AXIS_START = 0.17f;			// Axes start this part of the way out, clear of the center handle
		constexpr float ARROW_LENGTH = 21.0f;
		constexpr float ARROW_HALF = 7.5f;
		constexpr float BOX_HALF = 6.0f;			// Scale: the boxes at the axes' ends
		constexpr double PLANE_NEAR = 0.26;			// Plane squares span this part of the axes' reach, both ways
		constexpr double PLANE_FAR = 0.46;
		constexpr double RING_RADIUS = 0.95;		// Rotate: the axis rings
		constexpr float VIEW_RING_RADIUS = 1.16f;	// Rotate: the view ring, around them
		constexpr float LINE_WIDTH = 3.5f;
		constexpr float RING_WIDTH = 3.0f;
		constexpr float HIT = 8.0f;					// How near a line the cursor grabs it
		constexpr int RING_SEGMENTS = 96;
		constexpr float DIMMED = 0.4f;				// The other handles while one is dragged

		// sRGB, converted for the swapchain when drawn
		constexpr ImVec4 AXIS_COLORS[3] = { { 0.94f, 0.25f, 0.31f, 1.0f }, { 0.51f, 0.82f, 0.16f, 1.0f }, { 0.20f, 0.53f, 0.97f, 1.0f } };
		constexpr ImVec4 TINT_LIT = { 1.0f, 0.82f, 0.18f, 1.0f };
		constexpr ImVec4 TINT_NEUTRAL = { 0.93f, 0.93f, 0.93f, 1.0f };
		constexpr ImVec4 TINT_SHADOW = { 0.0f, 0.0f, 0.0f, 1.0f };

		constexpr Handle AXIS_HANDLES[3] = { AxisX, AxisY, AxisZ };
		constexpr Handle PLANE_HANDLES[3] = { PlaneYZ, PlaneXZ, PlaneXY };	// By their normal
		constexpr const char *AXIS_NAMES[3] = { "X", "Y", "Z" };
		constexpr const char *PLANE_NAMES[3] = { "YZ", "XZ", "XY" };

		// -------------------------------------------------------

		Point3 Add(const Point3 &a, const Point3 &b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
		Point3 Sub(const Point3 &a, const Point3 &b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
		Point3 Mul(const Point3 &a, const double s) { return { a.x * s, a.y * s, a.z * s }; }
		double Dot(const Point3 &a, const Point3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
		Point3 Cross(const Point3 &a, const Point3 &b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
		double Length(const Point3 &a) { return std::sqrt(Dot(a, a)); }

		Point3 Normalize(const Point3 &a)
		{
			const double length = Length(a);
			return length > 1e-12 ? Mul(a, 1.0 / length) : Point3{ 0.0, 0.0, 0.0 };
		}

		/* @brief The part of a vector across an axis (unit). */
		Point3 Across(const Point3 &v, const Point3 &axis) { return Sub(v, Mul(axis, Dot(v, axis))); }

		ImVec2 Plus(const ImVec2 &a, const ImVec2 &b) { return { a.x + b.x, a.y + b.y }; }
		ImVec2 Minus(const ImVec2 &a, const ImVec2 &b) { return { a.x - b.x, a.y - b.y }; }
		ImVec2 Times(const ImVec2 &a, const float s) { return { a.x * s, a.y * s }; }
		float Dot2(const ImVec2 &a, const ImVec2 &b) { return a.x * b.x + a.y * b.y; }
		float Length2(const ImVec2 &a) { return std::sqrt(Dot2(a, a)); }

		float SegmentDistance(const ImVec2 &p, const ImVec2 &a, const ImVec2 &b)
		{
			const ImVec2 ab = Minus(b, a);
			const float length2 = Dot2(ab, ab);
			const float t = length2 > 1e-6f ? std::clamp(Dot2(Minus(p, a), ab) / length2, 0.0f, 1.0f) : 0.0f;
			return Length2(Minus(p, Plus(a, Times(ab, t))));
		}

		bool InsideQuad(const ImVec2 &p, const ImVec2 (&quad)[4])
		{
			// Convex either way round: the point is on the same side of every edge
			int sign = 0;
			for (int i = 0; i < 4; ++i)
			{
				const ImVec2 &a = quad[i], &b = quad[(i + 1) % 4];
				const float cross = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
				const int side = cross > 0.0f ? 1 : (cross < 0.0f ? -1 : 0);
				if (side == 0)
					continue;
				if (sign != 0 && side != sign)
					return false;
				sign = side;
			}
			return sign != 0;
		}

		double Snap(const double value, const double step)
		{
			return step > 1e-9 ? std::round(value / step) * step : value;
		}

		/* @brief An angle brought into -pi..pi. */
		double Wrap(double angle)
		{
			while (angle > PI)
				angle -= 2.0 * PI;
			while (angle < -PI)
				angle += 2.0 * PI;
			return angle;
		}

		// -------------------------------------------------------

		Matrix Multiply(const Matrix &a, const Matrix &b)
		{
			Matrix out;
			for (int r = 0; r < 4; ++r)
			{
				for (int c = 0; c < 4; ++c)
				{
					double sum = 0.0;
					for (int k = 0; k < 4; ++k)
						sum += a.m[r][k] * b.m[k][c];
					out.m[r][c] = sum;
				}
			}
			return out;
		}

		Matrix Translation(const Point3 &t)
		{
			Matrix out;
			out.m[3][0] = t.x;
			out.m[3][1] = t.y;
			out.m[3][2] = t.z;
			return out;
		}

		/* @brief A linear transform applied about a pivot: T(-pivot) * linear * T(pivot). */
		Matrix AboutPivot(const Matrix &linear, const Point3 &pivot)
		{
			return Multiply(Multiply(Translation(Mul(pivot, -1.0)), linear), Translation(pivot));
		}

		/* @brief Rotation about a unit axis (right-handed, so about +Y it turns counter-clockwise seen from above). */
		Matrix Rotation(const Point3 &axis, const double angle)
		{
			// The transpose of the column-vector Rodrigues matrix: c I + (1 - c) a a^T - s [a]x
			const double c = std::cos(angle), s = std::sin(angle), t = 1.0 - c;
			const double x = axis.x, y = axis.y, z = axis.z;
			Matrix out;
			out.m[0][0] = c + t * x * x;		out.m[0][1] = t * x * y + s * z;	out.m[0][2] = t * x * z - s * y;
			out.m[1][0] = t * x * y - s * z;	out.m[1][1] = c + t * y * y;		out.m[1][2] = t * y * z + s * x;
			out.m[2][0] = t * x * z + s * y;	out.m[2][1] = t * y * z - s * x;	out.m[2][2] = c + t * z * z;
			return out;
		}

		/* @brief Scale along the frame's axes: G^T * S * G (G's rows are the axes). */
		Matrix ScaleAlong(const Frame &frame, const double (&scale)[3])
		{
			Matrix out;
			for (int r = 0; r < 3; ++r)
			{
				for (int c = 0; c < 3; ++c)
				{
					double sum = 0.0;
					for (int k = 0; k < 3; ++k)
					{
						const Point3 &axis = frame.axes[k];
						const double row = r == 0 ? axis.x : (r == 1 ? axis.y : axis.z);
						const double column = c == 0 ? axis.x : (c == 1 ? axis.y : axis.z);
						sum += row * scale[k] * column;
					}
					out.m[r][c] = sum;
				}
			}
			return out;
		}

		// -------------------------------------------------------

		struct Ring
		{
			ImVec2 points[RING_SEGMENTS + 1];
			bool visible[RING_SEGMENTS + 1] = {};	// The point projected
			bool front[RING_SEGMENTS] = {};		// The segment is on the camera's side of the gizmo
		};

		/* @brief The gizmo as drawn this frame: screen positions of its handles. */
		struct Geometry
		{
			bool valid = false;
			float dpi = 1.0f;
			float size = SIZE;			// Pixels
			double length = 1.0;		// Meters the axes reach at the pivot
			Point3 toEye;				// From the pivot toward the camera
			ImVec2 center;
			ImVec2 starts[3];
			ImVec2 tips[3];
			ImVec2 directions[3];		// Unit, on screen
			float axisAlpha[3] = {};	// Axes pointing at the camera fade out
			ImVec2 planes[3][4];		// By their normal
			double planeSigns[3][2] = {};	// Which way along their two axes the squares sit (toward the camera)
			float planeAlpha[3] = {};	// Planes seen edge-on fade out
			std::array<Ring, 3> rings;
			float ringRadius = 0.0f;	// Pixels, about
			float viewRingRadius = 0.0f;
		};

		Point3 TowardEye(const ViewportView &view, const Point3 &pivot)
		{
			if (view.orthographic)
				return Mul(view.forward, -1.0);
			const Point3 toEye = Normalize(Sub(view.eye, pivot));
			return Length(toEye) > 0.5 ? toEye : Mul(view.forward, -1.0);
		}

		Geometry Build(const ViewportView &view, const Frame &frame)
		{
			Geometry g;
			g.dpi = ImGui::GetStyle().FontScaleDpi;
			g.size = SIZE * g.dpi;
			if (!view.valid || !view.WorldToScreen(frame.pivot, g.center))
				return g;
			const double pixelsPerMeter = view.PixelsPerMeter(frame.pivot);
			if (pixelsPerMeter <= 1e-9)
				return g;
			g.length = g.size / pixelsPerMeter;
			g.toEye = TowardEye(view, frame.pivot);

			for (int i = 0; i < 3; ++i)
			{
				g.axisAlpha[i] = 0.0f;
				g.directions[i] = ImVec2(1.0f, 0.0f);
				g.tips[i] = g.starts[i] = g.center;
				ImVec2 tip;
				if (!view.WorldToScreen(Add(frame.pivot, Mul(frame.axes[i], g.length)), tip))
					continue;
				const ImVec2 along = Minus(tip, g.center);
				const float reach = Length2(along);
				if (reach < 1e-3f)
					continue;
				g.tips[i] = tip;
				g.directions[i] = Times(along, 1.0f / reach);
				g.starts[i] = Plus(g.center, Times(along, AXIS_START));
				g.axisAlpha[i] = std::clamp((reach / g.size - 0.12f) / 0.16f, 0.0f, 1.0f);
			}

			for (int n = 0; n < 3; ++n)
			{
				const int u = (n + 1) % 3, v = (n + 2) % 3;
				const double su = Dot(frame.axes[u], g.toEye) >= 0.0 ? 1.0 : -1.0;
				const double sv = Dot(frame.axes[v], g.toEye) >= 0.0 ? 1.0 : -1.0;
				g.planeSigns[n][0] = su;
				g.planeSigns[n][1] = sv;
				static constexpr double CORNERS[4][2] = { { PLANE_NEAR, PLANE_NEAR }, { PLANE_FAR, PLANE_NEAR }, { PLANE_FAR, PLANE_FAR }, { PLANE_NEAR, PLANE_FAR } };
				bool projected = true;
				for (int k = 0; k < 4; ++k)
				{
					const Point3 offset = Add(Mul(frame.axes[u], su * CORNERS[k][0] * g.length), Mul(frame.axes[v], sv * CORNERS[k][1] * g.length));
					projected &= view.WorldToScreen(Add(frame.pivot, offset), g.planes[n][k]);
				}
				const double facing = std::abs(Dot(frame.axes[n], g.toEye));
				g.planeAlpha[n] = projected ? static_cast<float>(std::clamp((facing - 0.12) / 0.15, 0.0, 1.0)) : 0.0f;
			}

			const double radius = g.length * RING_RADIUS;
			g.ringRadius = g.size * static_cast<float>(RING_RADIUS);
			g.viewRingRadius = g.size * VIEW_RING_RADIUS;
			for (int i = 0; i < 3; ++i)
			{
				const Point3 &u = frame.axes[(i + 1) % 3], &v = frame.axes[(i + 2) % 3];
				Ring &ring = g.rings[i];
				for (int k = 0; k <= RING_SEGMENTS; ++k)
				{
					const double t = 2.0 * PI * k / RING_SEGMENTS;
					const Point3 offset = Add(Mul(u, std::cos(t) * radius), Mul(v, std::sin(t) * radius));
					ring.visible[k] = view.WorldToScreen(Add(frame.pivot, offset), ring.points[k]);
				}
				for (int k = 0; k < RING_SEGMENTS; ++k)
				{
					const double t = 2.0 * PI * (k + 0.5) / RING_SEGMENTS;
					const Point3 middle = Add(Mul(u, std::cos(t)), Mul(v, std::sin(t)));
					ring.front[k] = ring.visible[k] && ring.visible[k + 1] && Dot(middle, g.toEye) > -0.03;
				}
			}
			g.valid = true;
			return g;
		}

		/**
		 * @brief The ring angle (radians from the first in-plane axis) nearest a screen position: among its front
		 * segments, or all of them when whole (a ring shown alone can be grabbed anywhere).
		 */
		bool NearestOnRing(const Ring &ring, const ImVec2 &mouse, const bool whole, double &angle, float &distance)
		{
			distance = 1e30f;
			for (int k = 0; k < RING_SEGMENTS; ++k)
			{
				if (!(ring.front[k] || (whole && ring.visible[k] && ring.visible[k + 1])))
					continue;
				const float d = SegmentDistance(mouse, ring.points[k], ring.points[k + 1]);
				if (d < distance)
				{
					distance = d;
					angle = 2.0 * PI * (k + 0.5) / RING_SEGMENTS;
				}
			}
			return distance < 1e29f;
		}

		// -------------------------------------------------------

		struct State
		{
			int frameCount = -1;		// When Manipulate() last ran
			uint32_t owner = 0;
			Frame frame;				// At rest: the caller's; while dragging: the start's
			Options options;
			Geometry rest;				// Where the handles are for hit tests (the start's while dragging)
			Geometry shown;				// What is drawn (follows a move)
			Frame shownFrame;
			Handle hovered = HandleNone;

			Handle active = HandleNone;
			bool waitRelease = false;	// Cancelled with the button down: ignore the mouse until it comes up
			ImVec2 pressMouse;
			Point3 planeNormal;			// Move: the plane the cursor slides on
			Point3 pressHit;
			bool flatten = false;		// Move: drop the vertical part (ground-only moves dragged in a side view)
			Point3 translation;
			Point3 axis;				// Rotate: about this
			bool tangent = false;		// Rotate: the ring is edge-on, the drag runs along its screen tangent instead
			ImVec2 tangentScreen;
			Point3 lastVector;
			double lastAngle = 0.0;		// View ring: screen angle last frame
			Point3 startVector;			// Rotate: where on the ring the drag began (unit, in its plane)
			double startScreenAngle = 0.0;
			double angle = 0.0;			// Rotate: so far, unsnapped
			double rotation = 0.0;		// Rotate: snapped
			ImVec2 scaleDirection;
			float scaleStart = 1.0f;
			double scale[3] = { 1.0, 1.0, 1.0 };
			Matrix delta;
			std::string label;
		};

		State &S()
		{
			static State state;
			return state;
		}

		int AxisOf(const Handle handle)
		{
			for (int i = 0; i < 3; ++i)
				if (handle == AXIS_HANDLES[i])
					return i;
			return -1;
		}

		int PlaneOf(const Handle handle)
		{
			for (int n = 0; n < 3; ++n)
				if (handle == PLANE_HANDLES[n])
					return n;
			return -1;
		}

		/* @brief A single axis ring (headings only): nothing to mistake it for, so it shows and grabs all the way round. */
		bool SingleRing(const Options &options)
		{
			const uint32_t rings = options.handles & Axes;
			return rings != 0 && (rings & (rings - 1)) == 0;
		}

		Handle HitTest(const Geometry &g, const Options &options, const ImVec2 &mouse)
		{
			if (!g.valid)
				return HandleNone;
			const float hit = HIT * g.dpi;

			if (options.mode == Mode::Rotate)
			{
				Handle best = HandleNone;
				float bestDistance = hit;
				for (int i = 0; i < 3; ++i)
				{
					double angle = 0.0;
					float distance = 0.0f;
					if ((options.handles & AXIS_HANDLES[i]) && NearestOnRing(g.rings[i], mouse, SingleRing(options), angle, distance) && distance < bestDistance)
					{
						bestDistance = distance;
						best = AXIS_HANDLES[i];
					}
				}
				if (best == HandleNone && (options.handles & ViewRing) && std::abs(Length2(Minus(mouse, g.center)) - g.viewRingRadius) < hit)
					best = ViewRing;
				return best;
			}

			const float centerRadius = CENTER_RADIUS * g.dpi + 2.0f * g.dpi;
			if ((options.handles & Center) && Length2(Minus(mouse, g.center)) <= centerRadius)
				return Center;
			for (int n = 0; n < 3; ++n)
				if ((options.handles & PLANE_HANDLES[n]) && g.planeAlpha[n] > 0.3f && InsideQuad(mouse, g.planes[n]))
					return PLANE_HANDLES[n];

			Handle best = HandleNone;
			float bestDistance = hit;
			for (int i = 0; i < 3; ++i)
			{
				if (!(options.handles & AXIS_HANDLES[i]) || g.axisAlpha[i] < 0.3f)
					continue;
				float distance = SegmentDistance(mouse, g.starts[i], g.tips[i]);
				// The arrow head or box at the end is wider than the line
				const float headRadius = (options.mode == Mode::Translate ? ARROW_HALF + 3.0f : BOX_HALF + 3.0f) * g.dpi;
				const ImVec2 head = options.mode == Mode::Translate ? Minus(g.tips[i], Times(g.directions[i], ARROW_LENGTH * 0.4f * g.dpi)) : g.tips[i];
				if (Length2(Minus(mouse, head)) < headRadius)
					distance = 0.0f;
				if (distance < bestDistance)
				{
					bestDistance = distance;
					best = AXIS_HANDLES[i];
				}
			}
			return best;
		}

		/* @brief The direction from the eye through the pivot. */
		Point3 ViewDirection(const ViewportView &view, const Point3 &pivot)
		{
			return Mul(TowardEye(view, pivot), -1.0);
		}

		bool IntersectPlane(const ViewportView &view, const ImVec2 &mouse, const Point3 &point, const Point3 &normal, Point3 &hit)
		{
			Point3 origin, direction;
			view.ScreenRay(mouse, origin, direction);
			const double facing = Dot(direction, normal);
			if (std::abs(facing) < 1e-6)
				return false;
			const double t = Dot(Sub(point, origin), normal) / facing;
			if (t <= 0.0)
				return false;
			hit = Add(origin, Mul(direction, t));
			return true;
		}

		bool BeginDrag(State &s, const ViewportView &view, const ImVec2 &mouse)
		{
			const Frame &frame = s.frame;
			const Geometry &g = s.rest;
			const Point3 &pivot = frame.pivot;
			const Point3 viewDirection = ViewDirection(view, pivot);
			const Handle handle = s.hovered;
			const int axis = AxisOf(handle);
			const int plane = PlaneOf(handle);

			s.translation = {};
			s.angle = s.rotation = 0.0;
			s.scale[0] = s.scale[1] = s.scale[2] = 1.0;
			s.delta = Matrix();
			s.flatten = false;
			s.tangent = false;
			s.pressMouse = mouse;
			s.label.clear();

			switch (s.options.mode)
			{
				case Mode::Translate:
				{
					if (axis >= 0)
					{
						// Slide on the plane through the axis that faces the camera most
						s.planeNormal = Normalize(Across(viewDirection, frame.axes[axis]));
						if (Length(s.planeNormal) < 0.5)
							return false;
					}
					else if (plane >= 0)
						s.planeNormal = frame.axes[plane];
					else
					{
						// Across the ground, unless it is seen edge-on: then across the view (ground-only moves drop the height)
						const Point3 &up = frame.axes[1];
						if (std::abs(Dot(up, viewDirection)) >= 0.25)
							s.planeNormal = up;
						else
						{
							s.planeNormal = viewDirection;
							s.flatten = !(s.options.handles & AxisY);
						}
					}
					return IntersectPlane(view, mouse, pivot, s.planeNormal, s.pressHit);
				}

				case Mode::Rotate:
				{
					if (handle == ViewRing)
					{
						s.axis = Mul(viewDirection, -1.0);
						s.lastAngle = s.startScreenAngle = std::atan2(-(mouse.y - g.center.y), mouse.x - g.center.x);
						return true;
					}
					if (axis < 0)
						return false;
					s.axis = frame.axes[axis];
					const Point3 &u = frame.axes[(axis + 1) % 3], &v = frame.axes[(axis + 2) % 3];
					Point3 hit;
					if (std::abs(Dot(s.axis, viewDirection)) > 0.25 && IntersectPlane(view, mouse, pivot, s.axis, hit) && Length(Across(Sub(hit, pivot), s.axis)) > g.length * 1e-3)
					{
						s.lastVector = Across(Sub(hit, pivot), s.axis);
						s.startVector = Normalize(s.lastVector);
						return true;
					}
					// Edge-on: drag along the ring's tangent where it was grabbed
					double angle = 0.0;
					float distance = 0.0f;
					if (!NearestOnRing(g.rings[axis], mouse, SingleRing(s.options), angle, distance))
						return false;
					s.startVector = Add(Mul(u, std::cos(angle)), Mul(v, std::sin(angle)));
					const Point3 onRing = Add(pivot, Mul(s.startVector, g.length * RING_RADIUS));
					const Point3 ahead = Add(onRing, Mul(Cross(s.axis, s.startVector), g.length * 0.1));
					ImVec2 a, b;
					if (!view.WorldToScreen(onRing, a) || !view.WorldToScreen(ahead, b))
						return false;
					const ImVec2 along = Minus(b, a);
					const float reach = Length2(along);
					s.tangent = true;
					s.tangentScreen = reach > 1e-3f ? Times(along, 1.0f / reach) : ImVec2(1.0f, 0.0f);
					return true;
				}

				case Mode::Scale:
				{
					if (handle == Center)
						return true;
					if (axis >= 0)
						s.scaleDirection = g.directions[axis];
					else if (plane >= 0)
					{
						const int u = (plane + 1) % 3, v = (plane + 2) % 3;
						const double su = g.planeSigns[plane][0], sv = g.planeSigns[plane][1];
						const double middle = (PLANE_NEAR + PLANE_FAR) * 0.5 * g.length;
						ImVec2 corner;
						if (!view.WorldToScreen(Add(pivot, Add(Mul(frame.axes[u], su * middle), Mul(frame.axes[v], sv * middle))), corner))
							return false;
						const ImVec2 along = Minus(corner, g.center);
						const float reach = Length2(along);
						if (reach < 1e-3f)
							return false;
						s.scaleDirection = Times(along, 1.0f / reach);
					}
					else
						return false;
					s.scaleStart = std::max(4.0f * g.dpi, Dot2(Minus(mouse, g.center), s.scaleDirection));
					return true;
				}
			}
			return false;
		}

		void UpdateDrag(State &s, const ViewportView &view, const ImVec2 &mouse)
		{
			const Frame &frame = s.frame;
			const Geometry &g = s.rest;
			const Point3 &pivot = frame.pivot;
			const Options &options = s.options;
			const int axis = AxisOf(s.active);
			const int plane = PlaneOf(s.active);
			char text[96] = {};

			switch (options.mode)
			{
				case Mode::Translate:
				{
					Point3 hit;
					// A grazing plane sends the hit off toward the horizon: keep the last good position then
					if (!IntersectPlane(view, mouse, pivot, s.planeNormal, hit) || Length(Sub(hit, pivot)) > g.length * 4000.0)
						break;
					Point3 moved = Sub(hit, s.pressHit);
					if (axis >= 0)
					{
						const double t = options.snap ? Snap(Dot(moved, frame.axes[axis]), options.snapMove) : Dot(moved, frame.axes[axis]);
						s.translation = Mul(frame.axes[axis], t);
						snprintf(text, sizeof(text), "%s  %+.2f m", AXIS_NAMES[axis], t);
					}
					else
					{
						if (s.flatten)
							moved = Across(moved, frame.axes[1]);
						s.translation = {};
						for (int k = 0; k < 3; ++k)
						{
							if (plane == k)
								continue;
							const double t = options.snap ? Snap(Dot(moved, frame.axes[k]), options.snapMove) : Dot(moved, frame.axes[k]);
							s.translation = Add(s.translation, Mul(frame.axes[k], t));
						}
						if (plane >= 0)
							snprintf(text, sizeof(text), "%s  %.2f m", PLANE_NAMES[plane], Length(s.translation));
						else
							snprintf(text, sizeof(text), "%.2f m", Length(s.translation));
					}
					s.delta = Translation(s.translation);
					break;
				}

				case Mode::Rotate:
				{
					if (s.active == ViewRing)
					{
						const double angle = std::atan2(-(mouse.y - g.center.y), mouse.x - g.center.x);
						s.angle += Wrap(angle - s.lastAngle);
						s.lastAngle = angle;
					}
					else if (s.tangent)
						s.angle = Dot2(Minus(mouse, s.pressMouse), s.tangentScreen) / std::max(1.0f, g.ringRadius);
					else
					{
						Point3 hit;
						if (IntersectPlane(view, mouse, pivot, s.axis, hit))
						{
							const Point3 vector = Across(Sub(hit, pivot), s.axis);
							if (Length(vector) > g.length * 1e-3)
							{
								s.angle += std::atan2(Dot(s.axis, Cross(s.lastVector, vector)), Dot(s.lastVector, vector));
								s.lastVector = vector;
							}
						}
					}
					s.rotation = options.snap ? Snap(s.angle * DEGREES, options.snapAngle) / DEGREES : s.angle;
					s.delta = AboutPivot(Rotation(s.axis, s.rotation), pivot);
					snprintf(text, sizeof(text), "%s  %+.1f\xC2\xB0", s.active == ViewRing ? "View" : AXIS_NAMES[std::max(0, axis)], s.rotation * DEGREES);
					break;
				}

				case Mode::Scale:
				{
					double factor = 1.0;
					if (s.active == Center)
						factor = 1.0 + ((mouse.x - s.pressMouse.x) - (mouse.y - s.pressMouse.y)) / std::max(1.0f, g.size);
					else
						factor = Dot2(Minus(mouse, g.center), s.scaleDirection) / s.scaleStart;
					factor = std::max(0.01, factor);
					if (options.snap && options.snapScale > 1e-6)
						factor = std::max(options.snapScale, Snap(factor, options.snapScale));
					for (int k = 0; k < 3; ++k)
					{
						bool scaled = false;
						if (s.active == Center)
							scaled = !(options.uniformGround && k == 1);
						else if (axis >= 0)
							scaled = k == axis;
						else if (plane >= 0)
							scaled = k != plane;
						s.scale[k] = scaled ? factor : 1.0;
					}
					s.delta = AboutPivot(ScaleAlong(frame, s.scale), pivot);
					const char *name = axis >= 0 ? AXIS_NAMES[axis] : (plane >= 0 ? PLANE_NAMES[plane] : (options.uniformGround ? "XZ" : "XYZ"));
					snprintf(text, sizeof(text), "%s  x%.3f", name, factor);
					break;
				}
			}
			if (text[0] != '\0')
				s.label = text;
		}

		// -------------------------------------------------------

		ImU32 Color(const ImVec4 &color, const float alpha)
		{
			return EditorTheme::ToU32(ImVec4(color.x, color.y, color.z, std::clamp(alpha, 0.0f, 1.0f) * color.w));
		}

		/* @brief A handle's color: lit when hovered or dragged, dimmed while another one is dragged. */
		ImU32 HandleColor(const State &s, const Handle handle, const ImVec4 &base, const float alpha)
		{
			if (s.active != HandleNone)
				return handle == s.active ? Color(TINT_LIT, 1.0f) : Color(base, alpha * DIMMED);
			return handle == s.hovered ? Color(TINT_LIT, 1.0f) : Color(base, alpha);
		}

		void ShadowLine(ImDrawList *drawList, const ImVec2 &a, const ImVec2 &b, const ImU32 color, const float width, const float alpha)
		{
			drawList->AddLine(a, b, Color(TINT_SHADOW, 0.45f * alpha), width + 2.0f);
			drawList->AddLine(a, b, color, width);
		}

		void DrawPlanes(const State &s, ImDrawList *drawList, const Geometry &g)
		{
			for (int n = 0; n < 3; ++n)
			{
				if (!(s.options.handles & PLANE_HANDLES[n]) || g.planeAlpha[n] <= 0.0f)
					continue;
				const bool lit = (s.active == HandleNone && s.hovered == PLANE_HANDLES[n]) || s.active == PLANE_HANDLES[n];
				const float dim = s.active != HandleNone && !lit ? DIMMED : 1.0f;
				const ImVec4 &color = lit ? TINT_LIT : AXIS_COLORS[n];
				drawList->AddConvexPolyFilled(g.planes[n], 4, Color(color, (lit ? 0.5f : 0.3f) * g.planeAlpha[n] * dim));
				drawList->AddPolyline(g.planes[n], 4, Color(color, 0.95f * g.planeAlpha[n] * dim), ImDrawFlags_Closed, 1.5f * g.dpi);
			}
		}

		/* @brief Axes back to front, so the nearer ones are drawn over the farther ones. */
		std::array<int, 3> AxisOrder(const Geometry &g, const Frame &frame)
		{
			std::array<int, 3> order = { 0, 1, 2 };
			std::sort(order.begin(), order.end(), [&](const int a, const int b) { return Dot(frame.axes[a], g.toEye) < Dot(frame.axes[b], g.toEye); });
			return order;
		}

		void DrawTranslate(const State &s, ImDrawList *drawList, const Geometry &g, const Frame &frame)
		{
			const float dpi = g.dpi;
			if (s.active != HandleNone)
			{
				// Where it started, and the axis it slides along
				const Geometry &start = s.rest;
				const int axis = AxisOf(s.active);
				if (axis >= 0)
				{
					const ImVec2 &direction = start.directions[axis];
					const float extent = 8000.0f * dpi;
					drawList->AddLine(Minus(start.center, Times(direction, extent)), Plus(start.center, Times(direction, extent)), Color(AXIS_COLORS[axis], 0.55f), 1.0f * dpi);
				}
				drawList->AddLine(start.center, g.center, Color(TINT_NEUTRAL, 0.5f), 1.0f * dpi);
				drawList->AddCircle(start.center, 4.0f * dpi, Color(TINT_NEUTRAL, 0.7f), 0, 1.5f * dpi);
			}

			DrawPlanes(s, drawList, g);
			for (const int i : AxisOrder(g, frame))
			{
				if (!(s.options.handles & AXIS_HANDLES[i]) || g.axisAlpha[i] <= 0.0f)
					continue;
				const float alpha = g.axisAlpha[i];
				const ImU32 color = HandleColor(s, AXIS_HANDLES[i], AXIS_COLORS[i], alpha);
				const ImVec2 &direction = g.directions[i];
				const float reach = Length2(Minus(g.tips[i], g.center));
				const float arrow = std::min(ARROW_LENGTH * dpi, reach * 0.4f);
				const float half = ARROW_HALF * dpi * std::min(1.0f, reach / (g.size * 0.6f));
				const ImVec2 base = Minus(g.tips[i], Times(direction, arrow));
				ShadowLine(drawList, g.starts[i], Plus(base, Times(direction, 1.0f)), color, LINE_WIDTH * dpi, alpha);

				// The cone: two halves in two shades, the lit side toward the upper left
				const ImVec2 side(-direction.y * half, direction.x * half);
				const ImVec2 left = Plus(base, side), right = Minus(base, side);
				const ImVec4 lit = s.active == AXIS_HANDLES[i] || (s.active == HandleNone && s.hovered == AXIS_HANDLES[i]) ? TINT_LIT : AXIS_COLORS[i];
				const float fade = s.active != HandleNone && s.active != AXIS_HANDLES[i] ? alpha * DIMMED : alpha;
				const ImVec4 bright(std::min(1.0f, lit.x * 1.25f + 0.08f), std::min(1.0f, lit.y * 1.25f + 0.08f), std::min(1.0f, lit.z * 1.25f + 0.08f), 1.0f);
				const bool leftLit = side.y < 0.0f || (side.y == 0.0f && side.x < 0.0f);
				drawList->AddTriangle(g.tips[i], left, right, Color(TINT_SHADOW, 0.45f * fade), 2.0f * dpi);
				drawList->AddTriangleFilled(g.tips[i], left, base, Color(leftLit ? bright : lit, fade));
				drawList->AddTriangleFilled(g.tips[i], base, right, Color(leftLit ? lit : bright, fade));
			}

			if (s.options.handles & Center)
			{
				const bool lit = (s.active == HandleNone && s.hovered == Center) || s.active == Center;
				const float dim = s.active != HandleNone && !lit ? DIMMED : 1.0f;
				const float radius = CENTER_RADIUS * dpi;
				drawList->AddCircleFilled(g.center, radius, Color(lit ? TINT_LIT : TINT_NEUTRAL, (lit ? 0.45f : 0.16f) * dim));
				drawList->AddCircle(g.center, radius, Color(TINT_SHADOW, 0.45f * dim), 0, 4.0f * dpi);
				drawList->AddCircle(g.center, radius, Color(lit ? TINT_LIT : TINT_NEUTRAL, dim), 0, 2.0f * dpi);
			}
		}

		/* @brief The segments of a ring on one side (front or back) as polylines, with a shadow under them when given one. */
		void DrawRingRuns(ImDrawList *drawList, const Ring &ring, const bool front, const ImU32 color, const ImU32 shadow, const float width)
		{
			const auto wanted = [&](const int k) { return ring.visible[k] && ring.visible[k + 1] && ring.front[k] == front; };
			std::vector<ImVec2> run;
			const auto flush = [&]()
			{
				if (run.size() > 1)
				{
					if (shadow != 0)
						drawList->AddPolyline(run.data(), static_cast<int>(run.size()), shadow, ImDrawFlags_None, width + 2.0f);
					drawList->AddPolyline(run.data(), static_cast<int>(run.size()), color, ImDrawFlags_None, width);
				}
				run.clear();
			};
			// Start after a segment of the other side (if any) so a run that wraps past the seam stays one line
			int first = 0;
			for (int k = 0; k < RING_SEGMENTS; ++k)
			{
				if (!wanted(k))
				{
					first = k + 1;
					break;
				}
			}
			for (int step = 0; step < RING_SEGMENTS; ++step)
			{
				const int k = (first + step) % RING_SEGMENTS;
				if (!wanted(k))
				{
					flush();
					continue;
				}
				if (run.empty())
					run.push_back(ring.points[k]);
				run.push_back(ring.points[k + 1]);
			}
			flush();
		}

		void DrawRotate(const State &s, ImDrawList *drawList, const Geometry &g, const Frame &frame, const ViewportView &view)
		{
			const float dpi = g.dpi;
			drawList->AddCircleFilled(g.center, g.ringRadius, Color(TINT_NEUTRAL, 0.05f), 64);

			if (s.options.handles & ViewRing)
			{
				const ImU32 color = HandleColor(s, ViewRing, TINT_NEUTRAL, 0.8f);
				drawList->AddCircle(g.center, g.viewRingRadius, Color(TINT_SHADOW, 0.4f), 96, 4.0f * dpi);
				drawList->AddCircle(g.center, g.viewRingRadius, color, 96, 2.0f * dpi);
			}

			// Rings: the lit one last, on top
			std::array<int, 3> order = { 0, 1, 2 };
			std::stable_sort(order.begin(), order.end(), [&](const int a, const int b)
			{
				const bool litA = s.active == AXIS_HANDLES[a] || s.hovered == AXIS_HANDLES[a];
				const bool litB = s.active == AXIS_HANDLES[b] || s.hovered == AXIS_HANDLES[b];
				return litA < litB;
			});
			// The far halves faint and thin (whole when a ring is alone), then the near halves on top
			const bool single = SingleRing(s.options);
			for (const bool front : { false, true })
			{
				for (const int i : order)
				{
					if (!(s.options.handles & AXIS_HANDLES[i]))
						continue;
					const float alpha = s.active != HandleNone && s.active != AXIS_HANDLES[i] ? DIMMED : 1.0f;
					if (front || single)
					{
						const ImU32 color = HandleColor(s, AXIS_HANDLES[i], AXIS_COLORS[i], 1.0f);
						DrawRingRuns(drawList, g.rings[i], front, color, Color(TINT_SHADOW, 0.4f * alpha), RING_WIDTH * dpi);
					}
					else
						DrawRingRuns(drawList, g.rings[i], false, Color(AXIS_COLORS[i], 0.28f * alpha), 0, 1.25f * dpi);
				}
			}
			drawList->AddCircleFilled(g.center, 2.5f * dpi, Color(TINT_NEUTRAL, 0.9f));

			if (s.active == HandleNone)
				return;

			// The angle so far: a pie from where the drag began, and the snapping steps along the ring
			const double turn = std::clamp(s.rotation, -2.0 * PI + 1e-3, 2.0 * PI - 1e-3);
			const int steps = std::max(2, static_cast<int>(std::abs(turn) / (2.0 * PI) * RING_SEGMENTS) + 1);
			std::vector<ImVec2> pie;
			pie.push_back(g.center);
			ImVec2 startPoint = g.center, endPoint = g.center;
			bool projected = true;
			if (s.active == ViewRing)
			{
				for (int k = 0; k <= steps; ++k)
				{
					const double angle = s.startScreenAngle + turn * k / steps;
					pie.emplace_back(g.center.x + g.viewRingRadius * static_cast<float>(std::cos(angle)), g.center.y - g.viewRingRadius * static_cast<float>(std::sin(angle)));
				}
			}
			else
			{
				const Point3 sideways = Cross(s.axis, s.startVector);
				const double radius = g.length * RING_RADIUS;
				for (int k = 0; k <= steps && projected; ++k)
				{
					const double angle = turn * k / steps;
					const Point3 offset = Add(Mul(s.startVector, std::cos(angle) * radius), Mul(sideways, std::sin(angle) * radius));
					ImVec2 point;
					projected = view.WorldToScreen(Add(frame.pivot, offset), point);
					pie.push_back(point);
				}
			}
			if (!projected || pie.size() < 3)
				return;
			startPoint = pie[1];
			endPoint = pie.back();
			if (std::abs(turn) > 1e-4)
				drawList->AddConcavePolyFilled(pie.data(), static_cast<int>(pie.size()), Color(TINT_LIT, 0.22f));
			ShadowLine(drawList, g.center, startPoint, Color(TINT_NEUTRAL, 0.7f), 1.5f * dpi, 0.6f);
			ShadowLine(drawList, g.center, endPoint, Color(TINT_LIT, 1.0f), 2.0f * dpi, 0.8f);

			if (s.options.snap && s.options.snapAngle >= 5.0 && s.active != ViewRing)
			{
				const int axis = AxisOf(s.active);
				const Point3 &u = frame.axes[(axis + 1) % 3], &v = frame.axes[(axis + 2) % 3];
				const double radius = g.length * RING_RADIUS;
				for (double degrees = 0.0; degrees < 360.0 - 1e-6; degrees += s.options.snapAngle)
				{
					const double angle = degrees / DEGREES;
					const Point3 direction = Add(Mul(u, std::cos(angle)), Mul(v, std::sin(angle)));
					if (Dot(direction, g.toEye) < -0.03)
						continue;
					ImVec2 a, b;
					if (view.WorldToScreen(Add(frame.pivot, Mul(direction, radius * 0.93)), a) && view.WorldToScreen(Add(frame.pivot, Mul(direction, radius * 1.07)), b))
						drawList->AddLine(a, b, Color(TINT_NEUTRAL, 0.55f), 1.0f * dpi);
				}
			}
		}

		void DrawScale(const State &s, ImDrawList *drawList, const Geometry &g, const Frame &frame)
		{
			const float dpi = g.dpi;
			DrawPlanes(s, drawList, g);
			for (const int i : AxisOrder(g, frame))
			{
				if (!(s.options.handles & AXIS_HANDLES[i]) || g.axisAlpha[i] <= 0.0f)
					continue;
				const float alpha = g.axisAlpha[i];
				const ImU32 color = HandleColor(s, AXIS_HANDLES[i], AXIS_COLORS[i], alpha);
				// The axis stretches with the scale while dragging
				const ImVec2 tip = Plus(g.center, Times(Minus(g.tips[i], g.center), static_cast<float>(s.scale[i])));
				ShadowLine(drawList, g.starts[i], tip, color, LINE_WIDTH * dpi, alpha);
				const float half = BOX_HALF * dpi;
				drawList->AddRectFilled(ImVec2(tip.x - half - 1.0f, tip.y - half - 1.0f), ImVec2(tip.x + half + 1.0f, tip.y + half + 1.0f), Color(TINT_SHADOW, 0.45f * alpha));
				drawList->AddRectFilled(ImVec2(tip.x - half, tip.y - half), ImVec2(tip.x + half, tip.y + half), color);
			}
			if (s.options.handles & Center)
			{
				const bool lit = (s.active == HandleNone && s.hovered == Center) || s.active == Center;
				const float dim = s.active != HandleNone && !lit ? DIMMED : 1.0f;
				const float half = CENTER_RADIUS * 0.8f * dpi;
				drawList->AddRectFilled(ImVec2(g.center.x - half - 1.5f, g.center.y - half - 1.5f), ImVec2(g.center.x + half + 1.5f, g.center.y + half + 1.5f), Color(TINT_SHADOW, 0.45f * dim));
				drawList->AddRectFilled(ImVec2(g.center.x - half, g.center.y - half), ImVec2(g.center.x + half, g.center.y + half), Color(lit ? TINT_LIT : TINT_NEUTRAL, 0.9f * dim));
			}
		}

		void DrawLabel(ImDrawList *drawList, const std::string &label, const float dpi)
		{
			if (label.empty())
				return;
			const ImVec2 mouse = ImGui::GetIO().MousePos;
			const ImVec2 size = ImGui::CalcTextSize(label.c_str());
			const ImVec2 padding(6.0f * dpi, 3.0f * dpi);
			const ImVec2 min(mouse.x + 18.0f * dpi, mouse.y + 14.0f * dpi);
			const ImVec2 max(min.x + size.x + padding.x * 2.0f, min.y + size.y + padding.y * 2.0f);
			drawList->AddRectFilled(min, max, Color(ImVec4(0.08f, 0.08f, 0.09f, 1.0f), 0.85f), 3.0f * dpi);
			drawList->AddText(ImVec2(min.x + padding.x, min.y + padding.y), Color(TINT_NEUTRAL, 1.0f), label.c_str());
		}
	}

	// -------------------------------------------------------

	Frame Frame::Turned(const Point3 &pivot, const double yaw)
	{
		Frame frame;
		frame.pivot = pivot;
		const double c = std::cos(yaw), s = std::sin(yaw);
		frame.axes[0] = { c, 0.0, -s };
		frame.axes[2] = { s, 0.0, c };
		return frame;
	}

	Result Manipulate(const uint32_t owner, const ViewportView &view, const Frame &frame, const Options &options, const bool allowInput)
	{
		State &s = S();
		const int frameCount = ImGui::GetFrameCount();
		// A drag doesn't survive a frame the gizmo wasn't shown in, or another owner taking it
		if (s.active != HandleNone && (s.owner != owner || s.frameCount < frameCount - 1))
		{
			s.active = HandleNone;
			s.label.clear();
		}
		s.owner = owner;
		s.frameCount = frameCount;

		Result result;
		if (s.waitRelease && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
			s.waitRelease = false;
		const ImVec2 mouse = ImGui::GetIO().MousePos;

		if (s.active == HandleNone)
		{
			s.frame = frame;
			s.options = options;
			s.rest = Build(view, frame);
			s.hovered = allowInput && !s.waitRelease && view.Contains(mouse) ? HitTest(s.rest, options, mouse) : HandleNone;
			if (s.hovered != HandleNone && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().KeyAlt)
			{
				if (BeginDrag(s, view, mouse))
				{
					s.active = s.hovered;
					result.started = true;
				}
			}
		}
		else
			s.options.snap = options.snap;	// Ctrl may be pressed or let go mid-drag

		if (s.active != HandleNone)
		{
			UpdateDrag(s, view, mouse);
			result.delta = s.delta;
			if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				result.finished = true;
				s.active = HandleNone;
				s.hovered = HandleNone;
				s.label.clear();
			}
			else
				result.active = true;
		}

		// What is drawn: the moved gizmo while moving, the start's otherwise
		Frame shown = s.frame;
		if (s.active != HandleNone && s.options.mode == Mode::Translate)
			shown.pivot = Add(shown.pivot, s.translation);
		s.shown = s.active != HandleNone ? Build(view, shown) : s.rest;
		s.shownFrame = shown;

		result.hovered = s.hovered != HandleNone;
		return result;
	}

	void Draw(const ViewportView &view)
	{
		State &s = S();
		if (s.frameCount != ImGui::GetFrameCount() || !s.shown.valid || !view.valid)
			return;
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		drawList->PushClipRect(view.imageMin, ImVec2(view.imageMin.x + view.imageSize.x, view.imageMin.y + view.imageSize.y), true);
		switch (s.options.mode)
		{
			case Mode::Translate:	DrawTranslate(s, drawList, s.shown, s.shownFrame); break;
			case Mode::Rotate:		DrawRotate(s, drawList, s.shown, s.shownFrame, view); break;
			case Mode::Scale:		DrawScale(s, drawList, s.shown, s.shownFrame); break;
		}
		if (s.active != HandleNone)
			DrawLabel(drawList, s.label, s.shown.dpi);
		drawList->PopClipRect();
	}

	bool IsActive()
	{
		return S().active != HandleNone;
	}

	void Cancel()
	{
		State &s = S();
		if (s.active == HandleNone)
			return;
		s.active = HandleNone;
		s.hovered = HandleNone;
		s.label.clear();
		s.waitRelease = ImGui::IsMouseDown(ImGuiMouseButton_Left);
	}

	bool GetHandleScreen(const Handle handle, ImVec2 &position)
	{
		const State &s = S();
		const Geometry &g = s.rest;
		if (!g.valid || !(s.options.handles & handle))
			return false;
		if (handle == Center)
		{
			position = g.center;
			return true;
		}
		if (handle == ViewRing)
		{
			position = ImVec2(g.center.x + g.viewRingRadius, g.center.y);
			return true;
		}
		if (const int plane = PlaneOf(handle); plane >= 0)
		{
			position = ImVec2((g.planes[plane][0].x + g.planes[plane][2].x) * 0.5f, (g.planes[plane][0].y + g.planes[plane][2].y) * 0.5f);
			return g.planeAlpha[plane] > 0.3f;
		}
		const int axis = AxisOf(handle);
		if (axis < 0)
			return false;
		if (s.options.mode == Mode::Rotate)
		{
			// The ring's point nearest the camera (always on its front half)
			const Point3 &u = s.frame.axes[(axis + 1) % 3], &v = s.frame.axes[(axis + 2) % 3];
			const double angle = std::atan2(Dot(v, g.toEye), Dot(u, g.toEye));
			const int k = static_cast<int>(std::floor((angle < 0.0 ? angle + 2.0 * PI : angle) / (2.0 * PI) * RING_SEGMENTS)) % RING_SEGMENTS;
			const Ring &ring = g.rings[axis];
			if (!ring.front[k])
				return false;
			position = ImVec2((ring.points[k].x + ring.points[k + 1].x) * 0.5f, (ring.points[k].y + ring.points[k + 1].y) * 0.5f);
			return true;
		}
		// On the line, two thirds of the way out (scale: at its box)
		position = s.options.mode == Mode::Scale ? g.tips[axis] : Plus(g.starts[axis], Times(Minus(g.tips[axis], g.starts[axis]), 0.66f));
		return g.axisAlpha[axis] > 0.3f;
	}

	Handle GetHovered()
	{
		const State &s = S();
		return s.active != HandleNone ? s.active : s.hovered;
	}

	bool GetFrame(Frame &frame)
	{
		const State &s = S();
		frame = s.frame;
		return s.frameCount >= 0;
	}
}

// -------------------------------------------------------
