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
 * viewport_view.cpp
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#include "viewport_view.h"
#include <algorithm>
#include <cmath>
#include <SceneryEditorX/scene/scene.h>
#include <SceneryEditorX/scene/components/component_sets.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		double Dot(const Point3 &a, const Point3 &b)
		{
			return a.x * b.x + a.y * b.y + a.z * b.z;
		}

		Point3 Cross(const Point3 &a, const Point3 &b)
		{
			return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
		}

		Point3 Normalize(const Point3 &v)
		{
			const double length = std::sqrt(Dot(v, v));
			return length > 1e-12 ? Point3{ v.x / length, v.y / length, v.z / length } : Point3{ 0.0, 0.0, 1.0 };
		}

		Point3 Minus(const Point3 &a, const Point3 &b)
		{
			return { a.x - b.x, a.y - b.y, a.z - b.z };
		}
	}

	ViewportView ViewportView::Build(const ImVec2 &imageMin, const ImVec2 &imageSize, const float aspect)
	{
		ViewportView out;
		out.imageMin = imageMin;
		out.imageSize = imageSize;

		Scene *scene = Scene::GetActive();
		if (!scene || imageSize.x <= 0.0f || imageSize.y <= 0.0f || aspect <= 0.0f)
			return out;

		// The primary camera, else the first: as the renderer picks it
		auto cameras = scene->GetRegistry().view<TransformComponent, CameraComponent>();
		entt::entity selected = entt::null;
		for (const entt::entity entity : cameras)
		{
			if (selected == entt::null)
				selected = entity;
			if (cameras.get<CameraComponent>(entity).isPrimary)
			{
				selected = entity;
				break;
			}
		}
		if (selected == entt::null)
			return out;

		const TransformComponent &transform = cameras.get<TransformComponent>(selected);
		const CameraComponent &camera = cameras.get<CameraComponent>(selected);

		const Vec3 euler = transform.GetRotationEuler();
		const double cosPitch = std::cos(static_cast<double>(euler.x));
		out.forward = Normalize({ cosPitch * std::sin(static_cast<double>(euler.y)), std::sin(static_cast<double>(euler.x)), cosPitch * std::cos(static_cast<double>(euler.y)) });
		out.right = Normalize(Cross(out.forward, { 0.0, 1.0, 0.0 }));
		out.up = Cross(out.right, out.forward);
		out.eye = { transform.translation.x, transform.translation.y, transform.translation.z };

		out.nearPlane = std::max(static_cast<double>(camera.nearPlane), 0.001);
		out.farPlane = std::max(static_cast<double>(camera.farPlane), out.nearPlane + 1.0);
		out.xScale = 1.0 / std::tan(static_cast<double>(camera.horizontalFov_Rad) * 0.5);
		out.yScale = out.xScale * aspect;

		// View: the camera looks down its local -Z (row-vector, so the basis vectors are the columns)
		const Point3 &r = out.right, &u = out.up, &f = out.forward, &e = out.eye;
		const float view[16] = {
			static_cast<float>(r.x), static_cast<float>(u.x), static_cast<float>(-f.x), 0.0f,
			static_cast<float>(r.y), static_cast<float>(u.y), static_cast<float>(-f.y), 0.0f,
			static_cast<float>(r.z), static_cast<float>(u.z), static_cast<float>(-f.z), 0.0f,
			static_cast<float>(-Dot(r, e)), static_cast<float>(-Dot(u, e)), static_cast<float>(Dot(f, e)), 1.0f };
		std::copy(std::begin(view), std::end(view), std::begin(out.view));

		// OpenGL perspective with the renderer's field of view (ImGuizmo only uses it to project its handles)
		const double n = out.nearPlane, fa = out.farPlane;
		const float projection[16] = {
			static_cast<float>(out.xScale), 0.0f, 0.0f, 0.0f,
			0.0f, static_cast<float>(out.yScale), 0.0f, 0.0f,
			0.0f, 0.0f, static_cast<float>((fa + n) / (n - fa)), -1.0f,
			0.0f, 0.0f, static_cast<float>(2.0 * fa * n / (n - fa)), 0.0f };
		std::copy(std::begin(projection), std::end(projection), std::begin(out.projection));

		out.valid = true;
		return out;
	}

	bool ViewportView::WorldToScreen(const Point3 &world, ImVec2 &screen, double *depth) const
	{
		if (!valid)
			return false;
		const Point3 relative = Minus(world, eye);
		const double z = Dot(relative, forward);
		if (depth)
			*depth = z;
		if (z <= nearPlane)
			return false;
		const double ndcX = Dot(relative, right) * xScale / z;
		const double ndcY = Dot(relative, up) * yScale / z;
		screen.x = imageMin.x + static_cast<float>((ndcX * 0.5 + 0.5) * imageSize.x);
		screen.y = imageMin.y + static_cast<float>((0.5 - ndcY * 0.5) * imageSize.y);
		return true;
	}

	void ViewportView::ScreenRay(const ImVec2 &screen, Point3 &origin, Point3 &direction) const
	{
		origin = eye;
		const double ndcX = (static_cast<double>(screen.x) - imageMin.x) / std::max(1.0f, imageSize.x) * 2.0 - 1.0;
		const double ndcY = 1.0 - (static_cast<double>(screen.y) - imageMin.y) / std::max(1.0f, imageSize.y) * 2.0;
		const double sx = ndcX / xScale, sy = ndcY / yScale;
		direction = Normalize({ forward.x + right.x * sx + up.x * sy, forward.y + right.y * sx + up.y * sy, forward.z + right.z * sx + up.z * sy });
	}

	bool ViewportView::RayToPlane(const ImVec2 &screen, const double planeY, Point3 &hit) const
	{
		if (!valid)
			return false;
		Point3 origin, direction;
		ScreenRay(screen, origin, direction);
		if (std::abs(direction.y) < 1e-9)
			return false;
		const double t = (planeY - origin.y) / direction.y;
		if (t <= 0.0 || t > farPlane * 4.0)
			return false;
		hit = { origin.x + direction.x * t, origin.y + direction.y * t, origin.z + direction.z * t };
		return true;
	}

	double ViewportView::PixelsPerMeter(const Point3 &world) const
	{
		const double z = std::max(Dot(Minus(world, eye), forward), nearPlane);
		return xScale * 0.5 * imageSize.x / z;
	}

	bool ViewportView::Contains(const ImVec2 &screen) const
	{
		return screen.x >= imageMin.x && screen.y >= imageMin.y && screen.x < imageMin.x + imageSize.x && screen.y < imageMin.y + imageSize.y;
	}
}

// -------------------------------------------------------
