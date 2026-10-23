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
 * viewport_view.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/renderer/ui/source/imgui/imgui.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/* @brief A world-space point in double precision (airport-scale coordinates, editing math). */
	struct Point3
	{
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
	};

	/**
	 * @struct ViewportView
	 * @brief The camera the renderer draws the viewport through, for overlays drawn over its image and for turning
	 * cursor positions into rays: same camera entity, forward vector, horizontal field of view and aspect as
	 * Renderer::BuildSceneCameraView, so what is drawn here lines up with the rendered pixels.
	 */
	struct ViewportView
	{
		bool valid = false;
		Point3 eye;
		Point3 forward;
		Point3 right;
		Point3 up;
		double xScale = 1.0;			// 1 / tan(horizontal fov / 2)
		double yScale = 1.0;			// xScale * aspect
		double nearPlane = 0.1;
		double farPlane = 10000.0;
		bool orthographic = false;		// Parallel rays along forward; the view is halfWidth x halfHeight meters around the eye
		double halfWidth = 50.0;
		double halfHeight = 50.0;
		ImVec2 imageMin;				// Screen rectangle the image is drawn in
		ImVec2 imageSize;

		// The same camera as row-vector matrices (p' = p * M), OpenGL clip space
		float view[16] = {};
		float projection[16] = {};

		/**
		 * @brief The view of the active scene's primary camera (or its first one).
		 * @param aspect Width / height of the render target (the image may be stretched while the panel resizes).
		 */
		static ViewportView Build(const ImVec2 &imageMin, const ImVec2 &imageSize, float aspect);

		/**
		 * @brief A view from explicit camera values (yaw / pitch in radians as the camera entity stores them): what Build
		 * does with the primary camera, for viewports with cameras of their own.
		 */
		static ViewportView FromCamera(const ImVec2 &imageMin, const ImVec2 &imageSize, float aspect, const Point3 &eye, double yaw, double pitch,
			double horizontalFov, double nearPlane, double farPlane, bool orthographic, double orthographicHeight);

		/* @brief Screen position of a world point; false when it is behind the camera (near plane). */
		bool WorldToScreen(const Point3 &world, ImVec2 &screen, double *depth = nullptr) const;

		/* @brief The ray from the eye through a screen position (direction normalized). */
		void ScreenRay(const ImVec2 &screen, Point3 &origin, Point3 &direction) const;

		/* @brief Where the ray through a screen position meets the horizontal plane y = planeY (false when it doesn't). */
		bool RayToPlane(const ImVec2 &screen, double planeY, Point3 &hit) const;

		/* @brief Screen pixels a meter covers at a world point (for hit tolerances and handle sizes). */
		double PixelsPerMeter(const Point3 &world) const;

		/* @brief True when a screen position lies on the image. */
		bool Contains(const ImVec2 &screen) const;
	};
}

// -------------------------------------------------------
